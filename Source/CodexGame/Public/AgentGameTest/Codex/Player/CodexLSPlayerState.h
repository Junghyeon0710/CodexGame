// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "CodexLSPlayerState.generated.h"

class UCodexLSAbilitySystemComponent;
class UCodexLSAttributeSet;
class UGameplayAbility;
class UGameplayEffect;

UCLASS()
class CODEXGAME_API ACodexLSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACodexLSPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UCodexLSAbilitySystemComponent* GetCodexAbilitySystemComponent() const
	{
		return AbilitySystemComponent;
	}

	const UCodexLSAttributeSet* GetAttributeSet() const { return AttributeSet; }

	void InitializeAbilitySystem(AActor* AvatarActor);
	void ApplyDefaultAttributes(TSubclassOf<UGameplayEffect> DefaultAttributesEffect);
	void GrantAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilityClasses);

private:
	UPROPERTY(VisibleAnywhere, Category = "Last Stand|GAS")
	TObjectPtr<UCodexLSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCodexLSAttributeSet> AttributeSet;

	bool bDefaultAttributesApplied = false;
};
