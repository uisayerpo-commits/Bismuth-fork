#include "Buffer.hpp"

using namespace geode::prelude;

/*
    COPY_READ/COPY_WRITE are used as the binding points for uploads and
    readbacks. Nothing else in the game uses them, so unlike ARRAY_BUFFER
    there is no previous binding that has to be queried and restored.
*/

Buffer::~Buffer() {
    glDeleteBuffers(1, &id);
}

void Buffer::read(void* data, usize size, usize offset) {
    if (!data || size == 0)
        return;

    if (offset + size > this->size) {
        log::error("Buffer::read out of bounds (offset: {}, size: {}, buffer size: {})", offset, size, this->size);
        return;
    }

    glBindBuffer(GL_COPY_READ_BUFFER, id);
    glGetBufferSubData(GL_COPY_READ_BUFFER, offset, size, data);
}

void Buffer::write(const void* data, usize size, usize offset) {
    if (!data || size == 0)
        return;

    if (offset + size > this->size) {
        log::error("Buffer::write out of bounds (offset: {}, size: {}, buffer size: {})", offset, size, this->size);
        return;
    }

    glBindBuffer(GL_COPY_WRITE_BUFFER, id);

    if (useMappedUploads && offset == 0 && size == this->size) {
        // Invalidating the buffer lets the driver hand out fresh memory
        // instead of waiting for the GPU to finish with the old contents.
        void* dst = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        if (dst) {
            memcpy(dst, data, size);
            if (glUnmapBuffer(GL_COPY_WRITE_BUFFER))
                return;
            // The contents were lost while mapped, upload them again below
        }
    }

    glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data);
}

Buffer* Buffer::create(const char* name, usize size, GLenum usage) {
    // Throw away errors left behind by other code so only ours get reported
    while (glGetError() != GL_NO_ERROR) {}

    u32 buffer = 0;
    glGenBuffers(1, &buffer);

    glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
    glBufferData(GL_COPY_WRITE_BUFFER, size, NULL, usage);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        log::error("failed to allocate buffer '{}' ({} bytes), GL error: 0x{:x}", name, size, (u32)error);
        glDeleteBuffers(1, &buffer);
        return nullptr;
    }

    // The buffer has to exist (be bound once) before it can get a label
    glObjectLabel(GL_BUFFER, buffer, strlen(name), name);

    auto ret = new Buffer();
    ret->id = buffer;
    ret->size = size;
    ret->useMappedUploads = Mod::get()->getSettingValue<bool>("use_mapped_buffers");

    return ret;
}
