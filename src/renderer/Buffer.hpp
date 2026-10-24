#pragma once

#include <common.hpp>

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

/*
    A GPU buffer.

    Buffers are never left mapped. Anything that has to be updated
    from the CPU keeps its own copy of the data and uploads it with
    write(). Leaving a buffer mapped while the GPU is using it is
    undefined behaviour unless it is a persistent mapping.
*/
class Buffer {
public:
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    inline usize getSize() const { return size; }
    inline u32 getId() const { return id; }

    void read(void* data, usize size, usize offset = 0);

    /*
        Uploads data into the buffer. Writes that go outside of
        the buffer are ignored (and logged).

        If the "use_mapped_buffers" setting is on, writes that
        replace the whole buffer go through glMapBufferRange.
    */
    void write(const void* data, usize size, usize offset = 0);

    inline void bindAs(GLenum binding) {
        glBindBuffer(binding, id);
    }

    inline void bindAsUniformBuffer(u32 binding) {
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
    }

    inline void bindAsStorageBuffer(u32 binding) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
    }

public:
    /*
        Returns nullptr if the buffer could not be created (like when
        the GPU is out of memory). Every caller has to check for that.
    */
    static Buffer* create(const char* name, usize size, GLenum usage);

    inline static void destroy(Buffer* buffer) {
        delete buffer;
    }

    /*
        Buffer to be written to once by the CPU
        and used by the GPU
    */
    inline static Buffer* createStaticDraw(const char* name, const void* data, usize size) {
        auto ret = create(name, size, GL_STATIC_DRAW);
        if (ret && data && size > 0)
            ret->write(data, size);
        return ret;
    }

    /*
        Buffer to be written to many times repeatedly by the CPU
        and used by the GPU
    */
    inline static Buffer* createDynamicDraw(const char* name, usize size) {
        return create(name, size, GL_DYNAMIC_DRAW);
    }

    /*
        Buffer to be written to many times repeatedly by the GPU
        and used by the CPU
    */
    inline static Buffer* createDynamicCopy(const char* name, usize size) {
        return create(name, size, GL_DYNAMIC_COPY);
    }

    /*
        Buffer to be written to and read from by the GPU exclusively
    */
    static Buffer* createGPUExclusive(const char* name, usize size) {
        return create(name, size, GL_DYNAMIC_DRAW); // GL_DYNAMIC_DRAW for now
    }

private:
    Buffer() = default;

private:
    usize size = 0;
    u32 id = 0;
    bool useMappedUploads = false;
};
