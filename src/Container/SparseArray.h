#pragma once
#include "DenseArray.h"

#include <iterator>
#include <stdexcept>
#include <cstddef>

namespace Gleam::Reflection {

template<typename T>
class SparseArrayView
{
public:

    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        Iterator(const SparseArrayView<T>* parent, size_t pos) 
            : mParent(parent)
            , mCurrentPos(pos)
        {
            
        }

        reference operator*() const
        {
            if (mCurrentPos >= mParent->mIndices.size())
            {
                throw std::out_of_range("Iterator out of range");
            }
            return mParent->mData[mParent->mIndices[mCurrentPos]];
        }

        pointer operator->() const
        {
            if (mCurrentPos >= mParent->mIndices.size())
            {
                throw std::out_of_range("Iterator out of range");
            }
            return &(mParent->mData[mParent->mIndices[mCurrentPos]]);
        }

        Iterator& operator++()
        {
            ++mCurrentPos;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator temp = *this;
            ++mCurrentPos;
            return temp;
        }

        bool operator==(const Iterator& other) const
        {
            return mParent == other.mParent && mCurrentPos == other.mCurrentPos;
        }

        bool operator!=(const Iterator& other) const
        {
            return !(*this == other);
        }
    private:
        size_t mCurrentPos;
        const SparseArrayView<T>* mParent;
    };

    SparseArrayView(const T* data, DenseArrayView<uint32_t> indices) 
        : mData(data)
        , mIndices(indices) {}

    const T& at(size_t pos) const
    {
        if (pos >= mIndices.size()) {
            throw std::out_of_range("Index out of range");
        }
        return mData[mIndices[pos]];
    }

    const T& operator[](size_t pos) const
    {
        return mData[mIndices[pos]];
    }

    size_t size() const
    {
        return mIndices.size();
    }

    bool empty() const
    {
        return mIndices.empty();
    }

    Iterator begin() const
    {
        return Iterator(this, 0);
    }

    Iterator end() const
    {
        return Iterator(this, mIndices.size());
    }

private:

    const T* mData;
    DenseArrayView<uint32_t> mIndices;
};

} // namespace Gleam::Reflection
