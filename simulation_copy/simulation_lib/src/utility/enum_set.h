#pragma once

#include <bitset>
#include <cstddef>
#include <vector>

#include "enum_as_index.h"

namespace simulation
{

template <typename T>
class EnumSetIterator;

// A set of enumeration values.
// Requires enumeration to implement EnumAsIndex.
// It is a fixed sized bitset internally so does not require heap allocation.
// The order of itertion over this set is deterministic.
template <typename T>
class EnumSet
{
public:
    using iterator = EnumSetIterator<T>;

    iterator begin() const
    {
        if (IsEmpty())
        {
            return end();
        }
        else
        {
            return iterator(*this, 0);
        }
    }

    iterator end() const
    {
        return iterator(*this, GetEnumEntriesCount<T>());
    }

    static constexpr size_t kBitsCount = GetEnumEntriesCount<T>();

    // Creates set with specified elements
    // Constructor avoid because it is generally bad idea to use template constructors
    // unless absolutely necessary.
    template <typename... Elements>
    static constexpr EnumSet<T> Create(Elements... elements)
    {
        EnumSet<T> result;
        (result.Add(elements), ...);
        return result;
    }

    // Creates set from another collection which can be iterated using simple C++11 for
    template <typename Collection>
    static constexpr EnumSet<T> CreateFrom(const Collection& collection)
    {
        EnumSet<T> result;
        for (const T element : collection)
        {
            result.Add(element);
        }
        return result;
    }

    constexpr EnumSet() = default;

    // Returns true if specified element is in the set
    constexpr bool Contains(const T value) const
    {
        return bits_[EnumToIndex(value)];
    }

    // Adds element to the set
    void Add(const T value)
    {
        bits_[EnumToIndex(value)] = true;
    }

    // Removes element from the set
    void Remove(const T value)
    {
        bits_[EnumToIndex(value)] = false;
    }

    // Returns union of this set and another one
    EnumSet<T> Union(const EnumSet<T>& another) const
    {
        auto copy = another;
        copy.Add(*this);
        return copy;
    }

    // Returns intersection of this set and another one
    EnumSet<T> Intersection(const EnumSet<T>& another) const
    {
        auto copy = *this;
        copy.bits_ &= another.bits_;
        return copy;
    }

    // Adds all elements of another set to this set
    void Add(const EnumSet<T>& another)
    {
        bits_ |= another.bits_;
    }

    // Removes all elements of another set from this set
    void Remove(const EnumSet<T>& another)
    {
        bits_ &= ~another.bits_;
    }

    // Returns false if the set is empty
    bool IsEmpty() const
    {
        return bits_.none();
    }

    // Removes all elements from the set
    void Clear()
    {
        bits_.reset();
    }

    size_t Size() const
    {
        return bits_.count();
    }

    std::vector<T> AsVector() const
    {
        std::vector<T> vector;
        for (T value : *this)
        {
            vector.push_back(value);
        }
        return vector;
    }

    constexpr bool operator==(const EnumSet<T>& another) const
    {
        return bits_ == another.bits_;
    }

    constexpr bool operator!=(const EnumSet<T>& another) const
    {
        return !(*this == another);
    }

    EnumSet<T> operator-(const EnumSet<T>& another) const
    {
        auto copy = *this;
        copy.Remove(another);
        return copy;
    }

    EnumSet<T> operator+(const EnumSet<T>& another) const
    {
        auto copy = *this;
        copy.Add(another);
        return copy;
    }

    // Makes enum set with all possible elements for this enumeration
    static constexpr EnumSet<T> MakeFull()
    {
        EnumSet<T> result;
        result.bits_ = ~result.bits_;
        return result;
    }

    // Returns false if the set is empty
    bool empty() const
    {
        return IsEmpty();
    }

private:
    std::bitset<kBitsCount> bits_{};
};

template <typename T>
class EnumSetIterator
{
public:
    using Container = EnumSet<T>;

    EnumSetIterator(const Container& container, size_t item_index) : container_(&container), item_index_(item_index)
    {
        while (ShouldAdvance())
        {
            ++item_index_;
        }
    }

    T GetValue() const
    {
        return IndexToEnum<T>(item_index_);
    }

    T operator*() const
    {
        return GetValue();
    }

    const EnumSetIterator& operator++()
    {
        do
        {
            ++item_index_;
        } while (ShouldAdvance());

        return *this;
    }

    bool operator==(const EnumSetIterator& other) const
    {
        return container_ == other.container_ && item_index_ == other.item_index_;
    }

    bool operator!=(const EnumSetIterator& other) const
    {
        return !(*this == other);
    }

private:
    constexpr bool ShouldAdvance() const
    {
        return item_index_ != GetEnumEntriesCount<T>() && !container_->Contains(GetValue());
    }

    const Container* container_;
    size_t item_index_;
};

template <typename First, typename... Tail>
constexpr inline EnumSet<First> MakeEnumSet(First element, Tail... elements)
{
    return EnumSet<First>::Create(element, elements...);
}

}  // namespace simulation

namespace std
{
// STL requires iterator traits to be defined for custom iterators.
// We don't want them all so we will be adding them when using some algorithm which uses it.
// For example, value_type is required by fmt::join.
template <typename T>
struct iterator_traits<simulation::EnumSetIterator<T>>
{
    using value_type = T;
};
}  // namespace std
