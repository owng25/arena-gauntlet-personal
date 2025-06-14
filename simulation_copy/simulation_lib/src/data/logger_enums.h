#pragma once

#include <array>
#include <string_view>

namespace simulation
{

// Defines all the log category
namespace LogCategory
{
static constexpr std::string_view kDefault = "general";
static constexpr std::string_view kMovement = "movement";
static constexpr std::string_view kAbility = "ability";
static constexpr std::string_view kEffect = "effect";
static constexpr std::string_view kDecision = "decision";
static constexpr std::string_view kAttachedEffect = "attached-effect";
static constexpr std::string_view kDisplacement = "displacement";
static constexpr std::string_view kSpawnable = "spawnable";
static constexpr std::string_view kHyper = "hyper";
static constexpr std::string_view kOverload = "overload";
static constexpr std::string_view kFocus = "focus";
static constexpr std::string_view kSynergy = "synergy";
static constexpr std::string_view kAugment = "augment";
static constexpr std::string_view kConsumable = "consumable";
static constexpr std::string_view kAura = "aura";

// All the categories that are non default
static constexpr std::array<std::string_view, 14> kAllNonDefaultList = {
    kMovement,
    kAbility,
    kEffect,
    kDecision,
    kAttachedEffect,
    kDisplacement,
    kSpawnable,
    kHyper,
    kOverload,
    kFocus,
    kSynergy,
    kAugment,
    kConsumable,
    kAura};

}  // namespace LogCategory

}  // namespace simulation
