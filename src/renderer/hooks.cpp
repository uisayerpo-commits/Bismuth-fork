#include "Geode/cocos/CCDirector.h"
#include "Renderer.hpp"
#include <decomp/PlayLayer.hpp>
#include <decomp/GJBaseGameLayer.hpp>
#include <BProfiler.hpp>

using namespace geode::prelude;

static bool newPlayLayer = false;
// static CCArray* allGameObjects = nullptr;

#include <Geode/modify/PlayLayer.hpp>
class $modify(RendererPlayLayer, PlayLayer) {
    // Practice mode checkpoints are created while the level runs, see Renderer::adoptRuntimeObject
    void addObject(GameObject* object) {
        PlayLayer::addObject(object);

        if (auto renderer = Renderer::get())
            renderer->adoptRuntimeObject(object);
    }

    void setupHasCompleted() {
        newPlayLayer = true;
        PlayLayer::setupHasCompleted();
        newPlayLayer = false;
    }

    bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
        // if (!allGameObjects) {
        //     allGameObjects = CCArray::create();
        //     allGameObjects->retain();
        // }
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    // This gets called from inside PlayLayer::setupHasCompleted()
    void resetLevel() {
        if (newPlayLayer) {
            auto batchLayer = this->m_objectLayer;
            if (!batchLayer) {
                // Don't return here, the level still has to be reset or it never starts
                blog::error("failed to attach renderer: batch layer not found");
            } else {
                auto renderer = Renderer::create(this, m_objects);
                if (renderer)
                    batchLayer->addChild(renderer, -100000);
            }

            // for (auto obj : CCArrayExt<GameObject, false>(allGameObjects))
            //     obj->release();
            // allGameObjects->removeAllObjects();
        }

        // Reset first. What the game does while it resets the level (starting from a start
        // position, loading a checkpoint) has to stay, it used to be wiped by a reset after it.
        auto renderer = Renderer::get();
        if (renderer)
            renderer->reset();

        PlayLayer::resetLevel();
    }

    // Practice mode: the game brings back the objects of a checkpoint by itself, the
    // group states live in the renderer so they get saved and restored with it
    CheckpointObject* createCheckpoint() {
        auto checkpoint = PlayLayer::createCheckpoint();

        auto renderer = Renderer::get();
        if (checkpoint && renderer)
            renderer->snapshotGroupStates(checkpoint, m_checkpointArray);

        return checkpoint;
    }

    void loadFromCheckpoint(CheckpointObject* checkpoint) {
        PlayLayer::loadFromCheckpoint(checkpoint);

        auto renderer = Renderer::get();
        if (checkpoint && renderer)
            renderer->restoreGroupStates(checkpoint);
    }

    void updateVisibility(float dt) {
        auto timer = BProfiler::start("PlayLayer::updateVisibility");
        auto renderer = Renderer::get();
        if (renderer && renderer->useOptVisibility()) {
            ((decomp_PlayLayer*)this)->optimized_updateVisibility(dt);
            timer.end();
            return;
        }

        PlayLayer::updateVisibility(dt);
        // ((decomp_PlayLayer*)this)->virtual_updateVisibility(dt);
        timer.end();
    }
};

#include <Geode/modify/GameObject.hpp>
class $modify(MyGameObject, GameObject) {
    /*
    static GameObject* objectFromVector(
        gd::vector<gd::string>& propValues,
        gd::vector<void*>& propIsPresent,
        GJBaseGameLayer* gameLayer,
        bool lowDetail
    ) {
        GameObject* ret = GameObject::objectFromVector(propValues, propIsPresent, gameLayer, lowDetail);
        if (!GameManager::get()->m_playLayer)
            return ret;

        allGameObjects->addObject(ret);
        ret->retain();

        if (ret->m_objectType == GameObjectType::Decoration)
            return ret;
        
        return ret;
    }
    */
};

static void removeDecoObjects(CCArray* array) {
    for (u32 i = 0; i < array->count();) {
        auto object = (GameObject*)array->objectAtIndex(i);
        if (object->m_objectType == GameObjectType::Decoration)
            array->removeObjectAtIndex(i);
        else
            i++;
    }
}

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(RendererGJBaseGameLayer, GJBaseGameLayer) {
    // GJBaseGameLayer::addObject has no binding on android (only PlayLayer::addObject does),
    // so runtime objects are adopted in the PlayLayer hook above.

    void update(float dt) {
        auto renderer = Renderer::get();
        const bool perf = renderer && bismuth::debugTools;
        double perfStart = perf ? Renderer::perfNow() : 0.0;

        auto timer = BProfiler::start("GJBaseGameLayer::update");
        GJBaseGameLayer::update(dt);
        timer.end();

        if (perf)
            renderer->perfAddUpdate(Renderer::perfNow() - perfStart);

        if (renderer)
            renderer->update(dt);
    }

    void processMoveActions() {
        auto renderer = Renderer::get();
        if (renderer == nullptr) {
            GJBaseGameLayer::processMoveActions();
            return;
        }

        m_effectManager->processMoveCalculations();
        for (auto node : m_effectManager->m_unkVector6c0) {
            if (node->m_unk0d0)
                continue;

            int groupId = node->getTag();

            renderer->getGroupManager().moveGroup(groupId, node->m_unk038, node->m_unk040);

            CCArray* objects = getStaticGroup(groupId);
            if (objects)
                moveObjects(objects, node->m_unk038, node->m_unk040, 0);

            if (!renderer->useOptMove()) {
                objects = getOptimizedGroup(groupId);
                if (objects)
                    moveObjects(objects, node->m_unk090, node->m_unk098, 0);
            }
        }
    }

    void siphon_processRotationActions() {
        auto renderer = Renderer::get();
        if (renderer == nullptr) {
            GJBaseGameLayer::processRotationActions();
            return;
        }

        auto eman = m_effectManager;
        for (auto cmdObj : eman->m_unkVector5b0) {
            if (/* cmdObj->m_unkInt204 != m_gameState.m_unkUint2 ||*/ cmdObj->m_someInterpValue1RelatedFalse)
                continue;

            i32 targetId = cmdObj->m_targetGroupID;
            i32 centerId = cmdObj->m_centerGroupID;

            auto mainObject = tryGetMainObject(centerId);
            auto staticGroup = getStaticGroup(targetId);

            float rotation;
            if (staticGroup->count() != 0)
                rotation = cmdObj->m_someInterpValue1RelatedOne - cmdObj->m_someInterpValue1RelatedZero;
            else
                rotation = cmdObj->m_someInterpValue2RelatedOne - cmdObj->m_someInterpValue2RelatedZero;

            if (rotation == 0.0 && !cmdObj->m_finishRelated)
                continue;

            // Idk what the hell this does but sure
            // find() and the iterator, operator[] with a pair key doesn't compile with Geode's STL on Android
            auto unkMapIt = eman->m_unkMap770.find({ targetId, centerId });
            if (unkMapIt != eman->m_unkMap770.end()) {
                for (auto obj : unkMapIt->second) {
                    if (obj->m_someInterpValue1RelatedFalse)
                        continue;
                    if (staticGroup->count() != 0)
                        rotation += cmdObj->m_someInterpValue1RelatedOne - cmdObj->m_someInterpValue1RelatedZero;
                    else
                        rotation += cmdObj->m_someInterpValue2RelatedOne - cmdObj->m_someInterpValue2RelatedZero;
                }
            }

            if (mainObject == nullptr)
                renderer->getGroupManager().rotateGroup(targetId, rotation, cmdObj->m_lockObjectRotation);
            else {
                auto pos = mainObject->getUnmodifiedPosition();
                renderer->getGroupManager().rotateGroup(targetId, rotation, cmdObj->m_lockObjectRotation, ccPointToGLM(pos));
            }
        }

        GJBaseGameLayer::processRotationActions();
    }

    // Scale / transform triggers. The game goes through the active commands and changes
    // every object of the target group on the CPU. Here the same math is applied to the
    // group state instead, the game still runs its own version for the collision.
    void applyTransformCommandsToGroups(GroupManager& groupManager) {
        // The active transform commands, a vector of pointers in the effect manager
        // (offset from the decompiled processTransformActions, GD 2.2081 android64)
        auto base = reinterpret_cast<char*>(m_effectManager) + 0x670;
        auto begin = *reinterpret_cast<GroupCommandObject2***>(base);
        auto end = *reinterpret_cast<GroupCommandObject2***>(base + 8);
        if (begin == nullptr || end <= begin || (end - begin) > 100000)
            return;

        auto rotation = [](float degrees) {
            float radians = -glm::radians(degrees);
            float c = cosf(radians), s = sinf(radians);
            return glm::mat2(c, s, -s, c);
        };
        auto scale = [](double x, double y) {
            return glm::mat2((float)x, 0.f, 0.f, (float)y);
        };
        auto skew = [](double skewXDegrees, double skewYDegrees) {
            return glm::mat2(1.f, tanf(glm::radians((float)skewYDegrees)), tanf(glm::radians((float)skewXDegrees)), 1.f);
        };
        auto keepAwayFromZero = [](double v) {
            const double epsilon = 0.0001;
            if (fabs(v) < epsilon)
                return v < 0.0 ? -epsilon : epsilon;
            return v;
        };

        for (auto it = begin; it != end; ++it) {
            GroupCommandObject2* gco = *it;
            if (gco == nullptr)
                continue;

            int targetId = gco->m_targetGroupID;
            int centerId = gco->m_centerGroupID;

            CCArray* staticGroup = getStaticGroup(targetId);
            bool hasStaticObjects = staticGroup && staticGroup->count() != 0;

            double current = hasStaticObjects ? gco->m_someInterpValue1RelatedOne : gco->m_someInterpValue2RelatedOne;
            double previous = hasStaticObjects ? gco->m_someInterpValue1RelatedZero : gco->m_someInterpValue2RelatedZero;

            if (!gco->m_finishRelated && current == previous)
                continue;

            double targetX = gco->m_targetScaleX - 1.0;
            double targetY = gco->m_targetScaleY - 1.0;

            double newX = targetX * current + 1.0;
            double newY = targetY * current + 1.0;
            double oldX, oldY;
            if (gco->m_someInterpValue1Related == 0.0) {
                oldX = targetX * previous + 1.0;
                oldY = targetY * previous + 1.0;
            } else {
                oldX = gco->m_someInterpValue1Related;
                oldY = gco->m_someInterpValue2Related;
            }

            oldX = keepAwayFromZero(oldX);
            newX = keepAwayFromZero(newX);
            oldY = keepAwayFromZero(oldY);
            newY = keepAwayFromZero(newY);

            GameObject* centerObject = tryGetMainObject(centerId);

            if (centerObject == nullptr) {
                // No center: the objects only scale where they are
                if (!gco->m_onlyMove)
                    groupManager.transformGroup(targetId, scale(newX / oldX, newY / oldY), std::nullopt, true);
                continue;
            }

            float centerRotation = gco->m_relativeRotation ? centerObject->getRotation() : 0.f;

            // Skewing, scaling and rotating around the center, in the same order as the game's nodes
            glm::mat2 linear =
                rotation(centerRotation)
                * scale(newX, newY)
                * skew(current * gco->m_transformTriggerProperty450, current * gco->m_transformTriggerProperty451)
                * skew(-previous * gco->m_transformTriggerProperty450, -previous * gco->m_transformTriggerProperty451)
                * scale(1.0 / oldX, 1.0 / oldY)
                * rotation(-centerRotation);

            auto center = ccPointToGLM(centerObject->getUnmodifiedPosition());
            groupManager.transformGroup(targetId, linear, center, !gco->m_onlyMove);
        }
    }

    void processTransformActions(bool visibleFrame) {
        auto renderer = Renderer::get();
        if (renderer && renderer->getScaleTriggers())
            applyTransformCommandsToGroups(renderer->getGroupManager());

        GJBaseGameLayer::processTransformActions(visibleFrame);
    }

    void processRotationActions() {
        auto renderer = Renderer::get();
        if (renderer == nullptr) {
            GJBaseGameLayer::processRotationActions();
            return;
        }

        if (renderer->useOptRotation())
            ((decomp_GJBaseGameLayer*)this)->optimized_processRotationActions();
        else
            siphon_processRotationActions();
    }

    void processFollowActions() {
        auto renderer = Renderer::get();
        if (renderer == nullptr) {
            GJBaseGameLayer::processFollowActions();
            return;
        }

        auto eman = m_effectManager;
        for (auto node : eman->m_unkVector6d8) {
            i32 targetGroupId = node->getTag();
            i32 followGroupId = node->m_unk074;

            auto mainObject = tryGetMainObject(followGroupId);
            if (mainObject == nullptr)
                continue;

            double moveX = 0.0, moveY = 0.0;
            if (mainObject->m_unk4C4 == m_gameState.m_commandIndex) {
                moveX = (mainObject->m_positionX - mainObject->m_lastPosition.x) * node->m_unk080; /* followXMod */
                moveY = (mainObject->m_positionY - mainObject->m_lastPosition.y) * node->m_unk088; /* followYMod */
            }

            renderer->getGroupManager().moveGroup(targetGroupId, moveX, moveY);
        }

        GJBaseGameLayer::processFollowActions();
    }

    void toggleGroup(int id, bool activate) {
        GJBaseGameLayer::toggleGroup(id, activate);
        auto renderer = Renderer::get();
        if (renderer)
            renderer->getGroupManager().toggleGroup(id, activate);
    }

    void optimizeMoveGroups() {
        GJBaseGameLayer::optimizeMoveGroups();

        auto renderer = Renderer::get();
        if (!renderer || !renderer->useOptDecoRemoval())
            return;

        for (auto array : m_optimizedGroups)
            removeDecoObjects(array);
        for (auto array : m_staticGroups)
            removeDecoObjects(array);
    }
};

#ifndef GEODE_ANDROID
#include <Geode/modify/CCDisplayLinkDirector.hpp>
class $modify(RendererCCDisplayLinkDirector, CCDisplayLinkDirector) {
    void mainLoop() {
        auto ren = Renderer::get();
        if (!ren) {
            CCDisplayLinkDirector::mainLoop();
            return;
        }

        const bool perf = bismuth::debugTools;
        double perfStart = perf ? Renderer::perfNow() : 0.0;

        auto timer = BProfiler::start("Frame time");
        CCDisplayLinkDirector::mainLoop();
        timer.end();

        if (perf)
            ren->perfAddLoop(Renderer::perfNow() - perfStart);

        BProfiler::frameEnd();
    }
};
#endif // GEODE_ANDROID
