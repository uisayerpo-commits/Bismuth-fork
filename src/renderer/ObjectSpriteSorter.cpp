#include "ObjectSpriteSorter.hpp"
#include "Renderer.hpp"

SortedObjectBatch::~SortedObjectBatch() {
    if (spriteRemapBuffer)
        Buffer::destroy(spriteRemapBuffer);
    if (workGroupBuffer)
        Buffer::destroy(workGroupBuffer);
    if (drawCommandBuffer)
        Buffer::destroy(drawCommandBuffer);
}

void SortedObjectBatch::bindBuffers() {
    spriteRemapBuffer->bindAsStorageBuffer(SPRITE_REMAP_BUFFER_BINDING);
    workGroupBuffer->bindAsStorageBuffer(WORK_GROUP_BUFFER_BINDING);
    drawCommandBuffer->bindAsStorageBuffer(DRAW_COMMAND_BUFFER_BINDING);
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

    for (auto& [key, layer] : sortedSpriteLayers) {
        // log::info("Process layer: {}", key.toString());
        u32 workGroupStartIndex = workGroups.size();
        generateWorkGroupsForLayer(key, approxSpritesPerWorkGroup, outIndex, spriteRemap, workGroups);
        u32 workGroupEndIndex = workGroups.size();

        workGroupRangesPerLayerKey[key] = {
            .offset = workGroupStartIndex,
            .count  = workGroupEndIndex - workGroupStartIndex
        };

        // log::info("Layer processed {}: (index={}, size={})", key.toString(), workGroupStartIndex, workGroupEndIndex - workGroupStartIndex);
    }

    log::info("Approx sprites per work group: {}", approxSpritesPerWorkGroup);
    log::info("Work group count: {}", workGroups.size());

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

    std::vector<DrawCommand> drawCommands;
    drawCommands.resize(workGroups.size());

    for (auto& cmd : drawCommands) {
        cmd.baseInstance = 0;
        cmd.baseVertex = 0;
        cmd.instanceCount = 1;
    }

    Buffer* drawCommandBuffer = Buffer::createGPUExclusive(
        "Draw command buffer",
        workGroups.size() * sizeof(DrawCommand)
    );

    if (!drawCommandBuffer) {
        Buffer::destroy(spriteRemapBuffer);
        Buffer::destroy(workGroupBuffer);
        return nullptr;
    }

    drawCommandBuffer->write(drawCommands.data(), drawCommandBuffer->getSize());

    return std::make_unique<SortedObjectBatch>(
        spriteRemapBuffer,
        workGroupBuffer,
        drawCommandBuffer,
        workGroups.size(),
        workGroupRangesPerLayerKey
    );
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

        // log::info("  - Work group {} = (index={}, count={}), {}", workGroupsOut.size(), startIndex, endIndex - startIndex, culledOutputIndex);

        workGroupsOut.push_back({
            .inputSpriteStart = startIndex,
            .inputSpriteEnd   = endIndex,
            .culledIndexBufferOutputIndex = culledOutputIndex
        });
    }
}

LayerKey ObjectSpriteSorter::SpriteEntry::getFullLayerKey() const {
    auto renderer = Renderer::get();
    return { zLayer, spriteSheet, renderer && renderer->isColorChannelBlending(colorChannelId) };
}