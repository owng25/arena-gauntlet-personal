#include "utility/enum.h"

#include <array>

#include "data/enums.h"
#include "utility/enum_mapping.h"

namespace simulation
{
// clang-format off

template<typename T>
using EnumNameValueMap = std::array<const enum_mapping::NameValuePair<T>, static_cast<size_t>(T::kNum)>;

// CombatUnitType
ILLUVIUM_ENSURE_ENUM_SIZE(CombatUnitType, 5);
static constexpr EnumNameValueMap<CombatUnitType> kCombatUnitTypes
{{
    {CombatUnitType::kNone, ""},
    {CombatUnitType::kIlluvial, "Illuvial"},
    {CombatUnitType::kRanger, "Ranger"},
    {CombatUnitType::kPet, "Pet"},
    {CombatUnitType::kCrate, "Crate"},
}};

// EffectDamageType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectDamageType, 5);
static constexpr EnumNameValueMap<EffectDamageType> kEffectDamageTypes
{{
    {EffectDamageType::kNone, ""},
    {EffectDamageType::kPhysical, "Physical"},
    {EffectDamageType::kEnergy, "Energy"},
    {EffectDamageType::kPure, "Pure"},
    {EffectDamageType::kPurest, "Purest"},
}};

// EffectPositiveState
ILLUVIUM_ENSURE_ENUM_SIZE(EffectPositiveState, 7);
static constexpr EnumNameValueMap<EffectPositiveState> kEffectPositiveStates
{{
    {EffectPositiveState::kNone, ""},
    {EffectPositiveState::kImmune, "Immune"},
    {EffectPositiveState::kTruesight, "Truesight"},
    {EffectPositiveState::kIndomitable, "Indomitable"},
    {EffectPositiveState::kInvulnerable, "Invulnerable"},
    {EffectPositiveState::kUntargetable, "Untargetable"},
    {EffectPositiveState::kEffectPackageBlock, "EffectPackageBlock"}
}};

// EffectNegativeState
ILLUVIUM_ENSURE_ENUM_SIZE(EffectNegativeState, 13);
static constexpr EnumNameValueMap<EffectNegativeState> kEffectNegativeStates
{{
    {EffectNegativeState::kNone, ""},
    {EffectNegativeState::kFocused, "Focused"},
    {EffectNegativeState::kFrozen, "Frozen"},
    {EffectNegativeState::kStun, "Stun"},
    {EffectNegativeState::kDisarm, "Disarm"},
    {EffectNegativeState::kBlind, "Blind"},
    {EffectNegativeState::kRoot, "Root"},
    {EffectNegativeState::kClumsy, "Clumsy"},
    {EffectNegativeState::kSilenced, "Silenced"},
    {EffectNegativeState::kLethargic, "Lethargic"},
    {EffectNegativeState::kTaunted, "Taunted"},
    {EffectNegativeState::kCharm, "Charm"},
    {EffectNegativeState::kFlee, "Flee"},

}};

// EffectPlaneChange
ILLUVIUM_ENSURE_ENUM_SIZE(EffectPlaneChange, 3);
static constexpr EnumNameValueMap<EffectPlaneChange> kEffectPlaneChanges
{{
    {EffectPlaneChange::kNone, ""},
    {EffectPlaneChange::kAirborne, "Airborne"},
    {EffectPlaneChange::kUnderground, "Underground"}
}};

// EffectConditionType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectConditionType, 5);
static constexpr EnumNameValueMap<EffectConditionType> kEffectConditionTypes
{{
    {EffectConditionType::kNone, ""},
    {EffectConditionType::kPoison, "Poison"},
    {EffectConditionType::kWound, "Wound"},
    {EffectConditionType::kBurn, "Burn"},
    {EffectConditionType::kFrost, "Frost"},
}};

// EffectPropagationType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectPropagationType, 3);
static constexpr EnumNameValueMap<EffectPropagationType> kEffectPropagationTypes
{{
    {EffectPropagationType::kNone, ""},
    {EffectPropagationType::kChain, "Chain"},
    {EffectPropagationType::kSplash, "Splash"},
}};

// EffectDisplacementType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectDisplacementType, 6);
static constexpr EnumNameValueMap<EffectDisplacementType> kEffectDisplacementTypes
{{
    {EffectDisplacementType::kNone, ""},
    {EffectDisplacementType::kKnockBack, "KnockBack"},
    {EffectDisplacementType::kKnockUp, "KnockUp"},
    {EffectDisplacementType::kPull, "Pull"},
    {EffectDisplacementType::kThrowToFurthestEnemy, "ThrowToFurthestEnemy"},
    {EffectDisplacementType::kThrowToHighestEnemyDensity, "ThrowToHighestEnemyDensity"}
}};

// EffectOverlapProcessType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectOverlapProcessType, 7);
static constexpr EnumNameValueMap<EffectOverlapProcessType> kEffectOverlapProcessTypes
{{
    {EffectOverlapProcessType::kNone, ""},
    {EffectOverlapProcessType::kSum, "Sum"},
    {EffectOverlapProcessType::kHighest, "Highest"},
    {EffectOverlapProcessType::kStacking, "Stacking"},
    {EffectOverlapProcessType::kShield, "Shield"},
    {EffectOverlapProcessType::kCondition, "Condition"},
    {EffectOverlapProcessType::kMerge, "Merge"},
}};

// MarkEffectType
ILLUVIUM_ENSURE_ENUM_SIZE(MarkEffectType, 3);
static constexpr EnumNameValueMap<MarkEffectType> kMarkEffectTypes
{{
    {MarkEffectType::kNone, ""},
    {MarkEffectType::kBeneficial, "Beneficial"},
    {MarkEffectType::kDetrimental, "Detrimental"}
}};

// CombatAffinity
ILLUVIUM_ENSURE_ENUM_SIZE(CombatAffinity, 21);
static constexpr EnumNameValueMap<CombatAffinity> kCombatAffinities
{{
    {CombatAffinity::kNone, ""},
    {CombatAffinity::kWater, "Water"},
    {CombatAffinity::kEarth, "Earth"},
    {CombatAffinity::kFire, "Fire"},
    {CombatAffinity::kNature, "Nature"},
    {CombatAffinity::kAir, "Air"},
    {CombatAffinity::kTsunami, "Tsunami"},
    {CombatAffinity::kMud, "Mud"},
    {CombatAffinity::kSteam, "Steam"},
    {CombatAffinity::kToxic, "Toxic"},
    {CombatAffinity::kFrost, "Frost"},
    {CombatAffinity::kGranite, "Granite"},
    {CombatAffinity::kMagma, "Magma"},
    {CombatAffinity::kBloom, "Bloom"},
    {CombatAffinity::kDust, "Dust"},
    {CombatAffinity::kInferno, "Inferno"},
    {CombatAffinity::kWildfire, "Wildfire"},
    {CombatAffinity::kShock, "Shock"},
    {CombatAffinity::kVerdant, "Verdant"},
    {CombatAffinity::kSpore, "Spore"},
    {CombatAffinity::kTempest, "Tempest"}
}};

// CombatClass
ILLUVIUM_ENSURE_ENUM_SIZE(CombatClass, 21);
static constexpr EnumNameValueMap<CombatClass> kCombatClasses
{{
    {CombatClass::kNone, ""},
    {CombatClass::kFighter, "Fighter"},
    {CombatClass::kBulwark, "Bulwark"},
    {CombatClass::kRogue, "Rogue"},
    {CombatClass::kPsion, "Psion"},
    {CombatClass::kEmpath, "Empath"},
    {CombatClass::kBerserker, "Berserker"},
    {CombatClass::kBehemoth, "Behemoth"},
    {CombatClass::kColossus, "Colossus"},
    {CombatClass::kSlayer, "Slayer"},
    {CombatClass::kArcanite, "Arcanite"},
    {CombatClass::kVanguard, "Vanguard"},
    {CombatClass::kPredator, "Predator"},
    {CombatClass::kHarbinger, "Harbinger"},
    {CombatClass::kPhantom, "Phantom"},
    {CombatClass::kInvoker, "Invoker"},
    {CombatClass::kTemplar, "Templar"},
    {CombatClass::kAegis, "Aegis"},
    {CombatClass::kRevenant, "Revenant"},
    {CombatClass::kEnchanter, "Enchanter"},
    {CombatClass::kMystic, "Mystic"}
}};

// AbilitySelectionType
ILLUVIUM_ENSURE_ENUM_SIZE(AbilitySelectionType, 5);
static constexpr EnumNameValueMap<AbilitySelectionType> kAbilitySelections
{{
    {AbilitySelectionType::kNone, ""},
    {AbilitySelectionType::kCycle, "Cycle"},
    {AbilitySelectionType::kSelfHealthCheck, "SelfHealthCheck"},
    {AbilitySelectionType::kSelfAttributeCheck, "SelfAttributeCheck"},
    {AbilitySelectionType::kEveryXActivations, "EveryXActivations"},
}};

// AbilityType
ILLUVIUM_ENSURE_ENUM_SIZE(AbilityType, 4);
static constexpr EnumNameValueMap<AbilityType> kAbilityTypes
{{
    {AbilityType::kNone, ""},
    {AbilityType::kAttack, "Attack"},
    {AbilityType::kOmega, "Omega"},
    {AbilityType::kInnate, "Innate"},
}};

// AbilityUpdateType
ILLUVIUM_ENSURE_ENUM_SIZE(AbilityUpdateType, 3);
static constexpr EnumNameValueMap<AbilityUpdateType> kAbilityUpdateTypes
{{
    {AbilityUpdateType::kNone, ""},
    {AbilityUpdateType::kAdd, "Add"},
    {AbilityUpdateType::kReplace, "Replace"},
}};

// Team
ILLUVIUM_ENSURE_ENUM_SIZE(Team, 3);
static constexpr EnumNameValueMap<Team> kTeamsNames
{{
    {Team::kNone, ""},
    {Team::kRed, "Red"},
    {Team::kBlue, "Blue"},
}};

// SourceContextType
ILLUVIUM_ENSURE_ENUM_SIZE(SourceContextType, 16);
static constexpr EnumNameValueMap<SourceContextType> kSourceContextTypes
{{
    {SourceContextType::kNone, ""},
    {SourceContextType::kAttachedEffect, "AttachedEffect"},
    {SourceContextType::kReachedMaxStacks, "ReachedMaxStacks"},
    {SourceContextType::kSynergy, "Synergy"},
    {SourceContextType::kAugment, "Augment"},
    {SourceContextType::kHyper, "Hyper"},
    {SourceContextType::kShield, "Shield"},
    {SourceContextType::kMark, "Mark"},
    {SourceContextType::kOverload, "Overload"},
    {SourceContextType::kSplash, "Splash"},
    {SourceContextType::kDisplacement, "Displacement"},
    {SourceContextType::kConsumable, "Consumable"},
    {SourceContextType::kEncounterMod, "EncounterMod"},
    {SourceContextType::kSuit, "Suit"},
    {SourceContextType::kAura, "Aura"},
    {SourceContextType::kDroneAugment, "DroneAugment"},
}};

// SkillTargetingType
ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
static constexpr EnumNameValueMap<SkillTargetingType> kSkillTargetingTypes
{{
    {SkillTargetingType::kNone, ""},
    {SkillTargetingType::kCurrentFocus, "CurrentFocus"},
    {SkillTargetingType::kSelf, "Self"},
    {SkillTargetingType::kInZone, "InZone"},
    {SkillTargetingType::kDistanceCheck, "DistanceCheck"},
    {SkillTargetingType::kCombatStatCheck, "CombatStatCheck"},
    {SkillTargetingType::kAllegiance, "Allegiance"},
    {SkillTargetingType::kSynergy, "Synergy"},
    {SkillTargetingType::kVanquisher, "Vanquisher"},
    {SkillTargetingType::kPreviousTargetList, "PreviousTargetList"},
    {SkillTargetingType::kActivator, "Activator"},
    {SkillTargetingType::kExpressionCheck, "ExpressionCheck"},
    {SkillTargetingType::kDensity, "Density"},
    {SkillTargetingType::kPets, "Pets"},
    {SkillTargetingType::kTier, "Tier"},
}};

// AllegianceType
ILLUVIUM_ENSURE_ENUM_SIZE(AllegianceType, 5);
static constexpr EnumNameValueMap<AllegianceType> kAllegianceType
{{
    {AllegianceType::kNone, ""},
    {AllegianceType::kSelf, "Self"},
    {AllegianceType::kAlly, "Ally"},
    {AllegianceType::kAll, "All"},
    {AllegianceType::kEnemy, "Enemy"},
}};

// SkillDeploymentType
ILLUVIUM_ENSURE_ENUM_SIZE(SkillDeploymentType, 7);
static constexpr EnumNameValueMap<SkillDeploymentType> kSkillDeploymentTypes
{{
    {SkillDeploymentType::kNone, ""},
    {SkillDeploymentType::kDirect, "Direct"},
    {SkillDeploymentType::kZone, "Zone"},
    {SkillDeploymentType::kProjectile, "Projectile"},
    {SkillDeploymentType::kBeam, "Beam"},
    {SkillDeploymentType::kDash, "Dash"},
    {SkillDeploymentType::kSpawnedCombatUnit, "SpawnedCombatUnit"},
}};

// GuidanceType
ILLUVIUM_ENSURE_ENUM_SIZE(GuidanceType, 4);
static constexpr EnumNameValueMap<GuidanceType> kGuidanceTypes
{{
    {GuidanceType::kNone, ""},
    {GuidanceType::kGround, "Ground"},
    {GuidanceType::kAirborne, "Airborne"},
    {GuidanceType::kUnderground, "Underground"}
}};

// HexGridCardinalDirection
ILLUVIUM_ENSURE_ENUM_SIZE(HexGridCardinalDirection, 7);
static constexpr EnumNameValueMap<HexGridCardinalDirection> kHexGridCardinalDirections
{{
    {HexGridCardinalDirection::kNone, ""},
    {HexGridCardinalDirection::kRight, "Right"},
    {HexGridCardinalDirection::kTopRight, "TopRight"},
    {HexGridCardinalDirection::kTopLeft, "TopLeft"},
    {HexGridCardinalDirection::kLeft, "Left"},
    {HexGridCardinalDirection::kBottomLeft, "BottomLeft"},
    {HexGridCardinalDirection::kBottomRight, "BottomRight"},
}};

// ActivationTriggerType
ILLUVIUM_ENSURE_ENUM_SIZE(ActivationTriggerType, 19);
static constexpr EnumNameValueMap<ActivationTriggerType> kActivationTriggerTypes
{{
    {ActivationTriggerType::kNone, ""},
    {ActivationTriggerType::kOnBattleStart, "OnBattleStart"},
    {ActivationTriggerType::kEveryXTime, "EveryXTime"},
    {ActivationTriggerType::kOnDodge, "OnDodge"},
    {ActivationTriggerType::kOnDealCrit, "OnDealCrit"},
    {ActivationTriggerType::kOnHit, "OnHit"},
    {ActivationTriggerType::kOnShieldHit, "OnShieldHit"},
    {ActivationTriggerType::kOnMiss, "OnMiss"},
    {ActivationTriggerType::kOnDamage, "OnDamage"},
    {ActivationTriggerType::kOnVanquish, "OnVanquish"},
    {ActivationTriggerType::kOnFaint, "OnFaint"},
    {ActivationTriggerType::kOnAssist, "OnAssist"},
    {ActivationTriggerType::kHealthInRange, "HealthInRange"},
    {ActivationTriggerType::kOnActivateXAbilities, "OnActivateXAbilities"},
    {ActivationTriggerType::kOnDeployXSkills, "OnDeployXSkills"},
    {ActivationTriggerType::kInRange, "InRange"},
    {ActivationTriggerType::kOnEnergyFull, "OnEnergyFull"},
    {ActivationTriggerType::kOnReceiveXEffectPackages, "OnReceiveXEffectPackages"},
    {ActivationTriggerType::kOnHyperactive, "OnHyperactive"},
}};

// AbilityTriggerContextRequirement
ILLUVIUM_ENSURE_ENUM_SIZE(AbilityTriggerContextRequirement, 2);
static constexpr EnumNameValueMap<AbilityTriggerContextRequirement> kAbilityTriggerContextRequirements
{{
    {AbilityTriggerContextRequirement::kNone, ""},
    {AbilityTriggerContextRequirement::kDuringOmega, "DuringOmega"}
}};

// AbilityTriggerContextRequirement
ILLUVIUM_ENSURE_ENUM_SIZE(PredefinedGridPosition, 3);
static constexpr EnumNameValueMap<PredefinedGridPosition> kPredefinedGridPositions
{{
    {PredefinedGridPosition::kNone, ""},
    {PredefinedGridPosition::kAllyBorderCenter, "AllyBorderCenter"},
    {PredefinedGridPosition::kEnemyBorderCenter, "EnemyBorderCenter"},
}};

// EventType
ILLUVIUM_ENSURE_ENUM_SIZE(EventType, 84);
static constexpr EnumNameValueMap<EventType> kEventTypes
{{
   {EventType::kNone, ""},
    {EventType::kCreated, "Created"},
    {EventType::kMoved, "Moved"},
    {EventType::kMovedStep, "MovedStep"},
    {EventType::kBlinked, "Blinked"},
    {EventType::kStoppedMovement, "StoppedMovement"},
    {EventType::kFindPath, "FindPath"},
    {EventType::kDeactivated, "Deactivated"},
    {EventType::kFainted, "Fainted"},
    {EventType::kAbilityActivated,   "AbilityActivated"},
    {EventType::kAbilityDeactivated, "AbilityDeactivated"},
    {EventType::kAfterAbilityDeactivated, "AfterAbilityDeactivated"},
    {EventType::kAbilityInterrupted, "AbilityInterrupted"},
    {EventType::kSkillWaiting, "SkillWaiting"},
    {EventType::kSkillDeploying, "SkillDeploying"},
    {EventType::kSkillChanneling, "SkillChanneling"},
    {EventType::kSkillFinished, "SkillFinished"},
    {EventType::kSkillNoTargets, "SkillNoTargets"},
    {EventType::kSkillTargetsUpdated, "SkillTargetsUpdated"},
    {EventType::kEffectPackageReceived, "EffectPackageReceived"},
    {EventType::kEffectPackageMissed, "EffectPackageMissed"},
    {EventType::kEffectPackageDodged, "EffectPackageDodged"},
    {EventType::kEffectPackageBlocked, "EffectPackageBlocked"},
    {EventType::kOnDamage, "OnDamage"},
    {EventType::kHealthChanged, "HealthChanged"},
    {EventType::kEnergyChanged, "EnergyChanged"},
    {EventType::kHyperChanged, "HyperChanged"},
    {EventType::kOnHealthGain, "OnHealthGain"},
    {EventType::kApplyHealthGain, "ApplyHealthGain"},
    {EventType::kApplyShieldGain, "ApplyShieldGain"},
    {EventType::kApplyEnergyGain, "ApplyEnergyGain"},
    {EventType::kOnEffectApplied, "OnEffectApplied"},
    {EventType::kTryToApplyEffect, "TryToApplyEffect"},
    {EventType::kOnAttachedEffectApplied, "OnAttachedEffectApplied"},
    {EventType::kOnAttachedEffectRemoved, "OnAttachedEffectRemoved"},
    {EventType::kActivateAnyAbility, "ActivateAnyAbility"},
    {EventType::kCombatUnitCreated, "CombatUnitCreated"},
    {EventType::kChainCreated, "ChainCreated"},
    {EventType::kChainBounced, "ChainBounced"},
    {EventType::kChainDestroyed, "ChainDestroyed"},
    {EventType::kSplashCreated, "SplashCreated"},
    {EventType::kSplashDestroyed, "SplashDestroyed"},
    {EventType::kProjectileCreated, "ProjectileCreated"},
    {EventType::kProjectileBlocked, "ProjectileBlocked"},
    {EventType::kProjectileDestroyed, "ProjectileDestroyed"},
    {EventType::kZoneCreated, "ZoneCreated"},
    {EventType::kZoneDestroyed, "ZoneDestroyed"},
    {EventType::kZoneActivated, "ZoneActivated"},
    {EventType::kBeamCreated, "BeamCreated"},
    {EventType::kBeamDestroyed, "BeamDestroyed"},
    {EventType::kBeamUpdated, "BeamUpdated"},
    {EventType::kBeamActivated, "BeamActivated"},
    {EventType::kDashCreated, "DashCreated"},
    {EventType::kDashDestroyed, "DashDestroyed"},
    {EventType::kShieldCreated, "ShieldCreated"},
    {EventType::kShieldDestroyed, "ShieldDestroyed"},
    {EventType::kShieldWasHit, "ShieldWasHit"},
    {EventType::kShieldExpired, "ShieldExpired"},
    {EventType::kShieldPendingDestroyed, "ShieldPendingDestroyed"},
    {EventType::kDisplacementStarted, "DisplacementStarted"},
    {EventType::kDisplacementStopped, "DisplacementStopped"},
    {EventType::kMarkCreated, "MarkCreated"},
    {EventType::kMarkDestroyed, "MarkDestroyed"},
    {EventType::kMarkChainAsDestroyed, "MarkChainAsDestroyed"},
    {EventType::kMarkSplashAsDestroyed, "MarkSplashAsDestroyed"},
    {EventType::kMarkProjectileAsDestroyed, "MarkProjectileAsDestroyed"},
    {EventType::kMarkZoneAsDestroyed, "MarkZoneAsDestroyed"},
    {EventType::kMarkBeamAsDestroyed, "MarkBeamAsDestroyed"},
    {EventType::kMarkDashAsDestroyed, "MarkDashAsDestroyed"},
    {EventType::kMarkAttachedEntityAsDestroyed, "MarkAttachedEntityAsDestroyed"},
    {EventType::kFocusNeverDeactivated, "FocusNeverDeactivated"},
    {EventType::kFocusFound, "FocusFound"},
    {EventType::kFocusUnreachable, "FocusUnreachable"},
    {EventType::kCombatUnitStateChanged, "CombatUnitStateChanged"},
    {EventType::kBattleStarted, "BattleStarted"},
    {EventType::kBattleFinished, "BattleFinished"},
    {EventType::kGoneHyperactive, "GoneHyperactive"},
    {EventType::kOverloadStarted, "OverloadStarted"},
    {EventType::kTimeStepped, "TimeStepped"},
    {EventType::kPathfindingDebugData, "PathfindingDebugData"},
    {EventType::kZoneCreated, "ZoneCreated"},
    {EventType::kAuraCreated, "AuraCreated"},
    {EventType::kAuraDestroyed, "AuraDestroyed"},
}};

// StatType
ILLUVIUM_ENSURE_ENUM_SIZE(StatType, 42);
static constexpr EnumNameValueMap<StatType> kStatTypes
{{
    {StatType::kNone, ""},
    {StatType::kMoveSpeedSubUnits, "MoveSpeedSubUnits"},
    {StatType::kAttackRangeUnits, "AttackRangeUnits"},
    {StatType::kOmegaRangeUnits, "OmegaRangeUnits"},
    {StatType::kHitChancePercentage, "HitChancePercentage"},
    {StatType::kAttackDodgeChancePercentage, "AttackDodgeChancePercentage"},
    {StatType::kAttackSpeed, "AttackSpeed"},
    {StatType::kAttackDamage, "AttackDamage"},
    {StatType::kAttackPhysicalDamage, "AttackPhysicalDamage"},
    {StatType::kAttackEnergyDamage, "AttackEnergyDamage"},
    {StatType::kAttackPureDamage, "AttackPureDamage"},
    {StatType::kMaxHealth, "MaxHealth"},
    {StatType::kCurrentHealth, "CurrentHealth"},
    {StatType::kEnergyCost, "EnergyCost"},
    {StatType::kStartingEnergy, "StartingEnergy"},
    {StatType::kCurrentEnergy, "CurrentEnergy"},
    {StatType::kEnergyRegeneration, "EnergyRegeneration"},
    {StatType::kHealthRegeneration, "HealthRegeneration"},
    {StatType::kEnergyGainEfficiencyPercentage, "EnergyGainEfficiencyPercentage"},
    {StatType::kHealthGainEfficiencyPercentage, "HealthGainEfficiencyPercentage"},
    {StatType::kShieldGainEfficiencyPercentage, "ShieldGainEfficiencyPercentage"},
    {StatType::kPhysicalVampPercentage, "PhysicalVampPercentage"},
    {StatType::kPureVampPercentage, "PureVampPercentage"},
    {StatType::kEnergyVampPercentage , "EnergyVampPercentage"},
    {StatType::kOnActivationEnergy, "OnActivationEnergy"},
    {StatType::kPhysicalResist, "PhysicalResist"},
    {StatType::kEnergyResist, "EnergyResist"},
    {StatType::kTenacityPercentage, "TenacityPercentage"},
    {StatType::kWillpowerPercentage, "WillpowerPercentage"},
    {StatType::kGrit, "Grit"},
    {StatType::kResolve, "Resolve"},
    {StatType::kVulnerabilityPercentage, "VulnerabilityPercentage"},
    {StatType::kCritChancePercentage, "CritChancePercentage"},
    {StatType::kCritAmplificationPercentage, "CritAmplificationPercentage"},
    {StatType::kOmegaPowerPercentage, "OmegaPowerPercentage"},
    {StatType::kOmegaDamagePercentage, "OmegaDamagePercentage"},
    {StatType::kEnergyPiercingPercentage, "EnergyPiercingPercentage"},
    {StatType::kPhysicalPiercingPercentage, "PhysicalPiercingPercentage"},
    {StatType::kThorns, "Thorns"},
    {StatType::kStartingShield, "StartingShield"},
    {StatType::kCurrentSubHyper, "CurrentSubHyper"},
    {StatType::kCritReductionPercentage, "CritReductionPercentage"},
}};

// StatEvaluationType
ILLUVIUM_ENSURE_ENUM_SIZE(StatEvaluationType, 4);
static constexpr EnumNameValueMap<StatEvaluationType> kStatEvaluationTypes
{{
    {StatEvaluationType::kNone, ""},
    {StatEvaluationType::kLive, "Live"},
    {StatEvaluationType::kBase, "Base"},
    {StatEvaluationType::kBonus, "Bonus"},
}};

// EffectType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectType, 30);
static constexpr EnumNameValueMap<EffectType> kEffectTypes
{{
    {EffectType::kNone, ""},
    {EffectType::kInstantDamage, "InstantDamage"},
    {EffectType::kDamageOverTime, "DOT"},
    {EffectType::kCondition, "Condition"},
    {EffectType::kHealOverTime, "HOT"},
    {EffectType::kInstantHyperBurn, "InstantHyperBurn"},
    {EffectType::kInstantHyperGain, "InstantHyperGain"},
    {EffectType::kHyperGainOverTime, "HyperGainOverTime"},
    {EffectType::kHyperBurnOverTime, "HyperBurnOverTime"},
    {EffectType::kEnergyGainOverTime, "EnergyGainOverTime"},
    {EffectType::kSpawnShield, "SpawnShield"},
    {EffectType::kSpawnMark, "SpawnMark"},
    {EffectType::kDisplacement, "Displacement"},
    {EffectType::kEmpower, "Empower"},
    {EffectType::kDisempower, "Disempower"},
    {EffectType::kPositiveState, "PositiveState"},
    {EffectType::kNegativeState, "NegativeState"},
    {EffectType::kDebuff, "Debuff"},
    {EffectType::kBuff, "Buff"},
    {EffectType::kAura, "Aura"},
    {EffectType::kInstantHeal, "InstantHeal"},
    {EffectType::kCleanse, "Cleanse"},
    {EffectType::kInstantEnergyBurn, "InstantEnergyBurn"},
    {EffectType::kInstantEnergyGain, "InstantEnergyGain"},
    {EffectType::kEnergyBurnOverTime, "EnergyBurnOverTime"},
    {EffectType::kExecute, "Execute"},
    {EffectType::kBlink, "Blink"},
    {EffectType::kPlaneChange, "PlaneChange"},
    {EffectType::kStatOverride, "StatOverride"},
    {EffectType::kInstantShieldBurn, "InstantShieldBurn"}
}};

// EnergyGainType
ILLUVIUM_ENSURE_ENUM_SIZE(EnergyGainType, 6);
static constexpr EnumNameValueMap<EnergyGainType> kEnergyGainType
{{
    {EnergyGainType::kNone, ""},
    {EnergyGainType::kAttack, "Attack"},
    {EnergyGainType::kOnActivation, "OnActivation"},
    {EnergyGainType::kRefund, "Refund"},
    {EnergyGainType::kOnTakeDamage, "OnTakeDamage"},
    {EnergyGainType::kRegeneration, "Regeneration"},
}};

// HealthGainType
ILLUVIUM_ENSURE_ENUM_SIZE(HealthGainType, 7);
static constexpr EnumNameValueMap<HealthGainType> kHealthGainType
{{
    {HealthGainType::kNone, ""},
    {HealthGainType::kPureVamp, "PureVamp"},
    {HealthGainType::kEnergyVamp, "EnergyVamp"},
    {HealthGainType::kPhysicalVamp, "PhysicalVamp"},
    {HealthGainType::kRegeneration, "Regeneration"},
    {HealthGainType::kInstantHeal, "InstantHeal"},
    {HealthGainType::kInstantPureHeal, "InstantPureHeal"},
}};

// EffectHealthType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectHealType, 3);
static constexpr EnumNameValueMap<EffectHealType> kEffectHealType
{{
    {EffectHealType::kNone, ""},
    {EffectHealType::kNormal, "Normal"},
    {EffectHealType::kPure, "Pure"},
}};

// ConditionType
ILLUVIUM_ENSURE_ENUM_SIZE(ConditionType, 6);
static constexpr EnumNameValueMap<ConditionType> kConditionTypes
{{
    {ConditionType::kNone, ""},
    {ConditionType::kPoisoned, "Poisoned"},
    {ConditionType::kBurned, "Burned"},
    {ConditionType::kWounded, "Wounded"},
    {ConditionType::kFrosted, "Frosted"},
    {ConditionType::kShielded, "Shielded"},
}};

// EffectValueType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectValueType, 8);
static constexpr EnumNameValueMap<EffectValueType> kEffectValueTypes
{{
    {EffectValueType::kValue, "Value"},
    {EffectValueType::kStat, "Stat"},
    {EffectValueType::kStatPercentage, "StatPercentage"},
    {EffectValueType::kStatHighPrecisionPercentage, "StatHighPrecisionPercentage"},
    {EffectValueType::kSynergyCount, "SynergyCount"},
    {EffectValueType::kMeteredStatPercentage, "MeteredStatPercentage"},
    {EffectValueType::kHyperEffectiveness, "HyperEffectiveness"},
    {EffectValueType::kCustomFunction, "CustomFunction"},
}};

// EffectOperationType
ILLUVIUM_ENSURE_ENUM_SIZE(EffectOperationType, 10);
static constexpr EnumNameValueMap<EffectOperationType> kEffectOperationTypes
{{
    {EffectOperationType::kNone, ""},
    {EffectOperationType::kAdd, "+"},
    {EffectOperationType::kSubtract, "-"},
    {EffectOperationType::kMultiply, "*"},
    {EffectOperationType::kDivide, "/"},
    {EffectOperationType::kIntegralDivision, "//"},
    {EffectOperationType::kPercentageOf, "%"},
    {EffectOperationType::kHighPrecisionPercentageOf, "%%"},
    {EffectOperationType::kMin, "Min"},
    {EffectOperationType::kMax, "Max"},
}};

// ExpressionDataSourceType
ILLUVIUM_ENSURE_ENUM_SIZE(ExpressionDataSourceType, 4);
static constexpr EnumNameValueMap<ExpressionDataSourceType> kExpressionDataSourceTypes
{{
    {ExpressionDataSourceType::kNone, ""},
    {ExpressionDataSourceType::kSender, "Sender"},
    {ExpressionDataSourceType::kReceiver, "Receiver"},
    {ExpressionDataSourceType::kSenderFocus, "SenderFocus"},
}};

// EffectComparisonType
ILLUVIUM_ENSURE_ENUM_SIZE(ComparisonType, 4);
static constexpr EnumNameValueMap<ComparisonType> kComparisonTypes
{{
    {ComparisonType::kNone, ""},
    {ComparisonType::kGreater, ">"},
    {ComparisonType::kLess, "<"},
    {ComparisonType::kEqual, "=="},
}};

// ZoneEffectShape
ILLUVIUM_ENSURE_ENUM_SIZE(ZoneEffectShape, 4);
static constexpr EnumNameValueMap<ZoneEffectShape> kZoneEffectShapes
{{
    {ZoneEffectShape::kNone, ""},
    {ZoneEffectShape::kHexagon, "Hexagon"},
    {ZoneEffectShape::kRectangle, "Rectangle"},
    {ZoneEffectShape::kTriangle, "Triangle"},
}};

// DecisionNextAction
ILLUVIUM_ENSURE_ENUM_SIZE(DecisionNextAction, 5);
static constexpr EnumNameValueMap<DecisionNextAction> kDecisionNextActions
{{
    {DecisionNextAction::kNone, "kNone"},
    {DecisionNextAction::kFindFocus, "kFindFocus"},
    {DecisionNextAction::kMovement, "kMovement"},
    {DecisionNextAction::kAttackAbility, "kAttackAbility"},
    {DecisionNextAction::kOmegaAbility, "kOmegaAbility"}
}};

// LogLevel
ILLUVIUM_ENSURE_ENUM_SIZE(LogLevel, 6);
static constexpr EnumNameValueMap<LogLevel> kLogLevels
{{
    {LogLevel::kTrace, "Trace"},
    {LogLevel::kDebug, "Debug"},
    {LogLevel::kInfo, "Info"},
    {LogLevel::kWarn, "Warn"},
    {LogLevel::kErr, "Err"},
    {LogLevel::kCritical, "Critical"},
}};

// AttachedEffectStateType
ILLUVIUM_ENSURE_ENUM_SIZE(AttachedEffectStateType, 4);
static constexpr EnumNameValueMap<AttachedEffectStateType> kAttachedEffectStateTypes
{{
    {AttachedEffectStateType::kPendingActivation, "PendingActivation"},
    {AttachedEffectStateType::kActive, "Active"},
    {AttachedEffectStateType::kWaiting, "Waiting"},
    {AttachedEffectStateType::kDestroyed, "Destroyed"},
}};

// CombatUnitState
ILLUVIUM_ENSURE_ENUM_SIZE(CombatUnitState, 4);
static constexpr EnumNameValueMap<CombatUnitState> kCombatUnitStates
{{
    {CombatUnitState::kNone, ""},
    {CombatUnitState::kAlive, "Alive"},
    {CombatUnitState::kFainted, "Fainted"},
    {CombatUnitState::kDisappeared, "Disappeared"},
}};

// ReservedPositionType
ILLUVIUM_ENSURE_ENUM_SIZE(ReservedPositionType, 4);
static constexpr EnumNameValueMap<ReservedPositionType> kReservedPositionTypes
{{
    {ReservedPositionType::kNone, ""},
    {ReservedPositionType::kAcross, "Across"},
    {ReservedPositionType::kBehindReceiver, "BehindReceiver"},
    {ReservedPositionType::kNearReceiver, "NearReceiver"},
}};

// AttachedEntityTypes
ILLUVIUM_ENSURE_ENUM_SIZE(AttachedEntityType, 3);
static constexpr EnumNameValueMap<AttachedEntityType> kAttachedEntityTypes
{{
    {AttachedEntityType::kNone, ""},
    {AttachedEntityType::kShield, "Shield"},
    {AttachedEntityType::kMark, "Mark"},
}};

// ValidationTypes
ILLUVIUM_ENSURE_ENUM_SIZE(ValidationType, 3);
static constexpr EnumNameValueMap<ValidationType> kValidationTypes
{{
    {ValidationType::kNone, ""},
    {ValidationType::kDistanceCheck, "DistanceCheck"},
    {ValidationType::kExpression, "Expression"},
}};

// ValidationTypes
ILLUVIUM_ENSURE_ENUM_SIZE(ValidationStartEntity, 3);
static constexpr EnumNameValueMap<ValidationStartEntity> kValidationStartEntities
{{
    {ValidationStartEntity::kNone, ""},
    {ValidationStartEntity::kSelf, "Self"},
    {ValidationStartEntity::kCurrentFocus, "CurrentFocus"},
}};

// Augment types
ILLUVIUM_ENSURE_ENUM_SIZE(AugmentType, 5);
static constexpr EnumNameValueMap<AugmentType> kAugmentTypes
{{
    {AugmentType::kNone, ""},
    {AugmentType::kNormal, "Normal"},
    {AugmentType::kComponent, "Component"},
    {AugmentType::kGreaterPower, "GreaterPower"},
    {AugmentType::kSynergyBonus, "SynergyBonus"},
}};

// Suit types
ILLUVIUM_ENSURE_ENUM_SIZE(SuitType, 3);
static constexpr EnumNameValueMap<SuitType> kSuitTypes
{{
    {SuitType::kNone, ""},
    {SuitType::kNormal, "Normal"},
    {SuitType::kAmplifier, "Amplifier"},
}};

// Suit types
ILLUVIUM_ENSURE_ENUM_SIZE(SuitType, 3);
static constexpr EnumNameValueMap<WeaponType> kWeaponTypes
{{
    {WeaponType::kNone, ""},
    {WeaponType::kNormal, "Normal"},
    {WeaponType::kAmplifier, "Amplifier"},
}};


// Drone Augment types
ILLUVIUM_ENSURE_ENUM_SIZE(DroneAugmentType, 4);
static constexpr EnumNameValueMap<DroneAugmentType> kDroneAugmentTypes
{{
    {DroneAugmentType::kNone, ""},
    {DroneAugmentType::kSimulation, "Simulation"},
    {DroneAugmentType::kGame, "Game"},
    {DroneAugmentType::kServerAndGame, "ServerAndGame"}
}};

// Death reason type
ILLUVIUM_ENSURE_ENUM_SIZE(DeathReasonType, 4);
static constexpr EnumNameValueMap<DeathReasonType> kDeathReasonTypes
{{
    {DeathReasonType::kNone, ""},
    {DeathReasonType::kDamage, "Damage"},
    {DeathReasonType::kExecution, "Execution"},
    {DeathReasonType::kOverload, "Overload"}
}};

// clang-format on

std::string_view Enum::CombatUnitTypeToString(const CombatUnitType type)
{
    return enum_mapping::GetNameForValue(kCombatUnitTypes, type);
}

CombatUnitType Enum::StringToCombatUnitType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kCombatUnitTypes, name);
}

std::string_view Enum::EffectDamageTypeToString(const EffectDamageType damage_type)
{
    return enum_mapping::GetNameForValue(kEffectDamageTypes, damage_type);
}

EffectDamageType Enum::StringToEffectDamageType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectDamageTypes, name);
}

std::string_view Enum::EffectPositiveStateToString(const EffectPositiveState positive_state)
{
    return enum_mapping::GetNameForValue(kEffectPositiveStates, positive_state);
}

EffectPositiveState Enum::StringToEffectPositiveState(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectPositiveStates, name);
}

std::string_view Enum::EffectNegativeStateToString(const EffectNegativeState negative_state)
{
    return enum_mapping::GetNameForValue(kEffectNegativeStates, negative_state);
}

EffectNegativeState Enum::StringToEffectNegativeState(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectNegativeStates, name);
}

std::string_view Enum::EffectPlaneChangeToString(const EffectPlaneChange plane_change)
{
    return enum_mapping::GetNameForValue(kEffectPlaneChanges, plane_change);
}

EffectPlaneChange Enum::StringToEffectPlaneChange(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectPlaneChanges, name);
}

std::string_view Enum::EffectConditionTypeToString(const EffectConditionType condition_type)
{
    return enum_mapping::GetNameForValue(kEffectConditionTypes, condition_type);
}

EffectConditionType Enum::StringToEffectConditionType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectConditionTypes, name);
}

std::string_view Enum::EffectPropagationTypeToString(const EffectPropagationType propagation_type)
{
    return enum_mapping::GetNameForValue(kEffectPropagationTypes, propagation_type);
}

EffectPropagationType Enum::StringToEffectPropagationType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectPropagationTypes, name);
}

std::string_view Enum::EffectDisplacementTypeToString(const EffectDisplacementType displacement_type)
{
    return enum_mapping::GetNameForValue(kEffectDisplacementTypes, displacement_type);
}

EffectDisplacementType Enum::StringToEffectDisplacementType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectDisplacementTypes, name);
}

std::string_view Enum::EffectOverlapProcessTypeToString(const EffectOverlapProcessType overlap_type)
{
    return enum_mapping::GetNameForValue(kEffectOverlapProcessTypes, overlap_type);
}

EffectOverlapProcessType Enum::StringToEffectOverlapProcessType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectOverlapProcessTypes, name);
}

std::string_view Enum::MarkEffectTypeToString(const MarkEffectType mark_effect_type)
{
    return enum_mapping::GetNameForValue(kMarkEffectTypes, mark_effect_type);
}

MarkEffectType Enum::StringToMarkEffectType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kMarkEffectTypes, name);
}

std::string_view Enum::EffectTypeToString(const EffectType effect_type)
{
    return enum_mapping::GetNameForValue(kEffectTypes, effect_type);
}

EffectType Enum::StringToEffectType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectTypes, name);
}

std::string_view Enum::SourceContextTypeToString(const SourceContextType context_type)
{
    return enum_mapping::GetNameForValue(kSourceContextTypes, context_type);
}

SourceContextType Enum::StringToSourceContextType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kSourceContextTypes, name);
}

std::string_view Enum::EnergyGainTypeToString(const EnergyGainType energy_gain_type)
{
    return enum_mapping::GetNameForValue(kEnergyGainType, energy_gain_type);
}

EnergyGainType Enum::StringToEnergyGainType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEnergyGainType, name);
}

std::string_view Enum::HealthGainTypeToString(const HealthGainType health_gain_type)
{
    return enum_mapping::GetNameForValue(kHealthGainType, health_gain_type);
}

HealthGainType Enum::StringToHealthGainType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kHealthGainType, name);
}

std::string_view Enum::EffectHealTypeToString(const EffectHealType effect_health_type)
{
    return enum_mapping::GetNameForValue(kEffectHealType, effect_health_type);
}

EffectHealType Enum::StringToEffectHealType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectHealType, name);
}

std::string_view Enum::ConditionTypeToString(const ConditionType condition)
{
    return enum_mapping::GetNameForValue(kConditionTypes, condition);
}

ConditionType Enum::StringToConditionType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kConditionTypes, name);
}

std::string_view Enum::EffectValueTypeToString(const EffectValueType value_type)
{
    return enum_mapping::GetNameForValue(kEffectValueTypes, value_type);
}

EffectValueType Enum::StringToEffectValueType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectValueTypes, name);
}

std::string_view Enum::EffectOperationTypeToString(const EffectOperationType operation_type)
{
    return enum_mapping::GetNameForValue(kEffectOperationTypes, operation_type);
}

EffectOperationType Enum::StringToEffectOperationType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEffectOperationTypes, name);
}

std::string_view Enum::ExpressionDataSourceTypeToString(const ExpressionDataSourceType effect_data_source_type)
{
    return enum_mapping::GetNameForValue(kExpressionDataSourceTypes, effect_data_source_type);
}

ExpressionDataSourceType Enum::StringToExpressionDataSourceType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kExpressionDataSourceTypes, name);
}

std::string_view Enum::ComparisonTypeToString(const ComparisonType comparison_type)
{
    return enum_mapping::GetNameForValue(kComparisonTypes, comparison_type);
}

ComparisonType Enum::StringToComparisonType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kComparisonTypes, name);
}

std::string_view Enum::CombatClassToString(const CombatClass combat_class)
{
    return enum_mapping::GetNameForValue(kCombatClasses, combat_class);
}

CombatClass Enum::StringToCombatClass(const std::string_view name)
{
    return enum_mapping::GetValueForName(kCombatClasses, name);
}

std::string_view Enum::CombatAffinityToString(const CombatAffinity combat_affinity)
{
    return enum_mapping::GetNameForValue(kCombatAffinities, combat_affinity);
}

CombatAffinity Enum::StringToCombatAffinity(const std::string_view name)
{
    return enum_mapping::GetValueForName(kCombatAffinities, name);
}

std::string_view Enum::ZoneEffectShapeToString(const ZoneEffectShape shape)
{
    return enum_mapping::GetNameForValue(kZoneEffectShapes, shape);
}

ZoneEffectShape Enum::StringToZoneEffectShape(const std::string_view name)
{
    return enum_mapping::GetValueForName(kZoneEffectShapes, name);
}

std::string_view Enum::SkillTargetingTypeToString(const SkillTargetingType targeting_type)
{
    return enum_mapping::GetNameForValue(kSkillTargetingTypes, targeting_type);
}

SkillTargetingType Enum::StringToSkillTargetingType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kSkillTargetingTypes, name);
}

std::string_view Enum::SkillDeploymentTypeToString(const SkillDeploymentType deployment_type)
{
    return enum_mapping::GetNameForValue(kSkillDeploymentTypes, deployment_type);
}

SkillDeploymentType Enum::StringToSkillDeploymentType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kSkillDeploymentTypes, name);
}

std::string_view Enum::GuidanceTypeToString(const GuidanceType guidance_type)
{
    return enum_mapping::GetNameForValue(kGuidanceTypes, guidance_type);
}

GuidanceType Enum::StringToGuidanceType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kGuidanceTypes, name);
}

std::string_view Enum::AllegianceTypeToString(const AllegianceType allegiance_type)
{
    return enum_mapping::GetNameForValue(kAllegianceType, allegiance_type);
}

AllegianceType Enum::StringToAllegianceType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kAllegianceType, name);
}

std::string_view Enum::HexGridCardinalDirectionToString(const HexGridCardinalDirection spawn_direction)
{
    return enum_mapping::GetNameForValue(kHexGridCardinalDirections, spawn_direction);
}

HexGridCardinalDirection Enum::StringToHexGridCardinalDirection(const std::string_view name)
{
    return enum_mapping::GetValueForName(kHexGridCardinalDirections, name);
}

std::string_view Enum::AbilitySelectionTypeToString(const AbilitySelectionType ability_selection)
{
    return enum_mapping::GetNameForValue(kAbilitySelections, ability_selection);
}

AbilitySelectionType Enum::StringToAbilitySelectionType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kAbilitySelections, name);
}

std::string_view Enum::AbilityTypeToString(const AbilityType ability_type)
{
    return enum_mapping::GetNameForValue(kAbilityTypes, ability_type);
}

AbilityType Enum::StringToAbilityType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kAbilityTypes, name);
}

std::string_view Enum::AbilityUpdateTypeToString(const AbilityUpdateType update_type)
{
    return enum_mapping::GetNameForValue(kAbilityUpdateTypes, update_type);
}

AbilityUpdateType Enum::StringToAbilityUpdateType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kAbilityUpdateTypes, name);
}

std::string_view Enum::TeamToString(const Team team)
{
    return enum_mapping::GetNameForValue(kTeamsNames, team);
}
Team Enum::StringToTeam(const std::string_view name)
{
    return enum_mapping::GetValueForName(kTeamsNames, name);
}

std::string_view Enum::StatTypeToString(const StatType stat_type)
{
    return enum_mapping::GetNameForValue(kStatTypes, stat_type);
}

StatType Enum::StringToStatType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kStatTypes, name);
}

std::string_view Enum::StatEvaluationTypeToString(const StatEvaluationType stat_evaluation_type)
{
    return enum_mapping::GetNameForValue(kStatEvaluationTypes, stat_evaluation_type);
}

StatEvaluationType Enum::StringToStatEvaluationType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kStatEvaluationTypes, name);
}

std::string_view Enum::EventTypeToString(const EventType event_type)
{
    return enum_mapping::GetNameForValue(kEventTypes, event_type);
}

EventType Enum::StringToEventType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kEventTypes, name);
}

std::string_view Enum::ActivationTriggerTypeToString(const ActivationTriggerType trigger_type)
{
    return enum_mapping::GetNameForValue(kActivationTriggerTypes, trigger_type);
}

ActivationTriggerType Enum::StringToActivationTriggerType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kActivationTriggerTypes, name);
}

std::string_view Enum::DecisionNextActionToString(const DecisionNextAction next_action)
{
    return enum_mapping::GetNameForValue(kDecisionNextActions, next_action);
}

DecisionNextAction Enum::StringToDecisionNextAction(const std::string_view name)
{
    return enum_mapping::GetValueForName(kDecisionNextActions, name);
}

std::string_view Enum::LogLevelToString(const LogLevel log_level)
{
    return enum_mapping::GetNameForValue(kLogLevels, log_level);
}

LogLevel Enum::StringToLogLevel(const std::string_view name)
{
    return enum_mapping::GetValueForName(kLogLevels, name);
}

std::string_view Enum::AttachedEffectStateTypeToString(const AttachedEffectStateType attache_effect_state_type)
{
    return enum_mapping::GetNameForValue(kAttachedEffectStateTypes, attache_effect_state_type);
}

AttachedEffectStateType Enum::StringToAttachedEffectStateType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kAttachedEffectStateTypes, name);
}

std::string_view Enum::CombatUnitStateToString(const CombatUnitState state)
{
    return enum_mapping::GetNameForValue(kCombatUnitStates, state);
}

CombatUnitState Enum::StringToCombatUnitState(const std::string_view name)
{
    return enum_mapping::GetValueForName(kCombatUnitStates, name);
}

std::string_view Enum::ReservedPositionTypeToString(const ReservedPositionType reserved_position_type)
{
    return enum_mapping::GetNameForValue(kReservedPositionTypes, reserved_position_type);
}

ReservedPositionType Enum::StringToReservedPositionType(const std::string_view name)
{
    return enum_mapping::GetValueForName(kReservedPositionTypes, name);
}

std::string_view Enum::AttachedEntityTypeToString(const AttachedEntityType attached_entity_type)
{
    return enum_mapping::GetNameForValue(kAttachedEntityTypes, attached_entity_type);
}

AttachedEntityType Enum::StringToAttachedEntityType(const std::string_view attached_entity_name)
{
    return enum_mapping::GetValueForName(kAttachedEntityTypes, attached_entity_name);
}

std::string_view Enum::ValidationTypeToString(const ValidationType validation_type)
{
    return enum_mapping::GetNameForValue(kValidationTypes, validation_type);
}

ValidationType Enum::StringToValidationType(const std::string_view validation_name)
{
    return enum_mapping::GetValueForName(kValidationTypes, validation_name);
}

std::string_view Enum::ValidationStartEntityToString(const ValidationStartEntity start_entity_type)
{
    return enum_mapping::GetNameForValue(kValidationStartEntities, start_entity_type);
}

ValidationStartEntity Enum::StringToValidationStartEntity(const std::string_view start_entity_name)
{
    return enum_mapping::GetValueForName(kValidationStartEntities, start_entity_name);
}

std::string_view Enum::AugmentTypeToString(const AugmentType augment_type)
{
    return enum_mapping::GetNameForValue(kAugmentTypes, augment_type);
}

AugmentType Enum::StringToAugmentType(const std::string_view augment_type_name)
{
    return enum_mapping::GetValueForName(kAugmentTypes, augment_type_name);
}

std::string_view Enum::SuitTypeToString(const SuitType suit_type)
{
    return enum_mapping::GetNameForValue(kSuitTypes, suit_type);
}

SuitType Enum::StringToSuitType(const std::string_view suit_type_name)
{
    return enum_mapping::GetValueForName(kSuitTypes, suit_type_name);
}

std::string_view Enum::WeaponTypeToString(const WeaponType weapon_type)
{
    return enum_mapping::GetNameForValue(kWeaponTypes, weapon_type);
}

WeaponType Enum::StringToWeaponType(const std::string_view weapon_type_name)
{
    return enum_mapping::GetValueForName(kWeaponTypes, weapon_type_name);
}

std::string_view Enum::DroneAugmentTypeToString(const DroneAugmentType drone_augment_type)
{
    return enum_mapping::GetNameForValue(kDroneAugmentTypes, drone_augment_type);
}

DroneAugmentType Enum::StringToDroneAugmentType(const std::string_view drone_augment_type_name)
{
    return enum_mapping::GetValueForName(kDroneAugmentTypes, drone_augment_type_name);
}

std::string_view Enum::AbilityTriggerContextRequirementToString(
    const AbilityTriggerContextRequirement context_requirement_type)
{
    return enum_mapping::GetNameForValue(kAbilityTriggerContextRequirements, context_requirement_type);
}

AbilityTriggerContextRequirement Enum::StringToAbilityTriggerContextRequirement(
    const std::string_view context_requirement_name)
{
    return enum_mapping::GetValueForName(kAbilityTriggerContextRequirements, context_requirement_name);
}

std::string_view Enum::DeathReasonTypeToString(const DeathReasonType death_reason)
{
    return enum_mapping::GetNameForValue(kDeathReasonTypes, death_reason);
}
std::string_view Enum::PredefinedGridPositionToString(const PredefinedGridPosition pos)
{
    return enum_mapping::GetNameForValue(kPredefinedGridPositions, pos);
}
PredefinedGridPosition Enum::StringToPredefinedGridPosition(const std::string_view str)
{
    return enum_mapping::GetValueForName(kPredefinedGridPositions, str);
}
}  // namespace simulation
