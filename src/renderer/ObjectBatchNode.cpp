#include "ObjectBatchNode.hpp"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "Renderer.hpp"
#include <profiler.hpp>

using namespace geode::prelude;

bool ObjectBatchNode::init() {
    auto renderer = Renderer::get();
    if (!renderer)
        return false;

    spriteSheetTexture = renderer->getSpriteSheetTexture(layerId.spriteSheet);
    return true;
}

void ObjectBatchNode::draw() {
    if (!spriteSheetTexture)
        return;

    // The renderer is gone (or never finished initializing), nothing to draw with
    auto renderer = Renderer::get();
    if (!renderer)
        return;

    PROFILER_PUSH("ObjectBatchNode::draw");

    auto shader = renderer->prepareDraw();
    shader->setTexture("u_spriteSheetTexture", 0, spriteSheetTexture);

    glEnable(GL_BLEND);
    if (layerId.blending)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    renderer->getObjectBatch().draw(layerId);

    renderer->finishDraw();
    PROFILER_POP();
}