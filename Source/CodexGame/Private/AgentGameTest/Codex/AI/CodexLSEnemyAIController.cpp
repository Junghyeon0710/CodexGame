// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/AI/CodexLSEnemyAIController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/Enemy/CodexLSEnemyCharacter.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

ACodexLSEnemyAIController::ACodexLSEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.15f;
	bAttachToPawn = true;
}

void ACodexLSEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledEnemy = Cast<ACodexLSEnemyCharacter>(InPawn);
	CachedTarget.Reset();
	CurrentState = ECodexLSEnemyAIState::Idle;
	NextTargetSearchTime = 0.0f;

	if (!ControlledEnemy)
	{
		UE_LOG(LogCodexLastStand, Error, TEXT("Enemy AI possessed an invalid pawn: %s"),
			*GetNameSafe(InPawn));
		return;
	}

	AcquireTarget();
	UE_LOG(LogCodexLastStand, Log, TEXT("Enemy AI Possessed | Enemy=%s Type=%s"),
		*ControlledEnemy->GetName(), *ControlledEnemy->GetEnemyArchetypeName());
}

void ACodexLSEnemyAIController::OnUnPossess()
{
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	CachedTarget.Reset();
	ControlledEnemy = nullptr;
	CurrentState = ECodexLSEnemyAIState::Idle;

	Super::OnUnPossess();
}

void ACodexLSEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ControlledEnemy || ControlledEnemy->IsDead())
	{
		EnterDeadState();
		return;
	}

	if (!IsTargetAlive())
	{
		ControlledEnemy->SetCombatTarget(nullptr);
		CachedTarget.Reset();
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		SetEnemyAIState(ECodexLSEnemyAIState::Idle);

		if (!AcquireTarget())
		{
			return;
		}
	}

	UpdateCombatState();
}

FString ACodexLSEnemyAIController::GetEnemyAIStateName() const
{
	switch (CurrentState)
	{
	case ECodexLSEnemyAIState::Idle: return TEXT("Idle");
	case ECodexLSEnemyAIState::Chase: return TEXT("Chase");
	case ECodexLSEnemyAIState::Attack: return TEXT("Attack");
	case ECodexLSEnemyAIState::Dead: return TEXT("Dead");
	default: return TEXT("Unknown");
	}
}

void ACodexLSEnemyAIController::EnterDeadState()
{
	if (CurrentState == ECodexLSEnemyAIState::Dead)
	{
		return;
	}

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	CachedTarget.Reset();
	SetEnemyAIState(ECodexLSEnemyAIState::Dead);
	SetActorTickEnabled(false);
}

bool ACodexLSEnemyAIController::AcquireTarget()
{
	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() < NextTargetSearchTime)
	{
		return false;
	}

	NextTargetSearchTime = World->GetTimeSeconds() + TargetSearchInterval;
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!PlayerCharacter)
	{
		return false;
	}

	UAbilitySystemComponent* PlayerASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter);
	if (!PlayerASC ||
		PlayerASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Player_Dead) ||
		PlayerASC->GetNumericAttribute(UCodexLSAttributeSet::GetHealthAttribute()) <= 0.0f)
	{
		return false;
	}

	CachedTarget = PlayerCharacter;
	ControlledEnemy->SetCombatTarget(PlayerCharacter);
	UE_LOG(LogCodexLastStand, Log, TEXT("Enemy Target Acquired | Enemy=%s Target=%s"),
		*GetNameSafe(ControlledEnemy), *PlayerCharacter->GetName());
	return true;
}

bool ACodexLSEnemyAIController::IsTargetAlive() const
{
	AActor* TargetActor = CachedTarget.Get();
	if (!TargetActor)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC || TargetASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Player_Dead))
	{
		return false;
	}

	return TargetASC->GetNumericAttribute(UCodexLSAttributeSet::GetHealthAttribute()) > 0.0f;
}

void ACodexLSEnemyAIController::UpdateCombatState()
{
	AActor* TargetActor = CachedTarget.Get();
	if (!ControlledEnemy || !TargetActor)
	{
		return;
	}

	const float Distance2D = FVector::Dist2D(
		ControlledEnemy->GetActorLocation(), TargetActor->GetActorLocation());

	if (Distance2D <= ControlledEnemy->GetAttackRange())
	{
		StopMovement();
		SetFocus(TargetActor, EAIFocusPriority::Gameplay);
		ControlledEnemy->SetCombatTarget(TargetActor);
		SetEnemyAIState(ECodexLSEnemyAIState::Attack);

		if (UAbilitySystemComponent* ASC = ControlledEnemy->GetAbilitySystemComponent())
		{
			const bool bAbilityBusy =
				ASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Enemy_Attacking) ||
				ASC->HasMatchingGameplayTag(CodexLSGameplayTags::Cooldown_Enemy_MeleeAttack) ||
				ASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Enemy_Dead);
			if (!bAbilityBusy)
			{
				ControlledEnemy->TryActivateMeleeAbility();
			}
		}
		return;
	}

	ClearFocus(EAIFocusPriority::Gameplay);
	ControlledEnemy->SetCombatTarget(TargetActor);
	const bool bEnteredChase = CurrentState != ECodexLSEnemyAIState::Chase;
	SetEnemyAIState(ECodexLSEnemyAIState::Chase);

	if (bEnteredChase || GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		const float AcceptanceRadius = FMath::Max(25.0f, ControlledEnemy->GetAttackRange() * 0.8f);
		const EPathFollowingRequestResult::Type Result =
			MoveToActor(TargetActor, AcceptanceRadius, false, true, true, nullptr, true);

		UE_LOG(LogCodexLastStand, Log,
			TEXT("Enemy MoveTo | Enemy=%s Type=%s Result=%d Acceptance=%.0f Distance=%.0f"),
			*ControlledEnemy->GetName(), *ControlledEnemy->GetEnemyArchetypeName(),
			static_cast<int32>(Result), AcceptanceRadius, Distance2D);
	}
}

void ACodexLSEnemyAIController::SetEnemyAIState(ECodexLSEnemyAIState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	const FString OldStateName = GetEnemyAIStateName();
	CurrentState = NewState;
	UE_LOG(LogCodexLastStand, Log, TEXT("Enemy AI State | Enemy=%s %s -> %s"),
		*GetNameSafe(ControlledEnemy), *OldStateName, *GetEnemyAIStateName());
}
