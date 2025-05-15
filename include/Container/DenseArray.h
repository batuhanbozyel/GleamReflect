#pragma once
#include "Reflection/Utils.h"

#include <iterator>
#include <stdexcept>
#include <cstddef>

namespace Gleam::Reflection {

template <typename T>
class DenseArrayView
{
public:

    class Iterator
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        explicit Iterator(const T* ptr)
            : mCurrent(ptr)
        {

        }

        reference operator*() const
        {
            return *mCurrent;
        }

        pointer operator->() const
        {
            return mCurrent;
        }

        Iterator& operator++()
        {
            ++mCurrent;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator temp = *this;
            ++mCurrent;
            return temp;
        }

        Iterator& operator--()
        {
            --mCurrent;
            return *this;
        }

        Iterator operator--(int)
        {
            Iterator temp = *this;
            --mCurrent;
            return temp;
        }

        Iterator operator+(difference_type n) const
        {
            return Iterator(mCurrent + n);
        }

        Iterator operator-(difference_type n) const
        {
            return Iterator(mCurrent - n);
        }

        Iterator& operator+=(difference_type n)
        {
            mCurrent += n;
            return *this;
        }

        Iterator& operator-=(difference_type n)
        {
            mCurrent -= n;
            return *this;
        }

        difference_type operator-(const Iterator& other) const
        {
            return mCurrent - other.mCurrent;
        }

        reference operator[](difference_type n) const
        {
            return mCurrent[n];
        }

        bool operator==(const Iterator& other) const
        {
            return mCurrent == other.mCurrent;
        }

        bool operator!=(const Iterator& other) const
        {
            return !(*this == other);
        }

        bool operator<(const Iterator& other) const
        {
            return mCurrent < other.mCurrent;
        }

        bool operator>(const Iterator& other) const
        {
            return mCurrent > other.mCurrent;
        }

        bool operator<=(const Iterator& other) const
        {
            return mCurrent <= other.mCurrent;
        }

        bool operator>=(const Iterator& other) const
        {
            return mCurrent >= other.mCurrent;
        }
    private:
        const T* mCurrent;
    };

    DenseArrayView(const T* data, size_t size) 
        : mData(data)
        , mSize(size)
    {

    }

    const T& at(size_t pos) const
    {
        if (pos >= mSize)
        {
            throw std::out_of_range("Index out of range");
        }
        return mData[pos];
    }

    const T& operator[](size_t pos) const
    {
        return mData[pos];
    }

    size_t size() const
    {
        return mSize;
    }

    bool empty() const
    {
        return mSize == 0;
    }

    Iterator begin() const
    {
        return Iterator(mData);
    }

    Iterator end() const
    {
        return Iterator(mData + mSize);
    }

    const T* data() const
    {
        return mData;
    }

private:

    size_t mSize;
    const T* mData;
};

} // namespace Gleam::Reflection
