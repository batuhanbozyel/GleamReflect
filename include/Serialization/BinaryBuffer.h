#pragma once
#include <cstring>

namespace Gleam::Reflection {

struct BufferView
{
    size_t offset = 0;
    size_t size = 0;
};

struct BinaryBuffer
{
    void* data = nullptr;
    size_t size = 0;
    
    void Allocate(size_t newSize)
    {
        if (size == newSize || newSize == 0) { return; }
        if (data) { Free(); }
        
        size = newSize;
        data = ::operator new(newSize);
    }
    
    void Free()
    {
        if (data)
        {
            ::operator delete(data);
            data = nullptr;
            size = 0;
        }
    }

    void Resize(size_t newSize)
    {
        if (size >= newSize || newSize == 0) { return; }
        
        void* buffer = ::operator new(newSize);
        if (data)
        { 
            memcpy(buffer, data, size);
            Free();
        }
        size = newSize;
        data = buffer;
    }
};

} // namespace Gleam::Reflection
