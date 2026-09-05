// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "CodexLSEnemyAIController.generated.h"

class ACodexLSEnemyCharacter;

UENUM(BlueprintType)
enum class ECodexLSEnemyAIState : uint8
{
	Idle,
	Chase,
	Attack,
	Suspended,
	Dead
};

UCLASS()
class CODEXGAME_API ACodexLSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACodexLSEnemyAIController();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Last Stand|AI")
	ECodexLSEnemyAIState GetEnemyAIState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|AI")
	FString GetEnemyAIStateName() const;

	AActor* GetTargetActor() const { return CachedTarget.Get(); }
	void EnterDeadState();
	void SuspendForGameEnd();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	bool AcquireTarget();
	bool IsTargetAlive() const;
	void UpdateCombatState();
	void SetEnemyAIState(ECodexLSEnemyAIState NewState);

	UPROPERTY(Transient)
	TObjectPtr<ACodexLSEnemyCharacter> ControlledEnemy;

	TWeakObjectPtr<AActor> CachedTarget;
	ECodexLSEnemyAIState CurrentState = ECodexLSEnemyAIState::Idle;
	float NextTargetSearchTime = 0.0f;
	float TargetSearchInterval = 1.0f;
};
