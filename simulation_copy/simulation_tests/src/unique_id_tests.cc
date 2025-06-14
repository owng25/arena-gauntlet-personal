#include "base_test_fixtures.h"
#include "data/combat_unit_data.h"
#include "data/constants.h"
#include "factories/entity_factory.h"

namespace simulation
{
class UniqueIdTest : public BaseTest
{
public:
    using Super = BaseTest;

    virtual bool DisableLogs() const
    {
        return true;
    }

    void SetUp() override
    {
        Super::SetUp();

        if (DisableLogs())
        {
            world->GetLogger()->Disable();
        }
    }

    static HexGridPosition MakeFreePosition(int index, const Team team)
    {
        int q = 3 * index - 50, r = -53;

        if (team == Team::kRed)
        {
            r = -r;
        }

        return HexGridPosition{q, r};
    }

    FullCombatUnitData MakeGenericIlluvialData(const int index, const Team team, const std::string_view unique_id)
    {
        FullCombatUnitData full_data{};
        CombatUnitData& data = full_data.data;
        data = CreateCombatUnitData();
        data.type_id = CombatUnitTypeID("SomeLine", 1);

        CombatUnitInstanceData& instance = full_data.instance;
        instance = CreateCombatUnitInstanceData();
        instance.id = std::string(unique_id);
        instance.team = team;
        instance.position = MakeFreePosition(index, team);

        return full_data;
    }

    // Spawns generic illuvial. Uses index and team to make unique position
    Entity* SpawnIlluvial(const int index, const Team team, const std::string_view unique_id)
    {
        const FullCombatUnitData full_data = MakeGenericIlluvialData(index, team, unique_id);
        return EntityFactory::SpawnCombatUnit(*world, full_data, kInvalidEntityID);
    }
};

TEST_F(UniqueIdTest, SpawnCombatUnitWithExistingId)
{
    Entity* first = SpawnIlluvial(0, Team::kRed, "UnitA");
    ASSERT_NE(first, nullptr);
    Entity* second = SpawnIlluvial(1, Team::kRed, "UnitB");
    ASSERT_NE(second, nullptr);
    Entity* third = SpawnIlluvial(2, Team::kRed, "UnitA");
    ASSERT_EQ(third, nullptr);
}

TEST_F(UniqueIdTest, UpdateCombatUnitFromData)
{
    Entity* first = SpawnIlluvial(0, Team::kRed, "UnitA");
    ASSERT_NE(first, nullptr);

    Entity* second = SpawnIlluvial(1, Team::kRed, "UnitB");
    ASSERT_NE(second, nullptr);

    // Must me OK because UnitC id is not occupied
    FullCombatUnitData full_data = MakeGenericIlluvialData(1, Team::kRed, "UnitC");
    ASSERT_TRUE(world->UpdateCombatUnitFromData(second->GetID(), full_data, true));

    // Must also be OK because UnitB id must be freed during previous update
    full_data.instance.id = "UnitB";
    ASSERT_TRUE(world->UpdateCombatUnitFromData(second->GetID(), full_data, true));

    // Updating to the same data (including id) is also legal
    full_data.instance.id = "UnitB";
    ASSERT_TRUE(world->UpdateCombatUnitFromData(second->GetID(), full_data, true));

    // This must fail because UnitA id is taken by first entity
    full_data.instance.id = "UnitA";
    ASSERT_FALSE(world->UpdateCombatUnitFromData(second->GetID(), full_data, true));
}
}  // namespace simulation
