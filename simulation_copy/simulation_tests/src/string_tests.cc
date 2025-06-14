#include "base_json_test.h"
#include "gtest/gtest.h"
#include "utility/fmtlib.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{
TEST(String, ToLower)
{
    std::string str1 = "AbCDe";
    String::ToLower(str1);
    ASSERT_EQ("abcde", str1);

    std::string str2 = "HELLO WORLD!?123";
    String::ToLower(str2);
    ASSERT_EQ("hello world!?123", str2);
}

TEST(String, ToLowerCopy)
{
    const std::string str1 = "AbCDe";
    EXPECT_EQ("abcde", String::ToLowerCopy(str1));

    const std::string str2 = "HELLO WORLD!?123";
    EXPECT_EQ("hello world!?123", String::ToLowerCopy(str2));
}

TEST(String, Concat)
{
    const std::string str1 = "aAa";
    const std::string str2 = "bBb";
    EXPECT_EQ("aAabBb", String::Concat(str1, str2));
}

TEST(String, RightTrim)
{
    std::string str1 = "  AbCDe   \t";
    EXPECT_EQ("  AbCDe", String::RightTrim(str1, " \t"));

    std::string str2 = "23HELLO WORLD!?123        23";
    EXPECT_EQ("23HELLO WORLD!?123        ", String::RightTrim(str2, "23"));
}

TEST(String, LeftTrim)
{
    std::string str1 = "   \tAbCDe  ";
    EXPECT_EQ("AbCDe  ", String::LeftTrim(str1, " \t"));

    std::string str2 = "23        HELLO WORLD!?1  23";
    EXPECT_EQ("        HELLO WORLD!?1  23", String::LeftTrim(str2, "23"));
}

TEST(String, Trim)
{
    std::string str1 = "   \tAbCDe   \t";
    EXPECT_EQ("AbCDe", String::Trim(str1, " \t"));

    std::string str2 = "23        HELLO WORLD!?1        23";
    EXPECT_EQ("HELLO WORLD!?1", String::Trim(str2, " 23"));
}

TEST(String, StartsWith)
{
    EXPECT_TRUE(String::StartsWith("Abcdef", "Abc"));
    EXPECT_TRUE(String::StartsWith("Abc", "Abc"));
    EXPECT_FALSE(String::StartsWith("Ab", "Abc"));
    EXPECT_FALSE(String::StartsWith("", "Abc"));
    EXPECT_TRUE(String::StartsWith("", ""));
    EXPECT_FALSE(String::StartsWith("aAbcdef", "Abc"));
}

TEST(String, EndsWith)
{
    EXPECT_TRUE(String::EndsWith("defAbc", "Abc"));
    EXPECT_TRUE(String::EndsWith("Abc", "Abc"));
    EXPECT_FALSE(String::EndsWith("bc", "Abc"));
    EXPECT_FALSE(String::EndsWith("", "Abc"));
    EXPECT_TRUE(String::EndsWith("", ""));
    EXPECT_FALSE(String::EndsWith("defAbca", "Abc"));
}

// Define a simple struct for the custom type test
struct TestPoint
{
    int x, y;

    void FormatTo(fmt::format_context& ctx) const
    {
        StructFormattingHelper h(ctx);
        h.Field("x", x);
        h.Field("y", y);
    }
};

// Test suite name can be String_JoinBy or similar to reflect the class and method
TEST(String_JoinBy, EmptyVector)
{
    std::vector<std::string> empty_vec_str;
    EXPECT_EQ(String::JoinBy(empty_vec_str, ","), "");

    std::vector<int> empty_vec_int;
    EXPECT_EQ(String::JoinBy(empty_vec_int, ","), "");
}

TEST(String_JoinBy, SingleElementVector)
{
    std::vector<std::string> single_str_vec = {"hello"};
    EXPECT_EQ(String::JoinBy(single_str_vec, ","), "hello");

    std::vector<int> single_int_vec = {123};
    EXPECT_EQ(String::JoinBy(single_int_vec, ","), "123");

    std::vector<double> single_double_vec = {3.14};
    EXPECT_EQ(String::JoinBy(single_double_vec, ","), "3.14");
}

TEST(String_JoinBy, MultipleStringElements)
{
    std::vector<std::string> str_vec = {"apple", "banana", "cherry"};
    EXPECT_EQ(String::JoinBy(str_vec, ","), "apple,banana,cherry");
    EXPECT_EQ(String::JoinBy(str_vec, "; "), "apple; banana; cherry");
    EXPECT_EQ(String::JoinBy(str_vec, "-"), "apple-banana-cherry");
    EXPECT_EQ(String::JoinBy(str_vec, ""), "applebananacherry");  // Empty separator
}

TEST(String_JoinBy, MultipleIntegerElements)
{
    std::vector<int> int_vec = {1, 2, 3, 45, 6};
    EXPECT_EQ(String::JoinBy(int_vec, ","), "1,2,3,45,6");
    EXPECT_EQ(String::JoinBy(int_vec, " - "), "1 - 2 - 3 - 45 - 6");
    EXPECT_EQ(String::JoinBy(int_vec, ""), "123456");
}

TEST(String_JoinBy, MultipleDoubleElements)
{
    std::vector<double> double_vec = {1.1, 2.25, 3.5};
    EXPECT_EQ(String::JoinBy(double_vec, ", "), "1.1, 2.25, 3.5");
    EXPECT_EQ(String::JoinBy(double_vec, "|"), "1.1|2.25|3.5");
}

TEST(String_JoinBy, CustomTypeElements)
{
    std::vector<TestPoint> points_vec = {{1, 2}, {3, 4}, {5, 6}};
    // This test relies on the fmt::formatter<TestPoint> specialization.
    EXPECT_EQ(String::JoinBy(points_vec, "; "), "x = 1, y = 2; x = 3, y = 4; x = 5, y = 6");
}

TEST(String_JoinBy, SeparatorWithSpecialCharacters)
{
    std::vector<std::string> str_vec = {"one", "two"};
    EXPECT_EQ(String::JoinBy(str_vec, "<SEP>"), "one<SEP>two");
}

TEST(String_JoinBy, VectorWithEmptyStrings)
{
    std::vector<std::string> str_vec = {"", "middle", ""};
    EXPECT_EQ(String::JoinBy(str_vec, ","), ",middle,");

    std::vector<std::string> str_vec_all_empty = {"", "", ""};
    EXPECT_EQ(String::JoinBy(str_vec_all_empty, ","), ",,");

    std::vector<std::string> str_vec_single_empty = {""};
    EXPECT_EQ(String::JoinBy(str_vec_single_empty, ","), "");
}

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(TestPoint, FormatTo);
