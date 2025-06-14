#pragma once

#include "ecs/system.h"

namespace simulation
{

namespace event_data
{
struct AbilityDeactivated;
struct Focus;
struct Skill;
}  // namespace event_data

/* -------------------------------------------------------------------------------------------------------
 * ChainSystem
 *
 * Handles the active chains inside the world
 * --------------------------------------------------------------------------------------------------------
 */
class ChainSystem : public System
{
    typedef System Super;
    typedef ChainSystem Self;

    // Internal struct
    struct ChainFindResult
    {
        // Can either be the sender_id or parent_id
        std::shared_ptr<Entity> chain_entity;

        EntityID sender_id = kInvalidEntityID;
        EntityID parent_id = kInvalidEntityID;

        bool is_sender_a_chain = false;
        bool is_parent_a_chain = false;
    };

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSpawnable;
    }

private:
    // Listen to world events
    void OnAbilityDeactivated(const event_data::AbilityDeactivated& data);
    void OnSkillNoTargets(const event_data::Skill& data);
    void OnFocusNeverDeactivated(const event_data::Focus& data);

    // Helper method to bounce the chain
    void TryToBounceChain(const ChainFindResult& find_result);

    // Helper method to get the chain entity from a sender_id
    void GetChainEntity(const std::string_view method_context, const EntityID sender_id, ChainFindResult* out_result)
        const;
};

}  // namespace simulation
