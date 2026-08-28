// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "CodexLSAbilitySystemComponent.generated.h"

UCLASS()
class CODEXGAME_API UCodexLSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	bool AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
};
