// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "CodexLSEnemyCharacter.generated.h"

class ACodexLSEnemyCharacter;
class UCodexLSAbilitySystemComponent;
class UCodexLSAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UMaterialInterface;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FCodexLSEnemyDeathSignature, ACodexLSEnemyCharacter*, Enemy);

UENUM(BlueprintType)
enum class ECodexLSEnemyArchetype : uint8
{
	Grunt,
	Runner
};

UCLASS(Abstract)
class CODEXGAME_API ACodexLSEnemyCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACodexLSEnemyCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	float GetAttackDamage() const { return AttackDamage; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	float GetAttackCooldown() const { return AttackCooldown; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	ECodexLSEnemyArchetype GetEnemyArchetype() const { return EnemyArchetype; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	int32 GetScoreValue() const { return ScoreValue; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Enemy")
	FString GetEnemyArchetypeName() const;

	bool TryActivateMeleeAbility();
	bool PerformMeleeAttack();
	bool HasUnobstructedMeleeTarget(const AActor* TargetActor) const;
	void SetCombatTarget(AActor* NewTarget);
	AActor* GetCombatTarget() const { return CombatTarget.Get(); }
	void StopEnemyAI();
	void StopCombatForGameEnd();

	UPROPERTY(BlueprintAssignable, Category = "Last Stand|Enemy")
	FCodexLSEnemyDeathSignature OnEnemyDeath;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Visual")
	TObjectPtr<UStaticMeshComponent> VisibleMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy|Visual")
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy")
	ECodexLSEnemyArchetype EnemyArchetype = ECodexLSEnemyArchetype::Grunt;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy", meta = (ClampMin = "1.0"))
	float InitialMaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy", meta = (ClampMin = "0.0"))
	float AttackDamage = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy", meta = (ClampMin = "0.1"))
	float AttackCooldown = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy", meta = (ClampMin = "50.0"))
	float AttackRange = 165.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy", meta = (ClampMin = "10.0"))
	float MeleeTraceRadius = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy")
	FLinearColor EnemyColor = FLinearColor(0.35f, 0.04f, 0.04f, 1.0f);

	FGameplayTag EnemyTypeTag;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy", meta = (ClampMin = "0.0"))
	float DestroyDelay = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Enemy", meta = (ClampMin = "0"))
	int32 ScoreValue = 100;

private:
	void InitializeAbilitySystem();
	void ApplyDefaultAttributes();
	void GrantDefaultAbility();
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void EnterDeathState();
	void ApplyDebugColor();

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|GAS")
	TObjectPtr<UCodexLSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCodexLSAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|GAS")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|GAS")
	TSubclassOf<UGameplayAbility> MeleeAbility;

	TWeakObjectPtr<AActor> CombatTarget;
	FDelegateHandle HealthChangedDelegateHandle;
	bool bAttributesApplied = false;
	bool bDead = false;
};

UCLASS()
class CODEXGAME_API ACodexLSEnemyGrunt : public ACodexLSEnemyCharacter
{
	GENERATED_BODY()

public:
	ACodexLSEnemyGrunt();
};

UCLASS()
class CODEXGAME_API ACodexLSEnemyRunner : public ACodexLSEnemyCharacter
{
	GENERATED_BODY()

public:
	ACodexLSEnemyRunner();
};
