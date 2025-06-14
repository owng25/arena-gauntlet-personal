#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "enum_as_index.h"
#include "enum_set.h"

namespace simulation
{

// This class will be defined right after EnumMap because
// readers usually want to read the collection implementation first.
template <typename Container>
class EnumMapIterator;

// A map with enumeration values as keys.
// Requires enumeration to implement EnumAsIndex.
// The map itself does not require extra allocations.
// The order of iteration over this map is deterministic.
template <typename Key, typename Value>
class EnumMap
{
public:
    using KeyType = Key;
    using ValueType = Value;

    // Returns true if value associated with key is present in the map
    bool Contains(const Key key) const
    {
        return values_[EnumToIndex(key)].has_value();
    }

    // Adds key and value to the map.
    // Previous value will be overriden
    Value& Add(const Key key, Value value)
    {
        if (!Contains(key))
        {
            size_ += 1;
        }
        std::optional<Value>& storage = values_[EnumToIndex(key)];
        storage = std::move(value);
        return *storage;
    }

    // Assumes that value associated with the key is present and returns reference to it
    // Aborts if there is no such key. If you want value to be constructed by default,
    // use GetOrAdd method
    Value& Get(const Key key)
    {
        return *values_[EnumToIndex(key)];
    }

    // Assumes that value associated with the key is present and returns reference to it
    // Aborts if there is no such key. If you want value to be constructed by default,
    // use GetOrAdd method
    const Value& Get(const Key key) const
    {
        return *values_[EnumToIndex(key)];
    }

    // Returns reference to value associated with specified key.
    // If value does not exist it will be initialized with default constructor.
    Value& GetOrAdd(const Key key)
    {
        if (Contains(key))
        {
            return Get(key);
        }

        return Add(key, Value{});
    }

    // Removes value associated with specified key and returns it
    std::optional<Value> Remove(const Key key)
    {
        std::optional<Value> result;
        std::swap(values_[EnumToIndex(key)], result);
        if (result.has_value())
        {
            size_ -= 1;
        }
        return result;
    }

    // Returns number of pairs in the map
    size_t Size() const
    {
        return size_;
    }

    // Shortcut for Size() == 0
    bool IsEmpty() const
    {
        return Size() == 0;
    }

    EnumSet<Key> GetKeys() const
    {
        EnumSet<Key> result;

        for (const auto& [key, value] : *this)
        {
            result.Add(key);
        }

        return result;
    }

    void Clear()
    {
        for (auto& storage : values_)
        {
            storage = std::nullopt;
        }

        size_ = 0;
    }

    // STL support section
public:
    using iterator = EnumMapIterator<EnumMap>;
    using const_iterator = EnumMapIterator<const EnumMap>;

    iterator begin()
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

    const_iterator begin() const
    {
        if (IsEmpty())
        {
            return end();
        }
        else
        {
            return const_iterator(*this, 0);
        }
    }

    iterator end()
    {
        return iterator(*this, GetEnumEntriesCount<Key>());
    }

    const_iterator end() const
    {
        return const_iterator(*this, GetEnumEntriesCount<Key>());
    }

private:
    static constexpr size_t kCapacity = GetEnumEntriesCount<Key>();

    // Optional used here to avoid custom object lifetime management
    // (for example manual calls to inplace constructors/destructors)
    std::array<std::optional<Value>, kCapacity> values_;

    // Cache elements count to avoid linear complexity for Size method
    size_t size_ = 0;
};

// Iterator is not parametrized by key and value types directly
// because we need to handle two cases const and non-const reference container.
template <typename Container>
class EnumMapIterator
{
public:
    using KeyType = typename Container::KeyType;
    using ValueType = std::conditional_t<
        std::is_const_v<Container>,                       // if container is const
        std::add_const_t<typename Container::ValueType>,  // then value type is const
        typename Container::ValueType                     // otherwise it is not.
        >;

    // Structure returned when iterator is dereferenced
    struct KeyValue
    {
        // The key in the map. Immutable
        const KeyType key;

        // Reference to the value in map
        ValueType& value;
    };

    EnumMapIterator(Container& container, size_t item_index) : container_(&container), item_index_(item_index)
    {
        while (ShouldAdvance())
        {
            ++item_index_;
        }
    }

    KeyValue operator*() const
    {
        const KeyType key = GetKey();
        return KeyValue{key, container_->Get(key)};
    }

    const EnumMapIterator& operator++()
    {
        do
        {
            ++item_index_;
        } while (ShouldAdvance());

        return *this;
    }

    bool operator==(const EnumMapIterator& other) const
    {
        return container_ == other.container_ && item_index_ == other.item_index_;
    }

    bool operator!=(const EnumMapIterator& other) const
    {
        return !(*this == other);
    }

protected:
    KeyType GetKey() const
    {
        return IndexToEnum<KeyType>(item_index_);
    }

private:
    constexpr bool ShouldAdvance() const
    {
        return item_index_ != GetEnumEntriesCount<KeyType>() && !container_->Contains(GetKey());
    }

    Container* container_;
    size_t item_index_;
};

}  // namespace simulation
