#include "ObjectBatch.hpp"
#include "Shader.hpp"
#include "BProfiler.hpp"
#include "Renderer.hpp"
#include "SpriteMeshDictionary.hpp"
#include "common.hpp"
#include "glm/fwd.hpp"
#include "math/ConvexList.hpp"
#include "math/ConvexPolygon.hpp"
#include <culling/SpriteDrawMap.hpp>
#include <algorithm>
#include <string>
#include "../profiler.hpp"

using namespace geode::prelude;

#define QUAD_BL 0
#define QUAD_BR 1
#define QUAD_TL 2
#define QUAD_TR 3

ObjectBatch::~ObjectBatch() {
    destroyBuffers();
    if (vao)
        glDeleteVertexArrays(1, &vao);
}

void ObjectBatch::destroyBuffers() {
    Buffer** buffers[] = {
        &vertexBuffer, &indexBuffer, &inputSpriteBuffer,
        &preCulledSpritesBuffer, &rawSpriteBuffer, &culledIndexBuffer
    };

    for (Buffer** buffer : buffers) {
        if (*buffer)
            Buffer::destroy(*buffer);
        *buffer = nullptr;
    }

    if (rawSpriteTexture) {
        glDeleteTextures(1, &rawSpriteTexture);
        rawSpriteTexture = 0;
    }

    sortedBatch.reset();
}

#ifdef GEODE_ANDROID

// The unit the vertex shader reads the raw sprites from, no other code in the game uses it
static constexpr u32 RAW_SPRITE_TEXTURE_UNIT = 15;

/*
    Creates the texture the culling compute shader writes the raw sprites into
    (RAW_SPRITE_TEXELS RGBA32F texels per sprite, see headers/shared.h).
    Returns 0 if that failed.
*/
static u32 createRawSpriteTexture(usize spriteCount) {
    usize texels = std::max<usize>(spriteCount, 1) * RAW_SPRITE_TEXELS;
    i32 width  = RAW_SPRITE_TEXTURE_WIDTH;
    i32 height = (i32)((texels + width - 1) / width);

    i32 maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if (height > maxSize) {
        log::error("too many sprites for the raw sprite texture ({} x {} texels, the GPU supports {})", width, height, maxSize);
        return 0;
    }

    while (glGetError() != GL_NO_ERROR) {}

    u32 texture = 0;
    glGenTextures(1, &texture);

    glActiveTexture(GL_TEXTURE0 + RAW_SPRITE_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);
    // Without these the texture is "incomplete" (it has no mipmaps) and reads give zeros
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        log::error("failed to create the raw sprite texture ({} x {} texels), GL error: 0x{:x}", width, height, (u32)error);
        glDeleteTextures(1, &texture);
        return 0;
    }

    return texture;
}

#endif

ObjectBatch::DrawStats ObjectBatch::readDrawStats() {
    DrawStats stats;
    if (!sortedBatch || !sortedBatch->drawCommandBuffer)
        return stats;

    Buffer* buffer = sortedBatch->drawCommandBuffer;
    usize size = buffer->getSize();
    if (size == 0)
        return stats;

    // The compute shader wrote this buffer, make it visible to buffer reads from the CPU
    bgl::memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    glBindBuffer(GL_COPY_READ_BUFFER, buffer->getId());
    const u32* data = (const u32*)glMapBufferRange(GL_COPY_READ_BUFFER, 0, size, GL_MAP_READ_BIT);
    if (!data)
        return stats;

    stats.commands = (u32)(size / (5 * sizeof(u32)));
    for (u32 i = 0; i < stats.commands; i++) {
        u32 count = data[i * 5]; // index count of the command
        if (count > 0)
            stats.nonEmptyCommands++;
        stats.indices += count;
        stats.largestCommand = std::max(stats.largestCommand, count);
    }

    glUnmapBuffer(GL_COPY_READ_BUFFER);
    return stats;
}

void ObjectBatch::bindRawSpriteTexture(Shader* shader) {
    #ifdef GEODE_ANDROID
    if (!rawSpriteTexture)
        return;

    glActiveTexture(GL_TEXTURE0 + RAW_SPRITE_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, rawSpriteTexture);
    glActiveTexture(GL_TEXTURE0);

    shader->setInt(shader->location("u_rawSpriteTexture"), RAW_SPRITE_TEXTURE_UNIT);
    #endif
}

SpriteVertexTransforms ObjectBatch::getSpriteVertexTransform(
    cocos2d::CCSprite* sprite,
    const cocos2d::CCAffineTransform& transform,
    SpriteSheet spriteSheet
) {
    CCRect crop = sprite->getTextureRect();
    
    CCTexture2D* texture = renderer.getSpriteSheetTexture(spriteSheet);
    if (texture == nullptr) return {};

    glm::vec2 posBottomLeft  = ccPointToGLM(CCPointApplyAffineTransform(sprite->getOffsetPosition(), transform));
    glm::vec2 posRightVector = glm::vec2(transform.a, transform.b) * crop.size.width;
    glm::vec2 posUpVector    = glm::vec2(transform.c, transform.d) * crop.size.height;

    glm::vec2 texBottomLeft;
    glm::vec2 texRightVector;
    glm::vec2 texUpVector;

    if (!sprite->isTextureRectRotated()) {
        texBottomLeft  = { crop.origin.x, crop.origin.y + crop.size.height };
        texRightVector = { crop.size.width, 0 };
        texUpVector    = { 0, -crop.size.height };
    } else {
        texBottomLeft  = { crop.origin.x, crop.origin.y };
        texRightVector = { 0, crop.size.width };
        texUpVector    = { crop.size.height, 0 };
    }
    
    if (sprite->isFlipX()) {
        posBottomLeft += posRightVector;
        posRightVector = -posRightVector;
    }
    if (sprite->isFlipY()) {
        posBottomLeft += posUpVector;
        posUpVector = -posUpVector;
    }

    float contentScaleFactor = CCDirector::get()->getContentScaleFactor();
    glm::vec2 texCoordFactor = glm::vec2(
        contentScaleFactor / texture->getPixelsWide(),
        contentScaleFactor / texture->getPixelsHigh()
    );

    texBottomLeft  *= texCoordFactor;
    texRightVector *= texCoordFactor;
    texUpVector    *= texCoordFactor;

    return {
        posBottomLeft, posRightVector, posUpVector,
        texBottomLeft, texRightVector, texUpVector
    };
}

void ObjectBatch::prepareSpriteMeshWrite(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    SpriteType type,
    const cocos2d::CCAffineTransform& transform
) {
    SpriteSheet spriteSheet = ObjectUtils::getSpritesheetOfObject(object, type);
    if (spriteSheetFilter != (SpriteSheet)-1 && spriteSheet != spriteSheetFilter)
        return;

    u32 colorChannel = ObjectUtils::getSpriteColorChannel(type, object, sprite);
    if (type == SpriteType::DETAIL)
        colorChannel |= A_COLOR_CHANNEL_IS_SPRITE_DETAIL;

    currentSpriteVertexTransforms    = getSpriteVertexTransform(sprite, transform, spriteSheet);
    currentSpriteObjectStartPosition = ccPointToGLM(object->m_startPosition);

    if (object != srbIndexObject) {
        srbIndexObject = object;
        srbIndexOfObject = renderer.getObjectSRBIndex(object);
    }

    currentSpriteSRBIndex    = srbIndexOfObject;
    currentSpriteVertexIndex = verticies.size();
}

void ObjectBatch::writeSpriteVertex(glm::vec2 pos) {
    isize index = verticies.size();
    verticies.resize(index + 1);
    ObjectVertex& vertex = verticies[index];

    auto& transforms = currentSpriteVertexTransforms;

    vertex.positionOffset = transforms.positionRight * pos.x +
                            transforms.positionUp    * pos.y +
                            transforms.positionBottomLeft - currentSpriteObjectStartPosition;
    
    vertex.texCoord = transforms.texCoordRight * pos.x +
                      transforms.texCoordUp    * pos.y +
                      transforms.texCoordBottomLeft;

    vertex.spriteIndex = currentSpriteIndex;
}

void ObjectBatch::writeSpriteIndex(u32 index) {
    indicies.push_back(currentSpriteVertexIndex + index);
}

void ObjectBatch::writeSpriteMeshFromConvexList(const ConvexList& list) {
    u32 vertexIndex = 0;
    list.triangulate([&](const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
        writeSpriteVertex(p1);
        writeSpriteVertex(p2);
        writeSpriteVertex(p3);
        writeSpriteIndex(vertexIndex + 0);
        writeSpriteIndex(vertexIndex + 1);
        writeSpriteIndex(vertexIndex + 2);
        vertexIndex += 3;
    });
}

void ObjectBatch::addSprite(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    SpriteType type,
    const cocos2d::CCAffineTransform& transform
) {
    if (object->m_hasNoGlow && type == SpriteType::GLOW)
        return;

    prepareSpriteMeshWrite(object, sprite, type, transform);

    usize indexStart = indicies.size();

    ConvexList* spriteMesh = SpriteMeshDictionary::getSpriteMeshForSprite(sprite);

    currentSpriteIndex = inputSprites.size();

    if (spriteMesh) {
        writeSpriteMeshFromConvexList(*spriteMesh);
    } else {
        writeSpriteVertex({ 0, 0 });
        writeSpriteVertex({ 1, 0 });
        writeSpriteVertex({ 0, 1 });
        writeSpriteVertex({ 1, 1 });

        writeSpriteIndex(QUAD_BL);
        writeSpriteIndex(QUAD_TL);
        writeSpriteIndex(QUAD_TR);
        writeSpriteIndex(QUAD_BL);
        writeSpriteIndex(QUAD_TR);
        writeSpriteIndex(QUAD_BR);
    }

    usize indexCount = indicies.size() - indexStart;

    inputSprites.push_back({
        .objectIndex = (u32)currentSpriteSRBIndex,
        .spriteType = (u32)type,
        .indexStart = (u32)indexStart,
        .indexCount = (u32)indexCount
    });

    if (object != groupIndexObject) {
        groupIndexObject = object;
        groupIndexOfObject = renderer.getGroupManager().getGroupCombinationIndexForObject(object);
    }

    preCulledSprites.push_back({
        .groupCombinationIndex = groupIndexOfObject,
        .startPosition = ccPointToGLM(object->getStartPos())
    });

    ZLayer zLayer = object->getObjectZLayer();
    SpriteSheet spriteSheet = ObjectUtils::getSpritesheetOfObject(object, type);

    usedLayerKeys.insert({zLayer, spriteSheet, false});
    usedLayerKeys.insert({zLayer, spriteSheet, true});

    spriteSorter.addSprite(object, sprite, type, currentSpriteIndex, indexCount);
}

void ObjectBatch::writeGameObject(GameObject* object) {
    float originalScaleX = object->getScaleX();
    float originalScaleY = object->getScaleY();

    if (object->m_usesAudioScale) {
        object->setScaleX(object->m_scaleX);
        object->setScaleY(object->m_scaleY);
    }

    ObjectUtils::unpackObjectIntoSprites(object, [&](const UnpackedSprite& sprite) {
        addSprite(sprite.parentObject, sprite.sprite, sprite.type, sprite.transform);
    });

    object->setScaleX(originalScaleX);
    object->setScaleY(originalScaleY);
}

template <typename T>
static void releaseVector(std::vector<T>& vector) {
    // clear() keeps the memory around, which for a big level is a lot
    std::vector<T>().swap(vector);
}

bool ObjectBatch::finishWriting() {
    BISMUTH_ASSERT(vertexBuffer == nullptr);

    storeGLStates();

    vertexCount = verticies.size();
    indexCount  = indicies.size();

    vertexBuffer           = Buffer::createStaticDraw("Object vertex buffer", verticies.data(), vertexCount * sizeof(ObjectVertex));
    indexBuffer            = Buffer::createStaticDraw("Object index buffer", indicies.data(), indexCount * sizeof(u32));
    inputSpriteBuffer      = Buffer::createStaticDraw("Input sprite buffer", inputSprites.data(), inputSprites.size() * sizeof(InputSprite));
    preCulledSpritesBuffer = Buffer::createStaticDraw("Pre-culled sprites buffer", preCulledSprites.data(), preCulledSprites.size() * sizeof(PreCulledSprite));

    culledIndexBuffer = Buffer::createGPUExclusive("Culled index buffer", indexCount * sizeof(u32));

    #ifdef GEODE_ANDROID
    // Vertex shaders can't read storage buffers on Mali GPUs, so the raw sprites go into a texture
    rawSpriteTexture = createRawSpriteTexture(inputSprites.size());
    bool rawSpritesCreated = rawSpriteTexture != 0;
    #else
    rawSpriteBuffer = Buffer::createGPUExclusive("Raw sprite buffer", inputSprites.size() * sizeof(RawSprite));
    bool rawSpritesCreated = rawSpriteBuffer != nullptr;
    #endif

    bool success = vertexBuffer && indexBuffer && inputSpriteBuffer &&
                   preCulledSpritesBuffer && rawSpritesCreated && culledIndexBuffer;

    if (success) {
        spriteSorter.sortForCurrentBlendingStates();
        sortedBatch = spriteSorter.generateSortedBatch();
        success = sortedBatch != nullptr;
    }

    releaseVector(verticies);
    releaseVector(indicies);
    releaseVector(inputSprites);
    releaseVector(preCulledSprites);

    srbIndexObject   = nullptr;
    groupIndexObject = nullptr;

    if (success)
        prepareVAO();
    else {
        log::error("could not upload the object batch to the GPU (out of video memory?)");
        destroyBuffers();
    }

    restoreGLStates();
    return success;
}

void ObjectBatch::draw(const LayerKey& layer) {
    PROFILER_PUSH("ObjectBatch::draw");

    if (!sortedBatch) {
        PROFILER_POP();
        return;
    }

    auto rangeIt = sortedBatch->workGroupRangePerLayerKey.find(layer);
    if (rangeIt == sortedBatch->workGroupRangePerLayerKey.end() || rangeIt->second.count == 0) {
        PROFILER_POP();
        return;
    }
    const auto& range = rangeIt->second;

    sortedBatch->bindBuffers();
    prepareCullingComputeShader();
    renderer.bindCullingComputeShaderBuffers();
    sortedBatch->drawCommandBuffer->bindAs(GL_DRAW_INDIRECT_BUFFER);
    glBindVertexArray(vao);
    culledIndexBuffer->bindAs(GL_ELEMENT_ARRAY_BUFFER);

    bgl::multiDrawElementsIndirect(
        GL_TRIANGLES,
        GL_UNSIGNED_INT,
        (const void*)(uintptr_t)(range.offset * sizeof(int) * 5),
        range.count,
        sizeof(int) * 5
    );

    PROFILER_POP();
}

void ObjectBatch::predraw() {
    PROFILER_PUSH("ObjectBatch::predraw");

    Shader* computeShader = renderer.getCullingComputeShader();

    if (!sortedBatch || !computeShader || sortedBatch->workGroupCount == 0) {
        PROFILER_POP();
        return;
    }

    cullingComputeShaderTimer.start();

    computeShader->use();

    prepareCullingComputeShader();
    renderer.bindCullingComputeShaderBuffers();
    sortedBatch->bindBuffers();

    // Every invocation processes one work group, the dispatch is rounded up
    u32 threadGroupCount = (sortedBatch->workGroupCount + CULLING_LOCAL_SIZE - 1) / CULLING_LOCAL_SIZE;
    bgl::dispatchCompute(threadGroupCount, 1, 1);

    /*
        The compute shader writes the culled indices, the draw commands and
        the raw sprites. The draw calls read them as an index buffer, as
        indirect commands and as a shader storage buffer. Without this,
        what the draw calls see is undefined (flickering, missing objects
        or even GPU hangs, depending on the driver).
    */
    GLbitfield barriers = GL_SHADER_STORAGE_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT | GL_COMMAND_BARRIER_BIT;
    #ifdef GEODE_ANDROID
    // The raw sprites are written with imageStore and read with texelFetch
    barriers |= GL_TEXTURE_FETCH_BARRIER_BIT;
    #endif
    bgl::memoryBarrier(barriers);

    cullingComputeShaderTimer.stop();
    PROFILER_POP();
}

void ObjectBatch::prepareCullingComputeShader() {
    indexBuffer->bindAsStorageBuffer(INPUT_INDEX_BUFFER_BINDING);
    inputSpriteBuffer->bindAsStorageBuffer(INPUT_SPRITE_BUFFER_BINDING);
    #ifdef GEODE_ANDROID
    // The compute shader writes the raw sprites into the texture with imageStore
    glBindImageTexture(0, rawSpriteTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    #else
    rawSpriteBuffer->bindAsStorageBuffer(RAW_SPRITE_BUFFER_BINDING);
    #endif
    culledIndexBuffer->bindAsStorageBuffer(CULLED_INDEX_BUFFER_BINDING);
    preCulledSpritesBuffer->bindAsStorageBuffer(PRE_CULLED_SPRITE_BUFFER_BINDING);
}

struct AttribTypeInfo {
    i32 openGlType;
    i32 componentCount;
    u32 size;
};

static AttribTypeInfo getInfoOfAttributeTypeString(std::string type) {
    if (type == "float") return { GL_FLOAT, 1, sizeof(float) * 1 };
    if (type == "vec2")  return { GL_FLOAT, 2, sizeof(float) * 2 };
    if (type == "vec3")  return { GL_FLOAT, 3, sizeof(float) * 3 };
    if (type == "vec4")  return { GL_FLOAT, 4, sizeof(float) * 4 };

    if (type == "i8")                   return { GL_BYTE,  1, sizeof(i8)  };
    if (type == "i16")                  return { GL_SHORT, 1, sizeof(i16) };
    if (type == "int" || type == "i32") return { GL_INT,   1, sizeof(i32) };

    if (type == "u8")  return { GL_UNSIGNED_BYTE,  1, sizeof(u8)  };
    if (type == "u16") return { GL_UNSIGNED_SHORT, 1, sizeof(u16) };
    if (type == "u32") return { GL_UNSIGNED_INT,   1, sizeof(u32) };
    
    return { 0, 0 };
}

static void vertexAttribPointer(u32 id, const AttribTypeInfo& info, usize stride, usize offset) {
    if (info.openGlType == GL_FLOAT)
        glVertexAttribPointer(id, info.componentCount, info.openGlType, GL_FALSE, sizeof(ObjectVertex), (void*)offset);
    else if (info.openGlType == GL_DOUBLE)
        glVertexAttribLPointer(id, info.componentCount, info.openGlType, sizeof(ObjectVertex), (void*)offset);
    else
        glVertexAttribIPointer(id, info.componentCount, info.openGlType, sizeof(ObjectVertex), (void*)offset);
}

#define VERTEX_ATTRIBUTE_AS_ATTRIB_POINTER_CALL(ID, TYPE, NAME) \
    { \
        auto info = getInfoOfAttributeTypeString(#TYPE); \
        vertexAttribPointer(ID, info, sizeof(ObjectVertex), offsetof(ObjectVertex, NAME)); \
        glEnableVertexAttribArray(ID); \
    }

void ObjectBatch::prepareVAO() {
    if (vao == 0)
        glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    vertexBuffer->bindAs(GL_ARRAY_BUFFER);

    OBJECT_VERTEX_ATTRIBUTES(VERTEX_ATTRIBUTE_AS_ATTRIB_POINTER_CALL)
}