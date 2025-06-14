#pragma once

#include "data/chain_data.h"
#include "data/constants.h"
#include "ecs/component.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * ChainComponent
 *
 * This class holds the details of a chain spawned in the world
 * --------------------------------------------------------------------------------------------------------
 */
class ChainComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<ChainComponent>(*this);
    }

    // Sets the data from ChainData
    void SetComponentData(const ChainData& data)
    {
        data_ = data;
    }
    const ChainData& GetComponentData() const
    {
        return data_;
    }

    // From which combat unit is this chain originally spawned
    EntityID GetCombatUnitSenderID() const
    {
        return data_.combat_unit_sender_id;
    }

    // From which entity is this chain spawned
    EntityID GetSenderID() const
    {
        return data_.sender_id;
    }

    int GetChainDelayMs() const
    {
        return data_.chain_delay_ms;
    }
    int GetChainNumber() const
    {
        return data_.chain_number;
    }

    // Get/set the chain bounce position
    const HexGridPosition& GetBouncePosition() const
    {
        return bounce_position_;
    }
    void SetBouncePosition(const HexGridPosition& bounce_position)
    {
        bounce_position_ = bounce_position;
    }

private:
    // Chain specific data
    ChainData data_{};

    // Position where the chain entity bounces
    HexGridPosition bounce_position_ = kInvalidHexHexGridPosition;
};

}  // namespace simulation
