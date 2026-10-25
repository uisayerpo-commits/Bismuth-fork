#pragma once

#include <Geode/binding/GameObject.hpp>
#include <common.hpp>
#include <Geode/Geode.hpp>
#include <vector>

#include "Buffer.hpp"

class Shader;
#include "glm/fwd.hpp"
#include "math/ConvexList.hpp"
#include "ObjectUtils.hpp"
#include "ObjectSpriteSorter.hpp"
#include "../utils/GPUTimer.hpp"

using namespace geode;

/*
    These are the vertex attributes of the
    object batch. They are written in this
    macro so you don't have duplicate code
    with the and the attrib pointer calls.

    ATTRIB(attributeLocation, type, name)

    NOTE: The positionOffset attribute is this
          vertex' offset from the position of
          the object this sprite belongs to.
*/
#define OBJECT_VERTEX_ATTRIBUTES(ATTRIB) \
    ATTRIB(0, vec2, positionOffset) \
    ATTRIB(1, vec2, texCoord) \
    ATTRIB(2, u32,  spriteIndex)

////////////////////////////////////////////////

#define VERTEX_ATTRIBUTE_AS_STRUCT_MEMBER(ID, TYPE, NAME) \
    TYPE NAME;

#define VERTICIES_PER_QUAD 4
#define INDICIES_PER_QUAD  6

struct ObjectVertex {
    OBJECT_VERTEX_ATTRIBUTES(VERTEX_ATTRIBUTE_AS_STRUCT_MEMBER);
};

struct ObjectQuad {
    union {
        ObjectVertex verticies[VERTICIES_PER_QUAD];
        struct {
            ObjectVertex bl;
            ObjectVertex br;
            ObjectVertex tl;
            ObjectVertex tr;
        };
    };
};

struct ObjectIndicies {
    u32 indicies[INDICIES_PER_QUAD];
};

////////////////////////////////////////////////

class Renderer;

struct SpriteVertexTransforms {
    glm::vec2 positionBottomLeft;
    glm::vec2 positionRight;
    glm::vec2 positionUp;
    glm::vec2 texCoordBottomLeft;
    glm::vec2 texCoordRight;
    glm::vec2 texCoordUp;
};

class ObjectBatch {
public:
    inline ObjectBatch(Renderer& renderer)
        : renderer(renderer) {}
    ~ObjectBatch();

    SpriteVertexTransforms getSpriteVertexTransform(
        cocos2d::CCSprite* sprite,
        const cocos2d::CCAffineTransform& transform,
        SpriteSheet spriteSheet
    );

    void prepareSpriteMeshWrite(
        GameObject* parentObject,
        cocos2d::CCSprite* sprite,
        SpriteType type,
        const cocos2d::CCAffineTransform& transform
    );

    void writeSpriteVertex(glm::vec2 pos);
    void writeSpriteIndex(u32 index);

    void writeSpriteMeshFromConvexList(const ConvexList& list);

    void addSprite(
        GameObject* parentObject,
        cocos2d::CCSprite* sprite,
        SpriteType type,
        const cocos2d::CCAffineTransform& transform
    );

    void writeGameObject(GameObject* object);

    /*
        Uploads everything that was written to the GPU.
        Returns false if that failed (like when the GPU is
        out of memory). The batch can't be drawn in that case.
    */
    bool finishWriting();

    struct DrawStats {
        u32 commands = 0;      // draw commands (one per work group)
        u32 nonEmptyCommands = 0;
        u64 indices = 0;       // indices the commands draw in total
        u32 largestCommand = 0;
    };

    /*
        Reads back the draw commands the culling compute shader wrote. This waits for the
        GPU to finish, so it is only for diagnostics, not something to do every frame.
    */
    DrawStats readDrawStats();

    /*
        Binds the texture with the raw sprites to the texture unit the vertex shader reads
        it from. The shader that is going to draw has to be in use. Does nothing when the
        raw sprites are in a buffer.
    */
    void bindRawSpriteTexture(Shader* shader);

    void draw(const LayerKey& layer);

    inline void bind() {
        glBindVertexArray(vao);
        indexBuffer->bindAs(GL_ELEMENT_ARRAY_BUFFER);
    }

    inline usize getVertexBufferSize() {
        return vertexCount * sizeof(ObjectVertex);
    }

    inline void setSpriteSheetFilter(SpriteSheet sheet) {
        spriteSheetFilter = sheet;
    }

    inline const std::set<LayerKey>& getUsedLayerIds() {
        return usedLayerKeys;
    }

    void predraw();

    void prepareCullingComputeShader();

private:
    void prepareVAO();

    void destroyBuffers();

private:
    Renderer& renderer;

    SpriteSheet spriteSheetFilter = (SpriteSheet)-1;

    // Static buffers
    Buffer* vertexBuffer = nullptr;
    Buffer* indexBuffer = nullptr;
    Buffer* inputSpriteBuffer = nullptr;
    Buffer* preCulledSpritesBuffer = nullptr;

    // GPU buffers
    Buffer* rawSpriteBuffer = nullptr; // not used when the raw sprites are in a texture
    Buffer* culledIndexBuffer = nullptr;

    // Android: the raw sprites, written by the culling compute shader, read by the vertex shader
    u32 rawSpriteTexture = 0;

    GPUTimer cullingComputeShaderTimer { "Culling compute shader time" };

    u32 vao = 0;

    ObjectSpriteSorter spriteSorter;
    UPtr<SortedObjectBatch> sortedBatch;

    std::vector<ObjectVertex> verticies;
    u32 vertexCount = 0;

    std::vector<u32> indicies;
    u32 indexCount = 0;

    std::vector<InputSprite> inputSprites;
    std::vector<PreCulledSprite> preCulledSprites;

    std::set<LayerKey> usedLayerKeys;

    SpriteVertexTransforms currentSpriteVertexTransforms;
    glm::vec2 currentSpriteObjectStartPosition;
    u32 currentSpriteVertexIndex;
    u32 currentSpriteSRBIndex;
    u32 currentSpriteIndex;

    // All the sprites of an object are written after each other, so
    // the lookups that only depend on the object can be remembered.
    GameObject* srbIndexObject = nullptr;
    u32 srbIndexOfObject = 0;

    GameObject* groupIndexObject = nullptr;
    u32 groupIndexOfObject = 0;
};