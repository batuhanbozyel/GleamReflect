#pragma once
#include "BinaryBuffer.h"
#include "Reflection/Utils.h"

namespace Gleam::Reflection {

class BinaryWriter
{
public:

    BinaryWriter(size_t capacity = 0)
    {
        mBuffer.Allocate(capacity);
    }

    ~BinaryWriter()
    {
        mBuffer.Free();
    }

    BufferView Write(const void* data, size_t size)
    {
		if (size == 0 || data == nullptr)
		{
			return {};
		}

        size_t requiredSize = mCursor + size;
        if (mBuffer.size < requiredSize)
        {
            mBuffer.Resize(requiredSize);
        }

		BufferView view = { mCursor, size };
        memcpy(Utils::OffsetPointer(mBuffer.data, mCursor), data, size);
        mCursor += size;
        return view;
    }

    template<typename T>
    BufferView Write(const T& data)
    {
        return Write(&data, sizeof(T));
    }

    size_t GetCursor() const
    {
        return mCursor;
    }

	const BinaryBuffer& GetBuffer() const
	{
		return mBuffer;
	}

private:

    size_t mCursor = 0;
    BinaryBuffer mBuffer = {};
};

} // namespace Gleam::Reflection
