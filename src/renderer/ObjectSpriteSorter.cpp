#include "ObjectSpriteSorter.hpp"
#include "Renderer.hpp"

SortedObjectBatch::~SortedObjectBatch() {
    if (spriteRemapBuffer)
        Buffer::destroy(spriteRemapBuffer);
    if (workGroupBuffer)
        Buffer::destroy(workGroupBuffer);
    if (drawCommandBuffer)
        Buffer::destroy(drawCommandBuffer);
    if (layerInfoBuffer)
        Buffer::destroy(layerInfoBuffer);
    if (workGroupOutputBuffer)
        Buffer::destroy(workGroupOutputBuffer);
    if (spriteStateBuffer)
        Buffer::destroy(spriteStateBuffer);
}

void SortedObjectBatch::bindBuffers() {
    spriteRemapBuffer->bindAsStorageBuffer(SPRITE_REMAP_BUFFER_BINDING);
    workGroupBuffer->bindAsStorageBuffer(WORK_GROUP_BUFFER_BINDING);
    drawCommandBuffer->bindAsStorageBuffer(DRAW_COMMAND_BUFFER_BINDING);
    layerInfoBuffer->bindAsStorageBuffer(LAYER_INFO_BUFFER_BINDING);
    workGroupOutputBuffer->bindAsStorageBuffer(WORK_GROUP_OUTPUT_BUFFER_BINDING);
    spriteStateBuffer->bindAsStorageBuffer(SPRITE_STATE_BUFFER_BINDING);
}

void ObjectSpriteSorter::addSprite(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    SpriteType type,
    u32 inputSpriteIndex,
    u32 vertexCount
) {
    spriteEntries.push_back({
        .zOrder = object->getObjectZOrder(),
        .zLayer = object->getObjectZLayer(),
        .spriteSheet = ObjectUtils::getSpritesheetOfObject(object, type),
        .colorChannelId = (u16)ObjectUtils::getSpriteColorChannel(type, object, sprite),
        .inputSpriteIndex = inputSpriteIndex,
        .vertexCount = vertexCount
    });
}

void ObjectSpriteSorter::sortForCurrentBlendingStates() {
    assert(!spriteEntries.empty());

    sortedSpriteLayers.clear();

    for (auto& sprite : spriteEntries) {
        auto key = sprite.getFullLayerKey();
        sortedSpriteLayers[key].push_back(&sprite);
    }

    for (auto& [key, layer] : sortedSpriteLayers) {
        std::sort(layer.begin(), layer.end(), [](const SpriteEntry* a, const SpriteEntry* b) {
            return a->zOrder < b->zOrder;
        });
    }
}

UPtr<SortedObjectBatch> ObjectSpriteSorter::generateSortedBatch() {
    std::vector<u32> spriteRemap;
    spriteRemap.reserve(spriteEntries.size());

    u32 approxWorkGroupCount = std::min<u32>(spriteEntries.size() / 2 + 1, MAX_WORK_GROUP_COUNT);
    u32 approxSpritesPerWorkGroup = spriteEntries.size() / approxWorkGroupCount;

    std::vector<WorkGroup> workGroups;
    workGroups.reserve(approxWorkGroupCount + 100);

    u32 outIndex = 0;
    std::map<LayerKey, WorkGroupRange> workGroupRangesPerLayerKey;
    std::vector<LayerInfo> layerInfos;
    std::vector<std::array<u32, 35>> layerChannelBits;
    static_assert(COLOR_CHANNEL_BLENDING_WORDS == 35);

    for (auto& [key, layer] : sortedSpriteLayers) {
        // blog::info("Process layer: {}", key.toString());
        u32 workGroupStartIndex = workGroups.size();
        u32 layerIndexBase = outIndex;
        generateWorkGroupsForLayer(key, approxSpritesPerWorkGroup, outIndex, spriteRemap, workGroups);
        u32 workGroupEndIndex = workGroups.size();

        workGroupRangesPerLayerKey[key] = {
            .offset = workGroupStartIndex,
            .count  = workGroupEndIndex - workGroupStartIndex,
            .slot   = (u32)layerInfos.size()
        };

        // The color channels the sprites of the layer use. The CPU uses this to skip the draw pass
        // of a layer when none of its channels is in that blending state.
        std::array<u32, 35> channelBits {};
        for (auto* entry : layer) {
            u32 channel = std::min<u32>(entry->colorChannelId, COLOR_CHANNEL_COUNT - 1);
            channelBits[channel >> 5] |= 1u << (channel & 31);
        }
        layerChannelBits.push_back(channelBits);

        // The layer owns the part of the culled index buffer that fits all of its indices
        layerInfos.push_back({
            .firstWorkGroup = workGroupStartIndex,
            .workGroupCount = workGroupEndIndex - workGroupStartIndex,
            .indexBase  = layerIndexBase,
            .indexTotal = outIndex - layerIndexBase
        });

        // blog::info("Layer processed {}: (index={}, size={})", key.toString(), workGroupStartIndex, workGroupEndIndex - workGroupStartIndex);
    }

    blog::info("Approx sprites per work group: {}", approxSpritesPerWorkGroup);
    blog::info("Work group count: {}", workGroups.size());

    Buffer* spriteRemapBuffer = Buffer::createStaticDraw(
        "Sprite remap buffer",
        spriteRemap.data(),
        spriteRemap.size() * sizeof(u32)
    );

    if (!spriteRemapBuffer)
        return nullptr;

    Buffer* workGroupBuffer = Buffer::createStaticDraw(
        "Work group buffer",
        workGroups.data(),
        workGroups.size() * sizeof(WorkGroup)
    );

    if (!workGroupBuffer) {
        Buffer::destroy(spriteRemapBuffer);
        return nullptr;
    }

    // 2 commands for each layer: the sprites drawn normally, then the sprites drawn additively
    std::vector<DrawCommand> drawCommands;
    drawCommands.resize(layerInfos.size() * 2);

    for (auto& cmd : drawCommands) {
        cmd.baseInstance = 0;
        cmd.baseVertex = 0;
        cmd.instanceCount = 1;
    }

    auto batch = std::make_unique<SortedObjectBatch>();
    batch->spriteRemapBuffer = spriteRemapBuffer;
    batch->workGroupBuffer   = workGroupBuffer;
    batch->workGroupCount    = workGroups.size();
    batch->layerCount        = layerInfos.size();
    batch->layerChannelBits  = std::move(layerChannelBits);
    batch->workGroupRangePerLayerKey = workGroupRangesPerLayerKey;

    // From here on the batch owns the buffers, it destroys what was created if it is dropped early
    batch->drawCommandBuffer = Buffer::createGPUExclusive(
        "Draw command buffer",
        drawCommands.size() * sizeof(DrawCommand)
    );
    batch->layerInfoBuffer = Buffer::createStaticDraw(
        "Layer info buffer",
        layerInfos.data(),
        layerInfos.size() * sizeof(LayerInfo)
    );
    batch->workGroupOutputBuffer = Buffer::createGPUExclusive(
        "Work group output buffer",
        workGroups.size() * sizeof(WorkGroupOutput)
    );
    batch->spriteStateBuffer = Buffer::createGPUExclusive(
        "Sprite state buffer",
        spriteEntries.size() * sizeof(u32)
    );

    if (!batch->drawCommandBuffer || !batch->layerInfoBuffer || !batch->workGroupOutputBuffer || !batch->spriteStateBuffer)
        return nullptr;

    batch->drawCommandBuffer->write(drawCommands.data(), batch->drawCommandBuffer->getSize());

    blog::info("Layer count: {}", layerInfos.size());

    return batch;
}

void ObjectSpriteSorter::generateWorkGroupsForLayer(
    LayerKey layerKey,
    u32 spritesPerWorkGroup,
    u32& culledIndexBufferOutputIndex,
    std::vector<u32>& inputSpriteIndiciesOut,
    std::vector<WorkGroup>& workGroupsOut
) {
    assert(spritesPerWorkGroup > 0);

    const auto& sprites = sortedSpriteLayers[layerKey];

    u32 i = 0;
    while (i < sprites.size()) {
        u32 spritesToAdd = spritesPerWorkGroup;

        u32 culledOutputIndex = culledIndexBufferOutputIndex;
        u32 startIndex = inputSpriteIndiciesOut.size();

        while (spritesToAdd > 0 && i < sprites.size()) {
            auto sprite = sprites[i];
            inputSpriteIndiciesOut.push_back(sprite->inputSpriteIndex);
            culledIndexBufferOutputIndex += sprite->vertexCount;
            spritesToAdd--;
            i++;
        }

        u32 endIndex = inputSpriteIndiciesOut.size();

        // blog::info("  - Work group {} = (index={}, count={}), {}", workGroupsOut.size(), startIndex, endIndex - startIndex, culledOutputIndex);

        workGroupsOut.push_back({
            .inputSpriteStart = startIndex,
            .inputSpriteEnd   = endIndex,
            .culledIndexBufferOutputIndex = culledOutputIndex
        });
    }
}

LayerKey ObjectSpriteSorter::SpriteEntry::getFullLayerKey() const {
    // The blending state is not part of the layer, the culling shader picks the draw pass every frame
    return { zLayer, spriteSheet, false };
}