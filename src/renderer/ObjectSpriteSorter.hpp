#pragma once

#include <array>

#include <common.hpp>
#include <ObjectUtils.hpp>
#include "Buffer.hpp"
#include <map>

class Renderer;

struct WorkGroupRange {
    u32 offset = 0, count = 0;
    // Which of the layers this is (the index into the layer info buffer). The draw commands
    // of the layer are command number slot * 2 (normal) and slot * 2 + 1 (additive).
    u32 slot = 0;
};

/*
    Layers are keyed by z layer and sprite sheet, the blending flag of the key is always false.
    Which sprites are drawn normally and which additively is decided by the culling shader every frame.
*/
struct SortedObjectBatch {
    Buffer* spriteRemapBuffer = nullptr;
    Buffer* workGroupBuffer   = nullptr;
    Buffer* drawCommandBuffer = nullptr; // 2 commands per layer, see WorkGroupRange::slot

    // Used by the 3 culling passes, see headers/workGroupBuffer.h
    Buffer* layerInfoBuffer       = nullptr;
    Buffer* workGroupOutputBuffer = nullptr;
    Buffer* spriteStateBuffer     = nullptr;

    u32 workGroupCount = 0;
    u32 layerCount = 0;

    // For every layer (by slot): one bit for each color channel its sprites use
    // (COLOR_CHANNEL_BLENDING_WORDS words, see headers/colorChannelBuffer.h)
    std::vector<std::array<u32, 35>> layerChannelBits;

    std::map<LayerKey, WorkGroupRange> workGroupRangePerLayerKey;

    ~SortedObjectBatch();

    void bindBuffers();
};

class ObjectSpriteSorter {
public:
    void addSprite(
        GameObject* object,
        cocos2d::CCSprite* sprite,
        SpriteType type,
        u32 inputSpriteIndex,
        u32 vertexCount
    );

    void sortForCurrentBlendingStates();

    UPtr<SortedObjectBatch> generateSortedBatch();

private:
    void generateWorkGroupsForLayer(
        LayerKey layerKey,
        u32 spritesPerWorkGroup,
        u32& culledIndexBufferOutputIndex,
        std::vector<u32>& inputSpriteIndiciesOut,
        std::vector<WorkGroup>& workGroupsOut
    );

private:
    struct SpriteEntry {
        i32 zOrder;
        ZLayer zLayer;
        SpriteSheet spriteSheet;
        u16 colorChannelId;
        u32 inputSpriteIndex;
        u32 vertexCount;

        LayerKey getFullLayerKey() const;
    };

    std::vector<SpriteEntry> spriteEntries;
    std::vector<SpriteEntry*> sortedSpriteEntries;

    std::unordered_map<LayerKey, std::vector<SpriteEntry*>> sortedSpriteLayers;
};