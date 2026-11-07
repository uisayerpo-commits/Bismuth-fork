#pragma once

#include <map>
#include <algorithm>
#include <vector>
#include <unordered_set>

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

    // Objects the game creates while the level is running (see Renderer::adoptRuntimeObject)
    bool showRuntimeObjects = true;
    // Per-path switches for the CPU optimizations (bisect aid, read once at init)
    bool optVisibility = true;
    bool optRotation = true;
    bool optMove = true;
    bool optDecoRemoval = true;
    bool hideHighDetailInLDM = true;
    // Parts of the game's own updateVisibility that the optimized version still runs (see optimized_updateVisibility)
    bool visParticles = true;
    bool scaleTriggers = true;
    bool visPreUpdate = false;
    bool visEnterArea = false;
    // Debug tools normally switch the CPU optimizations off, this keeps them on
    bool keepOptimizationsInDebug = false;
    std::map<const void*, GroupStateSnapshot> checkpointGroupStates;
    // Every object that was in the level when it loaded, anything else in a game batch node was created later
    std::unordered_set<GameObject*> initialObjects;
    // How many children each game batch node had the last time they were looked at
    std::map<cocos2d::CCNode*, u32> batchChildCounts;
    void adoptNewRuntimeObjects();
    std::map<cocos2d::CCSpriteBatchNode*, Ref<cocos2d::CCSpriteBatchNode>> runtimeObjectNodes;

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

    // Frame time probe, only runs with debug tools on (logs a [perf] line every 2 seconds)
    static double perfNow();
    void perfBeginFrame(double now);
    void perfEndFrame(double fetchMs, double prepMs, double predrawMs, double totalMs);
    inline void perfAddNode(double ms) { perfNodeFrameMs += ms; perfNodeDraws++; }
    // CPU time of the game's own update and of the whole director loop (see the hooks)
    inline void perfAddUpdate(double ms) { perfUpdateSum += ms; perfUpdateMax = std::max(perfUpdateMax, ms); perfUpdateCount++; }
    inline void perfAddHook(int slot, double ms) { perfHookSum[slot] += ms; }
    inline void perfAddLoop(double ms) { perfLoopSum += ms; perfLoopMax = std::max(perfLoopMax, ms); perfLoopCount++; }

    // Each optimized path, on only while useOptimizations() is on AND its own setting is on
    inline bool getScaleTriggers() const { return scaleTriggers; }
    inline bool getVisParticles() const { return visParticles; }
    inline bool getVisPreUpdate() const { return visPreUpdate; }
    inline bool getVisEnterArea() const { return visEnterArea; }
    inline bool useOptVisibility() { return optVisibility && useOptimizations(); }
    inline bool useOptRotation() { return optRotation && useOptimizations(); }
    inline bool useOptMove() { return optMove && useOptimizations(); }
    inline bool useOptDecoRemoval() { return optDecoRemoval && useOptimizations(); }

    void setEnabled(bool enabled);

    inline GroupManager& getGroupManager() { return groupManager; }
    
    void reset();

    // Practice mode checkpoints: the group states at the time of the checkpoint
    void snapshotGroupStates(const void* checkpoint, cocos2d::CCArray* liveCheckpoints);
    void restoreGroupStates(const void* checkpoint);

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

    /*
        Makes an object that the game created after the level loaded visible. It is called from
        the hook of PlayLayer::addObject. Does nothing for objects
        that don't need it.
    */
    void adoptRuntimeObject(GameObject* object);

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

    // Frame time probe state
    void perfLog(double windowMs);
    double perfLastStart = 0.0;
    double perfWindowStart = 0.0;
    double perfNodeFrameMs = 0.0;
    u32 perfNodeDraws = 0;
    u32 perfCount = 0;
    double perfHookSum[5] = {};
    double perfUpdateSum = 0.0, perfUpdateMax = 0.0;
    double perfLoopSum = 0.0, perfLoopMax = 0.0;
    u32 perfUpdateCount = 0, perfLoopCount = 0;
    double perfSum[5] = {};
    double perfMax[5] = {};
    std::vector<float> perfFrameMs;

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