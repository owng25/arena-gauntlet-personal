#include <iterator>

#include "data/enums.h"
#include "gtest/gtest.h"
#include "utility/enum.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{
class FormatterTest : public ::testing::Test
{
public:
    using Super = ::testing::Test;

    void SetUp() override
    {
        Super::SetUp();
    }

    fmt::buffer_context<char> MakeContext()
    {
        return fmt::buffer_context<char>(fmt::appender(mem_buffer), fmt::make_format_args("{}"));
    }

    std::string_view GetResultString() const
    {
        return std::string_view(
            mem_buffer.data(),
            static_cast<size_t>(std::distance(mem_buffer.begin(), mem_buffer.end())));
    }

private:
    fmt::memory_buffer mem_buffer;
};

class StructFormattingHelperTest : public FormatterTest
{
};

TEST_F(StructFormattingHelperTest, Write)
{
    auto ctx = MakeContext();
    StructFormattingHelper h(ctx);

    h.Write("456, ");
    h.Write("789, ");

    ASSERT_EQ(GetResultString(), "456, 789, ");
}

TEST_F(StructFormattingHelperTest, CustomField)
{
    auto ctx = MakeContext();
    StructFormattingHelper h(ctx);

    h.CustomField("a = {}", 10);
    h.CustomField("b: {}", 20);

    ASSERT_EQ(GetResultString(), "a = 10, b: 20");
}

TEST_F(StructFormattingHelperTest, CustomFieldIf)
{
    auto ctx = MakeContext();
    StructFormattingHelper h(ctx);

    h.CustomFieldIf(false, "a = {}", 10);
    h.CustomFieldIf(true, "b = {}", 20);
    h.CustomFieldIf(true, "c = {}", 30);

    ASSERT_EQ(GetResultString(), "b = 20, c = 30");
}
TEST_F(StructFormattingHelperTest, Field)
{
    auto ctx = MakeContext();
    StructFormattingHelper h(ctx);

    h.Field("a", 10);
    h.Field("b", 20);

    ASSERT_EQ(GetResultString(), "a = 10, b = 20");
}

TEST_F(StructFormattingHelperTest, FieldIf)
{
    auto ctx = MakeContext();
    StructFormattingHelper h(ctx);

    h.FieldIf(false, "a", 10);
    h.FieldIf(true, "b", 20);
    h.FieldIf(true, "c", 30);

    ASSERT_EQ(GetResultString(), "b = 20, c = 30");
}

TEST_F(StructFormattingHelperTest, EnumFieldIfNotNone)
{
    auto ctx = MakeContext();
    StructFormattingHelper h(ctx);
    h.EnumFieldIfNotNone("ability_type_1", AbilityType::kAttack);
    h.EnumFieldIfNotNone("ability_type_2", AbilityType::kNone);
    h.EnumFieldIfNotNone("ability_type_3", AbilityType::kOmega);
    ASSERT_EQ(GetResultString(), "ability_type_1 = Attack, ability_type_3 = Omega");
}

}  // namespace simulation
