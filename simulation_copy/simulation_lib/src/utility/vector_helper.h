#pragma once

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * VectorHelper
 *
 * This defines vector helper functions
 * --------------------------------------------------------------------------------------------------------
 */
class VectorHelper
{
public:
    // Erase vector element by value
    template <typename TValueVector, typename TValue>
    static void EraseValue(std::vector<TValueVector>& vector, const TValue& value_to_delete)
    {
        vector.erase(std::remove(vector.begin(), vector.end(), value_to_delete), vector.end());
    }

    template <typename TValueVector, typename TCondition>
    static void EraseValuesForCondition(std::vector<TValueVector>& vector, const TCondition& condition)
    {
        const auto itr_to_remove = std::remove_if(vector.begin(), vector.end(), condition);
        vector.erase(itr_to_remove, vector.end());
    }

    // Erase element at vector[index]
    template <typename TValueVector>
    static void EraseIndex(std::vector<TValueVector>& vector, const size_t index)
    {
        // Iterators work with long numbers apparently, check if it is in the limit
        const long index_long = static_cast<long>(index);
        assert(index_long < (std::numeric_limits<long>::max)());
        vector.erase(vector.begin() + index_long);
    }

    // Returns the number of search_value in the vector
    template <typename TValueVector, typename TValue>
    static int CountValue(const std::vector<TValueVector>& vector, const TValue& search_value)
    {
        return static_cast<int>(std::count(vector.begin(), vector.end(), search_value));
    }

    // Does search_value exist in the vector?
    template <typename TValueVector, typename TValue>
    static bool ContainsValue(const std::vector<TValueVector>& vector, const TValue& search_value)
    {
        for (const auto& value : vector)
        {
            if (value == search_value)
            {
                return true;
            }
        }

        return false;
    }

    // Does a value satisfying the predicate exist in the vector?
    template <typename TValueVector, typename TPredicate>
    static bool ContainsValuePredicate(const std::vector<TValueVector>& vector, const TPredicate& predicate)
    {
        for (const auto& value : vector)
        {
            if (predicate(value))
            {
                return true;
            }
        }
        return false;
    }

    // Insert vector b at the end of vector_a
    template <typename TValueA, typename TValueB>
    static void AppendVector(std::vector<TValueA>& a, const std::vector<TValueB>& b)
    {
        a.insert(a.end(), b.begin(), b.end());
    }
};

}  // namespace simulation
