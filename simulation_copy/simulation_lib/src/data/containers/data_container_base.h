#pragma once

#include <unordered_map>

#include "utility/enum.h"
#include "utility/logger.h"

namespace simulation
{
template <typename InTypeID, typename InDataType, typename TypeIDHasher = std::hash<InTypeID>>
class DataContainerBase
{
public:
    using TypeID = InTypeID;
    using DataType = InDataType;
    using DataPtr = std::shared_ptr<const DataType>;

    DataContainerBase() = default;
    virtual ~DataContainerBase() = default;

    DataContainerBase CreateDeepCopy() const
    {
        DataContainerBase copy;

        // Copy map first because not all DataType have the type_id
        for (const auto& [type_id, data_ptr] : type_id_to_data_map_)
        {
            copy.type_id_to_data_map_[type_id] = data_ptr->CreateDeepCopy();
        }

        // Use the map to populate the vector
        for (const auto& [type_id, data_ptr] : copy.type_id_to_data_map_)
        {
            copy.data_objects_.emplace_back(data_ptr);
        }

        return copy;
    }

    // Add data to container
    bool Add(const TypeID& type_id, const DataPtr& data, Logger& logger)
    {
        if constexpr (std::is_enum_v<TypeID>)
        {
            if (type_id == TypeID::kNone)
            {
                // Something else? error
                logger.LogErr("DataContainerBase::Add - Found an invalid data. type_id = {}. Ignoring.", type_id);
                return false;
            }
        }
        else
        {
            if (!type_id.IsValid())
            {
                // Something else? error
                logger.LogErr("DataContainerBase::Add - Found an invalid data. type_id = {}. Ignoring.", type_id);
                return false;
            }
        }

        data_objects_.push_back(data);
        type_id_to_data_map_[type_id] = data;

        HandleOnAdd(type_id);
        return true;
    }

    // Does the data identified by type_id exists?
    bool Contains(const TypeID& type_id) const
    {
        return type_id_to_data_map_.contains(type_id);
    }

    // Calls passed function object for each data object.
    // Callback signature: bool(const DataType&).
    // If callback returns true iteration will stop and methods returns true
    // otherwise method returns false.
    template <typename Callback>
    bool ForEachWithBreak(Callback&& callback) const
    {
        for (const auto& data_ptr : data_objects_)
        {
            if (callback(*data_ptr))
            {
                return true;
            }
        }

        return false;
    }

    // Calls passed function object for each data object.
    // Callback signature: void(const DataType&).
    template <typename Callback>
    void ForEach(Callback&& callback) const
    {
        // Note: don't implement it using version WithBreak version
        // - nobody likes deep unreadable callstacks
        for (const auto& data_ptr : data_objects_)
        {
            callback(*data_ptr);
        }
    }

    // Calls passed function object for each data object.
    // Callback signature: bool(const DataPtr&).
    // If callback returns true iteration will stop and methods returns true
    // otherwise method returns false.
    template <typename Callback>
    bool ForEachPtrWithBreak(Callback&& callback) const
    {
        for (const auto& data_ptr : data_objects_)
        {
            if (callback(data_ptr))
            {
                return true;
            }
        }

        return false;
    }

    // Calls passed function object for each data object.
    // Callback signature: void(const DataPtr&).
    template <typename Callback>
    void ForEachPtr(Callback&& callback) const
    {
        // Note: don't implement it using version WithBreak version
        // - nobody likes deep unreadable callstacks
        for (const auto& data_ptr : data_objects_)
        {
            callback(data_ptr);
        }
    }

    // Find the data by type_id
    DataPtr Find(const TypeID& type_id) const
    {
        const auto it = type_id_to_data_map_.find(type_id);
        if (it != type_id_to_data_map_.end())
        {
            return it->second;
        }

        return nullptr;
    }

protected:
    // Let child classes have custom on add functionality
    virtual void HandleOnAdd([[maybe_unused]] const TypeID& type_id) {}

private:
    // All data objects
    std::vector<DataPtr> data_objects_{};

    // Helper map to get in a fast way the correct value
    // Key: Type ID
    // Value: Data ptr
    std::unordered_map<TypeID, DataPtr, TypeIDHasher> type_id_to_data_map_{};
};
}  // namespace simulation
