#include <functional>

#include "base_test_fixtures.h"
#include "components/combat_unit_component.h"

namespace simulation
{

template <auto value>
struct TypedLiteral
{
    static constexpr auto GetValue()
    {
        return value;
    }
};

class EntitySortingTestBase : public BaseTest
{
public:
    bool SortEntitiesByUniqueID() const override
    {
        return true;
    }

    static HexGridPosition MakeFreePosition(size_t index, const Team team)
    {
        int q = 3 * static_cast<int>(index) - 25, r = -25;

        if (team == Team::kRed)
        {
            r = -r;
        }

        return HexGridPosition{q, r};
    }

    FullCombatUnitData MakeGenericIlluvialData(const size_t index, const Team team)
    {
        FullCombatUnitData full_data{};
        CombatUnitData& data = full_data.data;
        data = CreateCombatUnitData();
        data.type_id = CombatUnitTypeID("SomeLine", 1);

        CombatUnitInstanceData& instance = full_data.instance;
        instance = CreateCombatUnitInstanceData();
        instance.id = fmt::format("{}_{}", team, index);
        instance.team = team;
        instance.position = MakeFreePosition(index, team);

        return full_data;
    }

    // Spawns generic illuvial. Uses index and team to make unique position
    std::vector<Entity*> SpawnIlluvials(size_t count, const Team team)
    {
        std::vector<Entity*> entities;
        for (size_t index = 0; index != count; ++index)
        {
            const FullCombatUnitData full_data = MakeGenericIlluvialData(index, team);
            Entity* entity = EntityFactory::SpawnCombatUnit(*world, full_data, kInvalidEntityID);
            entities.push_back(entity);
        }
        return entities;
    }
};

template <typename TLiteral>
class EntitySortingTest : public EntitySortingTestBase
{
public:
    uint64_t GetWorldRandomSeed() const override
    {
        return static_cast<uint64_t>(TLiteral::GetValue());
    }
};

using EntitySortingTestImplementations = testing::Types<
    TypedLiteral<0>,
    TypedLiteral<1>,
    TypedLiteral<2>,
    TypedLiteral<3>,
    TypedLiteral<4>,
    TypedLiteral<5>,
    TypedLiteral<6>,
    TypedLiteral<7>>;

struct EntitySortingTestsNames
{
    template <typename TLiteral>
    static std::string GetName(int)
    {
        constexpr auto value = TLiteral::GetValue();
        constexpr bool ascending = Math::IsEven(value);
        return fmt::format("{}_{}", ascending ? "ascending" : "descending", value);
    }
};

TYPED_TEST_SUITE(EntitySortingTest, EntitySortingTestImplementations, EntitySortingTestsNames);

TYPED_TEST(EntitySortingTest, CheckOrder)
{
    constexpr size_t entities_per_team = 5;
    std::vector<Entity*> red_entities = this->SpawnIlluvials(entities_per_team, Team::kRed);
    std::vector<Entity*> blue_entities = this->SpawnIlluvials(entities_per_team, Team::kBlue);

    this->world->TimeStep();

    std::vector<std::string> ids;
    for (const std::shared_ptr<Entity>& entity : this->world->GetAll())
    {
        if (entity->Has<CombatUnitComponent>())
        {
            std::string unique_id = entity->Get<CombatUnitComponent>().GetUniqueID();
            ids.emplace_back(std::move(unique_id));
        }
    }

    ASSERT_EQ(ids.size(), entities_per_team * 2);
    if (Math::IsEven(this->GetWorldRandomSeed()))
    {
        ASSERT_TRUE(std::is_sorted(ids.begin(), ids.end(), std::less<std::string>{}));
    }
    else
    {
        ASSERT_TRUE(std::is_sorted(ids.begin(), ids.end(), std::greater<std::string>{}));
    }
}

}  // namespace simulation
