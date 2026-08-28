// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/GAS/CodexLSGameplayEffects.h"

#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UCodexLSGE_DefaultAttributes::UCodexLSGE_DefaultAttributes()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& MaxHealthModifier = Modifiers.AddDefaulted_GetRef();
	MaxHealthModifier.Attribute = UCodexLSAttributeSet::GetMaxHealthAttribute();
	MaxHealthModifier.ModifierOp = EGameplayModOp::Override;
	MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.0f));

	FGameplayModifierInfo& HealthModifier = Modifiers.AddDefaulted_GetRef();
	HealthModifier.Attribute = UCodexLSAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Override;
	HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.0f));
}

UCodexLSGE_Damage::UCodexLSGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = CodexLSGameplayTags::Data_Damage;

	FGameplayModifierInfo& DamageModifier = Modifiers.AddDefaulted_GetRef();
	DamageModifier.Attribute = UCodexLSAttributeSet::GetIncomingDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);
}

UCodexLSGE_PrimaryAttackCooldown::UCodexLSGE_PrimaryAttackCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.3f));

	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("PrimaryAttackCooldownTags"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(CodexLSGameplayTags::Cooldown_Player_PrimaryAttack);
	TargetTagsComponent->SetAndApplyTargetTagChanges(TagChanges);
}

UCodexLSGE_DashCooldown::UCodexLSGE_DashCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));

	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("DashCooldownTags"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(CodexLSGameplayTags::Cooldown_Player_Dash);
	TargetTagsComponent->SetAndApplyTargetTagChanges(TagChanges);
}

UCodexLSGE_EnemyDefaultAttributes::UCodexLSGE_EnemyDefaultAttributes()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat MaxHealthMagnitude;
	MaxHealthMagnitude.DataTag = CodexLSGameplayTags::Data_MaxHealth;

	FGameplayModifierInfo& MaxHealthModifier = Modifiers.AddDefaulted_GetRef();
	MaxHealthModifier.Attribute = UCodexLSAttributeSet::GetMaxHealthAttribute();
	MaxHealthModifier.ModifierOp = EGameplayModOp::Override;
	MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MaxHealthMagnitude);

	FSetByCallerFloat HealthMagnitude;
	HealthMagnitude.DataTag = CodexLSGameplayTags::Data_Health;

	FGameplayModifierInfo& HealthModifier = Modifiers.AddDefaulted_GetRef();
	HealthModifier.Attribute = UCodexLSAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Override;
	HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthMagnitude);
}

UCodexLSGE_EnemyMeleeCooldown::UCodexLSGE_EnemyMeleeCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat CooldownMagnitude;
	CooldownMagnitude.DataTag = CodexLSGameplayTags::Data_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(CooldownMagnitude);

	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("EnemyMeleeCooldownTags"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(CodexLSGameplayTags::Cooldown_Enemy_MeleeAttack);
	TargetTagsComponent->SetAndApplyTargetTagChanges(TagChanges);
}
