#include "utility/enum_as_index.h"

namespace simulation
{
enum class SampleEnumWithNone
{
    kNone,
    kEntryA,
    kEntryB,
    kEntryC,
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(SampleEnumWithNone);
static_assert(EnumToIndex(SampleEnumWithNone::kEntryA) == 0);
static_assert(EnumToIndex(SampleEnumWithNone::kEntryB) == 1);
static_assert(EnumToIndex(SampleEnumWithNone::kEntryC) == 2);
static_assert(GetEnumEntriesCount<SampleEnumWithNone>() == 3);

enum class SampleEnumWithoutNone
{
    kEntryA,
    kEntryB,
    kEntryC,
    kEntryD,
    kNum
};

ILLUVIUM_ENUM_AS_INDEX(SampleEnumWithoutNone);
static_assert(EnumToIndex(SampleEnumWithoutNone::kEntryA) == 0);
static_assert(EnumToIndex(SampleEnumWithoutNone::kEntryB) == 1);
static_assert(EnumToIndex(SampleEnumWithoutNone::kEntryC) == 2);
static_assert(EnumToIndex(SampleEnumWithoutNone::kEntryD) == 3);
static_assert(GetEnumEntriesCount<SampleEnumWithoutNone>() == 4);

}  // namespace simulation
