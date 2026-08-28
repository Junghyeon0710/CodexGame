// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/GAS/CodexLSAbilitySystemComponent.h"

#include "AgentGameTest/Codex/CodexLSLog.h"
#include "GameplayAbilitySpec.h"

bool UCodexLSAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return false;
	}

	bool bMatchedAbility = false;
	bool bActivatedAbility = false;
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		bMatchedAbility = true;
		AbilitySpecInputPressed(AbilitySpec);

		if (!AbilitySpec.IsActive())
		{
			bActivatedAbility |= TryActivateAbility(AbilitySpec.Handle);
		}
		else
		{
			bActivatedAbility = true;
		}
	}

	UE_LOG(LogCodexLastStand, Log, TEXT("Ability Input: %s | Matched=%s Activated=%s"),
		*InputTag.ToString(),
		bMatchedAbility ? TEXT("true") : TEXT("false"),
		bActivatedAbility ? TEXT("true") : TEXT("false"));

	return bActivatedAbility;
}

void UCodexLSAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpecInputReleased(AbilitySpec);
		}
	}
}
