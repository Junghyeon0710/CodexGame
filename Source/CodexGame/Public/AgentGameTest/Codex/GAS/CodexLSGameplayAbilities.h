// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "CodexLSGameplayAbilities.generated.h"

UCLASS(Abstract)
class CODEXGAME_API UCodexLSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCodexLSGameplayAbility();

	const FGameplayTag& GetInputTag() const { return InputTag; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

UCLASS()
class CODEXGAME_API UCodexLSGA_PrimaryAttack : public UCodexLSGameplayAbility
{
	GENERATED_BODY()

public:
	UCodexLSGA_PrimaryAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Attack")
	float Damage = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Attack")
	float AttackRange = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};

UCLASS()
class CODEXGAME_API UCodexLSGA_Dash : public UCodexLSGameplayAbility
{
	GENERATED_BODY()

public:
	UCodexLSGA_Dash();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Dash")
	float DashSpeed = 1400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Dash")
	float DashDuration = 0.18f;

private:
	void FinishDash();

	FTimerHandle DashEndTimer;
};
