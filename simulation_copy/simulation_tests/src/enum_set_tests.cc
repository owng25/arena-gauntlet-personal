#include "gtest/gtest.h"
#include "utility/enum_as_index.h"
#include "utility/enum_set.h"

namespace simulation
{

enum class ExampleEnum_ForEnumSet
{
    A,
    B,
    C,
    kNum,
};

ILLUVIUM_ENUM_AS_INDEX(ExampleEnum_ForEnumSet);

TEST(EnumSet, SetAndClear)
{
    using ExampleEnum = ExampleEnum_ForEnumSet;

    EnumSet<ExampleEnum> types;
    EXPECT_FALSE(types.Contains(ExampleEnum::B));
    EXPECT_FALSE(types.Contains(ExampleEnum::A));
    EXPECT_FALSE(types.Contains(ExampleEnum::C));

    types.Add(ExampleEnum::B);
    EXPECT_TRUE(types.Contains(ExampleEnum::B));
    EXPECT_FALSE(types.Contains(ExampleEnum::A));
    EXPECT_FALSE(types.Contains(ExampleEnum::C));

    types.Add(ExampleEnum::A);
    EXPECT_TRUE(types.Contains(ExampleEnum::B));
    EXPECT_TRUE(types.Contains(ExampleEnum::A));
    EXPECT_FALSE(types.Contains(ExampleEnum::C));

    types.Add(ExampleEnum::C);
    EXPECT_TRUE(types.Contains(ExampleEnum::B));
    EXPECT_TRUE(types.Contains(ExampleEnum::A));
    EXPECT_TRUE(types.Contains(ExampleEnum::C));

    types.Remove(ExampleEnum::B);
    ASSERT_FALSE(types.Contains(ExampleEnum::B));
    ASSERT_TRUE(types.Contains(ExampleEnum::A));
    ASSERT_TRUE(types.Contains(ExampleEnum::C));

    types.Remove(ExampleEnum::A);
    ASSERT_FALSE(types.Contains(ExampleEnum::B));
    ASSERT_FALSE(types.Contains(ExampleEnum::A));
    ASSERT_TRUE(types.Contains(ExampleEnum::C));

    types.Remove(ExampleEnum::C);
    ASSERT_FALSE(types.Contains(ExampleEnum::B));
    ASSERT_FALSE(types.Contains(ExampleEnum::A));
    ASSERT_FALSE(types.Contains(ExampleEnum::C));

    {
        EnumSet<ExampleEnum> a;
        a.Add(ExampleEnum::A);
        ASSERT_FALSE(a.Contains(ExampleEnum::B));
        ASSERT_TRUE(a.Contains(ExampleEnum::A));
        ASSERT_FALSE(a.Contains(ExampleEnum::C));

        EnumSet<ExampleEnum> b;
        b.Add(ExampleEnum::B);
        ASSERT_TRUE(b.Contains(ExampleEnum::B));
        ASSERT_FALSE(b.Contains(ExampleEnum::A));
        ASSERT_FALSE(b.Contains(ExampleEnum::C));

        auto c = a.Union(b);

        ASSERT_TRUE(c.Contains(ExampleEnum::B));
        ASSERT_TRUE(c.Contains(ExampleEnum::A));
        ASSERT_FALSE(c.Contains(ExampleEnum::C));
    }
}

TEST(EnumSet, Intersection)
{
    using MyEnum = ExampleEnum_ForEnumSet;

    // Intersection between {A, B} and {B, C} is expected to be {B}
    EXPECT_EQ(
        MakeEnumSet(MyEnum::A, MyEnum::B).Intersection(MakeEnumSet(MyEnum::B, MyEnum::C)),
        MakeEnumSet(MyEnum::B));
}

TEST(EnumSet, Union)
{
    using ExampleEnum = ExampleEnum_ForEnumSet;

    EnumSet<ExampleEnum> a;
    a.Add(ExampleEnum::A);
    ASSERT_FALSE(a.Contains(ExampleEnum::B));
    ASSERT_TRUE(a.Contains(ExampleEnum::A));
    ASSERT_FALSE(a.Contains(ExampleEnum::C));

    EnumSet<ExampleEnum> b;
    b.Add(ExampleEnum::B);
    ASSERT_TRUE(b.Contains(ExampleEnum::B));
    ASSERT_FALSE(b.Contains(ExampleEnum::A));
    ASSERT_FALSE(b.Contains(ExampleEnum::C));

    auto c = a.Union(b);

    ASSERT_TRUE(c.Contains(ExampleEnum::B));
    ASSERT_TRUE(c.Contains(ExampleEnum::A));
    ASSERT_FALSE(c.Contains(ExampleEnum::C));
}

TEST(EnumSet, Create)
{
    using ExampleEnum = ExampleEnum_ForEnumSet;

    {
        const auto set = MakeEnumSet(ExampleEnum::A);
        EXPECT_TRUE(set.Contains(ExampleEnum::A));
        EXPECT_FALSE(set.Contains(ExampleEnum::B));
        EXPECT_FALSE(set.Contains(ExampleEnum::C));
    }

    {
        const auto set = MakeEnumSet(ExampleEnum::A, ExampleEnum::B);
        EXPECT_TRUE(set.Contains(ExampleEnum::A));
        EXPECT_TRUE(set.Contains(ExampleEnum::B));
        EXPECT_FALSE(set.Contains(ExampleEnum::C));
    }
}

TEST(EnumSet, AsVector)
{
    using ExampleEnum = ExampleEnum_ForEnumSet;

    {
        const auto set = MakeEnumSet(ExampleEnum::A, ExampleEnum::C);
        const std::vector<ExampleEnum> expected{ExampleEnum::A, ExampleEnum::C};
        EXPECT_EQ(set.AsVector(), expected);
    }

    {
        const auto set = MakeEnumSet(ExampleEnum::A, ExampleEnum::B, ExampleEnum::C);
        const std::vector<ExampleEnum> expected{ExampleEnum::A, ExampleEnum::B, ExampleEnum::C};
        EXPECT_EQ(set.AsVector(), expected);
    }
}

TEST(EnumSet, Remove)
{
    using ExampleEnum = ExampleEnum_ForEnumSet;

    const auto abc = MakeEnumSet(ExampleEnum::A, ExampleEnum::B, ExampleEnum::C);
    const auto ab = MakeEnumSet(ExampleEnum::A, ExampleEnum::B);
    const auto ac = MakeEnumSet(ExampleEnum::A, ExampleEnum::C);
    const auto bc = MakeEnumSet(ExampleEnum::B, ExampleEnum::C);
    const auto a = MakeEnumSet(ExampleEnum::A);
    const auto b = MakeEnumSet(ExampleEnum::B);
    const auto c = MakeEnumSet(ExampleEnum::C);

    auto minus = [&](EnumSet<ExampleEnum> a, EnumSet<ExampleEnum> b)
    {
        auto copy = a;
        copy.Remove(b);
        return copy;
    };

    EXPECT_EQ(minus(abc, ab), c);
    EXPECT_EQ(minus(abc, ac), b);
    EXPECT_EQ(minus(abc, bc), a);
    EXPECT_EQ(minus(abc, a), bc);
    EXPECT_EQ(minus(abc, b), ac);
    EXPECT_EQ(minus(abc, c), ab);
}

TEST(EnumSet, ForLoop)
{
    using ExampleEnum = ExampleEnum_ForEnumSet;

    const auto abc = MakeEnumSet(ExampleEnum::A, ExampleEnum::B, ExampleEnum::C);
    const auto ab = MakeEnumSet(ExampleEnum::A, ExampleEnum::B);
    const auto ac = MakeEnumSet(ExampleEnum::A, ExampleEnum::C);
    const auto bc = MakeEnumSet(ExampleEnum::B, ExampleEnum::C);
    const auto a = MakeEnumSet(ExampleEnum::A);
    const auto b = MakeEnumSet(ExampleEnum::B);
    const auto c = MakeEnumSet(ExampleEnum::C);

    auto copy_using_loop = [](const EnumSet<ExampleEnum> set)
    {
        EnumSet<ExampleEnum> copy;
        for (ExampleEnum value : set)
        {
            copy.Add(value);
        }

        return copy;
    };

    EXPECT_EQ(copy_using_loop(abc), abc);
    EXPECT_EQ(copy_using_loop(ab), ab);
    EXPECT_EQ(copy_using_loop(ac), ac);
    EXPECT_EQ(copy_using_loop(bc), bc);
    EXPECT_EQ(copy_using_loop(a), a);
    EXPECT_EQ(copy_using_loop(b), b);
    EXPECT_EQ(copy_using_loop(c), c);
}

}  // namespace simulation
