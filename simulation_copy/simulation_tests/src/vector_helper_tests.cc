
#include <string>

#include "gtest/gtest.h"
#include "utility/vector_helper.h"

namespace simulation
{
TEST(VectorHelper, EraseValue)
{
    // No duplicate
    {
        std::vector<int> vector{1, 2, 3, 4};
        const std::vector<int> expected{1, 3, 4};
        VectorHelper::EraseValue(vector, 2);

        EXPECT_EQ(expected, vector);
    }

    // Duplicates
    {
        std::vector<int> vector{1, 2, 2, 3, 4};
        const std::vector<int> expected{1, 3, 4};
        VectorHelper::EraseValue(vector, 2);

        EXPECT_EQ(expected, vector);
    }

    // Not Found
    {
        std::vector<int> vector{1, 2, 2, 3, 4};
        const std::vector<int> expected = vector;
        VectorHelper::EraseValue(vector, 5);

        EXPECT_EQ(expected, vector);
    }
}

TEST(VectorHelper, EraseValueForCondition)
{
    // Remove numbers greater then 2
    {
        std::vector<int> vector{1, 2, 3, 4};
        const std::vector<int> expected{1, 2};
        VectorHelper::EraseValuesForCondition(
            vector,
            [](const int num)
            {
                return num > 2;
            });

        EXPECT_EQ(expected, vector);
    }

    // Remove even numbers
    {
        std::vector<int> vector{1, 2, 2, 3, 4};
        const std::vector<int> expected{1, 3};
        VectorHelper::EraseValuesForCondition(
            vector,
            [](const int num)
            {
                return num % 2 == 0;
            });

        EXPECT_EQ(expected, vector);
    }
}

TEST(VectorHelper, EraseIndex)
{
    {
        std::vector<int> vector{1, 2, 3, 4};
        const std::vector<int> expected{2, 3, 4};
        VectorHelper::EraseIndex(vector, 0);

        EXPECT_EQ(expected, vector);
    }
    {
        std::vector<int> vector{1, 2, 2, 3, 4};
        const std::vector<int> expected{1, 2, 2, 3};
        VectorHelper::EraseIndex(vector, 4);

        EXPECT_EQ(expected, vector);
    }
}

TEST(VectorHelper, CountValue)
{
    {
        const std::vector<int> vector{1, 2, 3, 4};
        EXPECT_EQ(VectorHelper::CountValue(vector, 2), 1);
        EXPECT_EQ(VectorHelper::CountValue(vector, 3), 1);
    }
    {
        const std::vector<int> vector{1, 2, 2, 3, 4};
        EXPECT_EQ(VectorHelper::CountValue(vector, 2), 2);
        EXPECT_EQ(VectorHelper::CountValue(vector, 0), 0);
    }
    {
        const std::vector<std::string> vector{"1", "2", "3", "2,", "2"};
        EXPECT_EQ(VectorHelper::CountValue(vector, "2"), 2);
        EXPECT_EQ(VectorHelper::CountValue(vector, "3"), 1);
        EXPECT_EQ(VectorHelper::CountValue(vector, "2,"), 1);
        EXPECT_EQ(VectorHelper::CountValue(vector, "a2"), 0);
    }
}

TEST(VectorHelper, ContainsValue)
{
    {
        const std::vector<int> vector{1, 2, 3, 4};
        EXPECT_TRUE(VectorHelper::ContainsValue(vector, 2));
        EXPECT_TRUE(VectorHelper::ContainsValue(vector, 3));
    }
    {
        const std::vector<int> vector{1, 2, 2, 3, 4};
        EXPECT_TRUE(VectorHelper::ContainsValue(vector, 2));
        EXPECT_FALSE(VectorHelper::ContainsValue(vector, 0));
    }
    {
        const std::vector<std::string> vector{"1", "2", "3", "2,", "2"};
        EXPECT_TRUE(VectorHelper::ContainsValue(vector, "2"));
        EXPECT_TRUE(VectorHelper::ContainsValue(vector, "3"));
        EXPECT_TRUE(VectorHelper::ContainsValue(vector, "2,"));
        EXPECT_FALSE(VectorHelper::ContainsValue(vector, "a2"));
    }
}

TEST(VectorHelper, ContainsValuePredicate)
{
    // Test with integers
    {
        const std::vector<int> vector{1, 2, 3, 4};
        EXPECT_TRUE(VectorHelper::ContainsValuePredicate(
            vector,
            [](int val)
            {
                return val == 2;
            }));
        EXPECT_TRUE(VectorHelper::ContainsValuePredicate(
            vector,
            [](int val)
            {
                return val > 3;
            }));
        EXPECT_FALSE(VectorHelper::ContainsValuePredicate(
            vector,
            [](int val)
            {
                return val == 0;
            }));
        EXPECT_FALSE(VectorHelper::ContainsValuePredicate(
            vector,
            [](int val)
            {
                return val > 4;
            }));
    }
    {
        const std::vector<int> vector{1, 2, 2, 3, 4};
        EXPECT_TRUE(VectorHelper::ContainsValuePredicate(
            vector,
            [](int val)
            {
                return val == 2;
            }));
        EXPECT_FALSE(VectorHelper::ContainsValuePredicate(
            vector,
            [](int val)
            {
                return val == 0;
            }));
    }
    // Test with strings
    {
        const std::vector<std::string> vector{"1", "2", "3", "2,", "2"};
        EXPECT_TRUE(VectorHelper::ContainsValuePredicate(
            vector,
            [](const std::string& val)
            {
                return val == "2";
            }));
        EXPECT_TRUE(VectorHelper::ContainsValuePredicate(
            vector,
            [](const std::string& val)
            {
                return val.length() == 2;
            }));  // "2,"
        EXPECT_FALSE(VectorHelper::ContainsValuePredicate(
            vector,
            [](const std::string& val)
            {
                return val == "a2";
            }));
        EXPECT_FALSE(VectorHelper::ContainsValuePredicate(
            vector,
            [](const std::string& val)
            {
                return val.empty();
            }));
    }
    // Test with empty vector
    {
        const std::vector<int> vector{};
        EXPECT_FALSE(VectorHelper::ContainsValuePredicate(
            vector,
            [](int val)
            {
                return val == 1;
            }));
    }
    {
        const std::vector<std::string> vector{};
        EXPECT_FALSE(VectorHelper::ContainsValuePredicate(
            vector,
            [](const std::string& val)
            {
                return val == "test";
            }));
    }
}

TEST(VectorHelper, AppendVector)
{
    // Append into itself
    {
        std::vector<int> vector{1, 2, 3, 4};
        const std::vector<int> expected{1, 2, 3, 4, 1, 2, 3, 4};
        VectorHelper::AppendVector(vector, vector);
        EXPECT_EQ(expected, vector);
    }

    // Append empty
    {
        std::vector<int> vector{1, 2, 3, 4};
        const std::vector<int> expected{1, 2, 3, 4};
        VectorHelper::AppendVector(vector, std::vector<int>{});
        EXPECT_EQ(expected, vector);
    }

    // Append into itself
    {
        std::vector<std::string> vector{"Name1", "Name2"};
        const std::vector<std::string> expected{"Name1", "Name2", "Third", "Fourth"};
        VectorHelper::AppendVector(vector, std::vector<std::string>{"Third", "Fourth"});
        EXPECT_EQ(expected, vector);
    }
}

}  // namespace simulation
