// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Player/CodexLSPlayerState.h"

#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/GAS/CodexLSAbilitySystemComponent.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "AgentGameTest/Codex/GAS/CodexLSGameplayAbilities.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"

ACodexLSPlayerState::ACodexLSPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UCodexLSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UCodexLSAttributeSet>(TEXT("AttributeSet"));
	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* ACodexLSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ACodexLSPlayerState::InitializeAbilitySystem(AActor* AvatarActor)
{
	if (!AbilitySystemComponent || !AvatarActor)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, AvatarActor);
	UE_LOG(LogCodexLastStand, Log, TEXT("ASC Initialized | Owner=%s Avatar=%s"),
		*GetName(), *GetNameSafe(AvatarActor));
}

void ACodexLSPlayerState::ApplyDefaultAttributes(TSubclassOf<UGameplayEffect> DefaultAttributesEffect)
{
	if (!HasAuthority() || !AbilitySystemComponent || !DefaultAttributesEffect || bDefaultAttributesApplied)
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec =
		AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1.0f, Context);

	if (Spec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		bDefaultAttributesApplied = true;
		UE_LOG(LogCodexLastStand, Log, TEXT("Default Attributes Applied | Health=%.0f MaxHealth=%.0f"),
			AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
	}
}

void ACodexLSPlayerState::GrantAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilityClasses)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	int32 GrantedCount = 0;
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : AbilityClasses)
	{
		if (!AbilityClass)
		{
			continue;
		}

		bool bAlreadyGranted = false;
		for (const FGameplayAbilitySpec& ExistingSpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (ExistingSpec.Ability && ExistingSpec.Ability->GetClass() == AbilityClass)
			{
				bAlreadyGranted = true;
				break;
			}
		}

		if (bAlreadyGranted)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
		if (const UCodexLSGameplayAbility* AbilityCDO =
			AbilityClass->GetDefaultObject<UCodexLSGameplayAbility>())
		{
			if (AbilityCDO->GetInputTag().IsValid())
			{
				AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityCDO->GetInputTag());
			}
		}

		AbilitySystemComponent->GiveAbility(AbilitySpec);
		++GrantedCount;
	}

	UE_LOG(LogCodexLastStand, Log, TEXT("Abilities Granted | New=%d Total=%d (duplicate-safe)"),
		GrantedCount, AbilitySystemComponent->GetActivatableAbilities().Num());
}
