#include <unordered_map>

#include "gtest/gtest.h"
#include "utility/enum_as_index.h"
#include "utility/enum_map.h"

namespace simulation
{

enum class ExampleEnumForEnumMap
{
    A,
    B,
    C,
    kNum,
};

ILLUVIUM_ENUM_AS_INDEX(ExampleEnumForEnumMap);

TEST(EnumMap, AddRemoveAndSize)
{
    using MyEnum = ExampleEnumForEnumMap;

    EnumMap<MyEnum, int> m;
    EXPECT_EQ(m.Size(), 0);

    m.Add(MyEnum::A, 1);
    EXPECT_EQ(m.Size(), 1);
    ASSERT_TRUE(m.Contains(MyEnum::A));
    EXPECT_EQ(m.Get(MyEnum::A), 1);

    m.Add(MyEnum::B, 2);
    EXPECT_EQ(m.Size(), 2);
    ASSERT_TRUE(m.Contains(MyEnum::A));
    EXPECT_EQ(m.Get(MyEnum::A), 1);
    ASSERT_TRUE(m.Contains(MyEnum::B));
    EXPECT_EQ(m.Get(MyEnum::B), 2);

    m.Add(MyEnum::C, 3);
    EXPECT_EQ(m.Size(), 3);
    ASSERT_TRUE(m.Contains(MyEnum::A));
    EXPECT_EQ(m.Get(MyEnum::A), 1);
    ASSERT_TRUE(m.Contains(MyEnum::B));
    EXPECT_EQ(m.Get(MyEnum::B), 2);
    ASSERT_TRUE(m.Contains(MyEnum::C));
    EXPECT_EQ(m.Get(MyEnum::C), 3);

    EXPECT_EQ(m.Remove(MyEnum::B), std::optional<int>(2));
    EXPECT_EQ(m.Size(), 2);
    ASSERT_FALSE(m.Contains(MyEnum::B));
    EXPECT_EQ(m.Remove(MyEnum::B), std::nullopt);
    ASSERT_FALSE(m.Contains(MyEnum::B));
    ASSERT_TRUE(m.Contains(MyEnum::A));
    EXPECT_EQ(m.Get(MyEnum::A), 1);
    ASSERT_TRUE(m.Contains(MyEnum::C));
    EXPECT_EQ(m.Get(MyEnum::C), 3);

    EXPECT_EQ(m.Remove(MyEnum::A), std::optional<int>(1));
    EXPECT_EQ(m.Size(), 1);
    ASSERT_FALSE(m.Contains(MyEnum::A));
    EXPECT_EQ(m.Remove(MyEnum::A), std::nullopt);
    ASSERT_FALSE(m.Contains(MyEnum::A));
    ASSERT_FALSE(m.Contains(MyEnum::B));
    ASSERT_TRUE(m.Contains(MyEnum::C));
    EXPECT_EQ(m.Get(MyEnum::C), 3);

    EXPECT_EQ(m.Remove(MyEnum::C), std::optional<int>(3));
    EXPECT_EQ(m.Size(), 0);
    ASSERT_FALSE(m.Contains(MyEnum::C));
    EXPECT_EQ(m.Remove(MyEnum::C), std::nullopt);
    ASSERT_FALSE(m.Contains(MyEnum::A));
    ASSERT_FALSE(m.Contains(MyEnum::B));
    ASSERT_FALSE(m.Contains(MyEnum::C));
}

TEST(EnumMap, ForEachLoop)
{
    using MyEnum = ExampleEnumForEnumMap;

    EnumMap<MyEnum, int> m;
    m.Add(MyEnum::A, 1);
    m.Add(MyEnum::B, 2);
    m.Add(MyEnum::C, 3);

    std::unordered_map<MyEnum, int> m_test;
    m_test[MyEnum::A] = 1;
    m_test[MyEnum::B] = 2;
    m_test[MyEnum::C] = 3;

    for (auto key_value : m)
    {
        EXPECT_EQ(key_value.value, m_test[key_value.key]);
    }

    // The same test BUT walk over constant reference
    for (auto key_value : const_cast<const EnumMap<MyEnum, int>&>(m))
    {
        EXPECT_EQ(key_value.value, m_test[key_value.key]);
    }
}

TEST(EnumMap, GetOrAdd)
{
    using MyEnum = ExampleEnumForEnumMap;

    EnumMap<MyEnum, int> m;
    ASSERT_FALSE(m.Contains(MyEnum::A));
    ASSERT_FALSE(m.Contains(MyEnum::B));
    ASSERT_FALSE(m.Contains(MyEnum::C));
    ASSERT_EQ(m.GetOrAdd(MyEnum::C), 0);
    ASSERT_EQ(m.Get(MyEnum::C), 0);

    m.Add(MyEnum::A, 1);
    m.Add(MyEnum::B, 2);
    m.Add(MyEnum::C, 3);
}

TEST(EnumMap, Clear)
{
    using MyEnum = ExampleEnumForEnumMap;

    EnumMap<MyEnum, int> m;
    m.Add(MyEnum::A, 1);
    m.Add(MyEnum::B, 2);
    m.Add(MyEnum::C, 3);
    m.Clear();
    ASSERT_TRUE(m.IsEmpty());
}

}  // namespace simulation
