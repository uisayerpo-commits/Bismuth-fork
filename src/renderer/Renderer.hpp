#pragma once

#include <Geode/binding/GameObject.hpp>
#include <common.hpp>
#include "Geode/cocos/sprite_nodes/CCSpriteBatchNode.h"
#include "ObjectBatch.hpp"
#include "ObjectUtils.hpp"
#include "OverdrawView.hpp"
#include "Shader.hpp"
#include <unordered_map>
#include <array>

#include "DifferenceMode.hpp"
#include "GroupManager.hpp"
#include "ObjectBatchNode.hpp"
#include "../../resources/shaders/headers/shared.h"

using namespace geode;

class Renderer : public cocos2d::CCNode {
private:
    inline Renderer()
        : objectBatch(*this), groupManager(*this), differenceMode(*this),
          overdrawView(*this) {}
    ~Renderer() override;

    bool init(PlayLayer* layer, cocos2d::CCArray* objects);

    void generateBatchNodes();

    void terminate();

    void prepareShaderUniforms();

    // Logs what the GPU is working with every few seconds (Android port debugging)
    void logDiagnostics();

    // True on the frames the culling shader counts sprites and the log gets written
    bool collectDiagnostics = false;
    i64 diagnosticsBucket = -1;

    void prepareColorChannelBuffer();

    bool generateStaticRenderingBuffer(std::vector<GameObject*>& objects);

    void draw() override;

    void finish();

    void updateDebugText();

    void bindCullingComputeShaderBuffers();

    void fetchBlendingStates();

    friend class FinishNode;
    friend class ObjectBatch;

protected:
    Shader* prepareDraw();

    void finishDraw();

    friend class GroupManager;
    friend class ObjectBatchNode;

public:
    void update(float dt) override;

    cocos2d::CCSpriteBatchNode* getSpriteBatchNodeWithLayerId(LayerKey id);

    inline bool isColorChannelBlending(i32 channel) {
        if (channel < 0 || (usize)channel >= colorChannelsIsBlending.size())
            return false;
        return colorChannelsIsBlending[channel];
    }

    inline cocos2d::CCTexture2D* getSpriteSheetTexture(SpriteSheet sheet) {
        if ((i32)sheet < 0 || (i32)sheet >= (i32)SpriteSheet::COUNT)
            return nullptr;
        return spriteSheets[(i32)sheet];
    }

    inline PlayLayer* getPlayLayer() {
        return layer;
    }

    inline usize getObjectSRBIndex(GameObject* object) {
        return objectSRBIndicies[object];
    }

    inline bool isEnabled() { return enabled; }

    inline DifferenceMode* getDifferenceMode() {
        if (!differenceModeEnabled) return nullptr;
        return &differenceMode;
    }

    inline void toggleDebugText() {
        debugTextEnabled = !debugTextEnabled;
    }

    inline bool isDifferenceModeEnabled() const { return differenceModeEnabled; }
    inline void setDifferenceModeEnabled(bool enabled) {
        differenceModeEnabled = enabled;
    }

    inline bool canEnableDisableIngame() const { return ingameEnableDisable; }

    inline bool isPaused() {
        return layer->m_isPaused;
    }

    inline bool isUseIndexCulling() const { return useIndexCulling; }

    inline ObjectBatch& getObjectBatch() { return objectBatch; }

    inline Shader* getCullingComputeShader() const {
        return cullingComputeShader;
    }

    // One bit per color channel, 1 while the channel is blending (NULL before the renderer is ready)
    inline const u32* getChannelBlendingBits() const {
        return colorChannelBuffer ? colorChannelBuffer->u_channelBlending : nullptr;
    }

    inline Shader* getCullingScanShader() const {
        return cullingScanShader;
    }

    inline Shader* getCullingCopyShader() const {
        return cullingCopyShader;
    }

    bool useOptimizations();

    void setEnabled(bool enabled);

    inline GroupManager& getGroupManager() { return groupManager; }
    
    void reset();

    void drawLine(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color);

public:
    static Ref<Renderer> create(PlayLayer* layer, cocos2d::CCArray* objects);

    /**
     * Returns the current renderer. If the game
     * is not in a PlayLayer. This returns nullptr.
     *
     * This is a raw pointer on purpose. It gets called many times
     * per frame from hooks, and creating a Ref every time means
     * an unnecessary retain and release.
     */
    static Renderer* get();

private:
    PlayLayer* layer = nullptr;

    bool enabled = false;
    bool debugTextEnabled = false;
    bool ingameEnableDisable = false;

    bool useIndexCulling = false;

    u64 rendererStartTime = 0;

    usize spritesOnScreen = 0;

    std::unordered_map<GameObject*, usize> objectSRBIndicies;

    u32 timeElapsedQuery = 0;

    glm::vec2 cameraCenterPos = { 0.f, 0.f };

    std::vector<ObjectBatchNode*> batchNodes;

    cocos2d::CCTexture2D* spriteSheets[(i32)SpriteSheet::COUNT] = { nullptr };

    std::array<bool, 1101> colorChannelsIsBlending;

    Ref<cocos2d::CCLabelBMFont> debugText;
    Ref<cocos2d::CCLabelBMFont> debugTextOutline1;
    Ref<cocos2d::CCLabelBMFont> debugTextOutline2;

    // Setting the text of a label is expensive, so it is only done when it changed
    std::string lastDebugText;

    GPUTimer renderingTimer { "Rendering time" };

    GroupManager groupManager;

    ObjectBatch objectBatch;
    Shader* shader = nullptr;
    Shader* basicShader = nullptr;
    Shader* cullingComputeShader = nullptr;
    Shader* cullingScanShader = nullptr;
    Shader* cullingCopyShader = nullptr;

    // CPU side copy of the color channel buffer, uploaded when it changed
    UPtr<ColorChannelBuffer> colorChannelBuffer;
    UPtr<ColorChannelBuffer> lastUploadedColorChannels;
    bool colorChannelsUploaded = false;
    Buffer* colorChannelBufferObject = nullptr;

    Buffer* srbBuffer = nullptr;

    GameStateBuffer gameState;
    Buffer* gameStateBuffer = nullptr;

    bool differenceModeEnabled = false;
    DifferenceMode differenceMode;
    OverdrawView overdrawView;

    float gameTimer = 0.0;

    i64 groupStateCount = 0;
    i64 renderedGameObjectCount = 0;
    i64 renderTime = 0;
};

void storeGLStates();

void restoreGLStates();