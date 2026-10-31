#include "ObjectBatchNode.hpp"
#include <common.hpp>
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

    auto& batch = renderer->getObjectBatch();

    // Nothing in this layer can be drawn in this pass right now, don't touch any GL state for it
    if (!batch.isLayerVisible(layerId))
        return;

    PROFILER_PUSH("ObjectBatchNode::draw");

    // Saves the state of the game, uses the shader. The sprite sheet and raw sprite texture
    // samplers were set once when the shader was created.
    renderer->prepareDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, spriteSheetTexture->getName());

    glEnable(GL_BLEND);
    if (layerId.blending)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    batch.draw(layerId);

    renderer->finishDraw();
    PROFILER_POP();
}
