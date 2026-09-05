// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Spawn/CodexLSEnemySpawner.h"

#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/Spawn/CodexLSEnemySpawnPoint.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"

namespace
{
	const TCHAR* GetEnemyArchetypeLabel(ECodexLSEnemyArchetype Archetype)
	{
		return Archetype == ECodexLSEnemyArchetype::Grunt ? TEXT("Grunt") : TEXT("Runner");
	}
}

ACodexLSEnemySpawner::ACodexLSEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	GruntClass = ACodexLSEnemyGrunt::StaticClass();
	RunnerClass = ACodexLSEnemyRunner::StaticClass();
	Tags.Add(TEXT("Codex.EnemySpawner"));
}

void ACodexLSEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	RefreshSpawnPoints();
}

void ACodexLSEnemySpawner::RefreshSpawnPoints()
{
	SpawnPoints.Reset();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACodexLSEnemySpawnPoint> It(World); It; ++It)
		{
			ACodexLSEnemySpawnPoint* SpawnPoint = *It;
			if (IsValid(SpawnPoint))
			{
				SpawnPoints.Add(SpawnPoint);
			}
		}
	}

	SpawnPoints.Sort([](
		const TWeakObjectPtr<ACodexLSEnemySpawnPoint>& Left,
		const TWeakObjectPtr<ACodexLSEnemySpawnPoint>& Right)
	{
		return GetNameSafe(Left.Get()) < GetNameSafe(Right.Get());
	});

	for (const TWeakObjectPtr<ACodexLSEnemySpawnPoint>& WeakSpawnPoint : SpawnPoints)
	{
		if (const ACodexLSEnemySpawnPoint* SpawnPoint = WeakSpawnPoint.Get())
		{
			const FVector Location = SpawnPoint->GetSpawnLocation();
			UE_LOG(LogCodexLastStand, Log,
				TEXT("CODEX_STEP3_SPAWN_POINT | Name=%s Location=(%.1f,%.1f,%.1f) Yaw=%.1f"),
				*SpawnPoint->GetName(), Location.X, Location.Y, Location.Z,
				SpawnPoint->GetSpawnRotation().Yaw);
		}
	}

	if (SpawnPoints.IsEmpty())
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_SPAWN_FAILURE | Archetype=None Attempts=0 Reason=NoSpawnPointsDiscovered"));
	}
}

bool ACodexLSEnemySpawner::SpawnEnemy(
	ECodexLSEnemyArchetype Archetype,
	const AActor* PlayerActor,
	ACodexLSEnemyCharacter*& OutEnemy,
	FString& OutSpawnPointName,
	FVector& OutSpawnLocation,
	float& OutPlayerDistance)
{
	OutEnemy = nullptr;
	OutSpawnPointName.Reset();
	OutSpawnLocation = FVector::ZeroVector;
	OutPlayerDistance = 0.0f;

	UWorld* World = GetWorld();
	if (!World)
	{
		LogSpawnFailure(Archetype, 0, TEXT("InvalidWorld"));
		return false;
	}

	if (!IsValid(PlayerActor))
	{
		LogSpawnFailure(Archetype, 0, TEXT("InvalidPlayer"));
		return false;
	}

	const TSubclassOf<ACodexLSEnemyCharacter> EnemyClass = ResolveEnemyClass(Archetype);
	if (!EnemyClass)
	{
		LogSpawnFailure(Archetype, 0, TEXT("EnemyClassNotConfigured"));
		return false;
	}

	if (SpawnPoints.IsEmpty())
	{
		RefreshSpawnPoints();
	}

	TArray<ACodexLSEnemySpawnPoint*> EligibleSpawnPoints;
	for (const TWeakObjectPtr<ACodexLSEnemySpawnPoint>& WeakSpawnPoint : SpawnPoints)
	{
		ACodexLSEnemySpawnPoint* SpawnPoint = WeakSpawnPoint.Get();
		if (!IsValid(SpawnPoint))
		{
			continue;
		}

		const float BaseDistance = FVector::Dist2D(
			PlayerActor->GetActorLocation(), SpawnPoint->GetSpawnLocation());
		if (BaseDistance + SpawnOffsetRadius >= MinimumPlayerDistance)
		{
			EligibleSpawnPoints.Add(SpawnPoint);
		}
	}

	if (EligibleSpawnPoints.Num() > 1 && LastSpawnPoint.IsValid())
	{
		EligibleSpawnPoints.RemoveSingleSwap(LastSpawnPoint.Get());
	}

	if (EligibleSpawnPoints.IsEmpty())
	{
		LogSpawnFailure(Archetype, 0, TEXT("NoPointMeetsMinimumPlayerDistance"));
		return false;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavigationSystem)
	{
		LogSpawnFailure(Archetype, 0, TEXT("NavigationSystemUnavailable"));
		return false;
	}

	const ACodexLSEnemyCharacter* EnemyDefault = EnemyClass->GetDefaultObject<ACodexLSEnemyCharacter>();
	const UCapsuleComponent* DefaultCapsule = EnemyDefault ? EnemyDefault->GetCapsuleComponent() : nullptr;
	const float CapsuleRadius = DefaultCapsule ? DefaultCapsule->GetScaledCapsuleRadius() : 42.0f;
	const float CapsuleHalfHeight = DefaultCapsule ? DefaultCapsule->GetScaledCapsuleHalfHeight() : 88.0f;
	const int32 FirstPointIndex = FMath::RandRange(0, EligibleSpawnPoints.Num() - 1);
	FString LastFailureReason = TEXT("AttemptsExhausted");
	int32 AttemptsPerformed = 0;

	for (int32 Attempt = 1; Attempt <= FMath::Max(1, MaxSpawnAttempts); ++Attempt)
	{
		AttemptsPerformed = Attempt;
		ACodexLSEnemySpawnPoint* SpawnPoint =
			EligibleSpawnPoints[(FirstPointIndex + Attempt - 1) % EligibleSpawnPoints.Num()];
		OutSpawnPointName = SpawnPoint->GetName();

		if (ForcedSpawnFailuresRemaining > 0)
		{
			--ForcedSpawnFailuresRemaining;
			LastFailureReason = TEXT("DebugForcedFailure");
			UE_LOG(LogCodexLastStand, Warning,
				TEXT("CODEX_STEP3_SPAWN_FAILURE_FORCED | Archetype=%s Attempt=%d Point=%s RemainingForced=%d"),
				GetEnemyArchetypeLabel(Archetype), Attempt, *OutSpawnPointName,
				ForcedSpawnFailuresRemaining);
			continue;
		}

		const float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float RandomRadius = FMath::Sqrt(FMath::FRand()) * SpawnOffsetRadius;
		const FVector LocalOffset(
			FMath::Cos(RandomAngle) * RandomRadius,
			FMath::Sin(RandomAngle) * RandomRadius,
			0.0f);
		const FVector RequestedLocation =
			SpawnPoint->GetSpawnLocation() + SpawnPoint->GetSpawnRotation().RotateVector(LocalOffset);

		FNavLocation ProjectedLocation;
		if (!NavigationSystem->ProjectPointToNavigation(
			RequestedLocation, ProjectedLocation, NavigationProjectionExtent))
		{
			LastFailureReason = TEXT("NavigationProjectionFailed");
			continue;
		}

		OutPlayerDistance = FVector::Dist2D(PlayerActor->GetActorLocation(), ProjectedLocation.Location);
		if (OutPlayerDistance < MinimumPlayerDistance)
		{
			LastFailureReason = TEXT("PlayerDistanceTooSmall");
			continue;
		}

		OutSpawnLocation = ProjectedLocation.Location +
			FVector(0.0f, 0.0f, CapsuleHalfHeight + SpawnHeightPadding);
		if (bHasLastSpawnLocation &&
			FVector::DistSquared2D(OutSpawnLocation, LastSpawnLocation) <
			FMath::Square(MinimumLocationSeparation))
		{
			LastFailureReason = TEXT("TooCloseToPreviousSpawn");
			continue;
		}

		FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(CodexLSEnemySpawner), false, this);
		CollisionParams.AddIgnoredActor(SpawnPoint);
		const FCollisionShape CapsuleShape =
			FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
		const FQuat SpawnRotation = SpawnPoint->GetSpawnRotation().Quaternion();
		if (World->OverlapBlockingTestByProfile(
			OutSpawnLocation,
			SpawnRotation,
			UCollisionProfile::Pawn_ProfileName,
			CapsuleShape,
			CollisionParams))
		{
			LastFailureReason = TEXT("SpawnLocationBlocked");
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

		OutEnemy = World->SpawnActor<ACodexLSEnemyCharacter>(
			EnemyClass, OutSpawnLocation, SpawnPoint->GetSpawnRotation(), SpawnParameters);
		if (!OutEnemy)
		{
			LastFailureReason = TEXT("SpawnActorReturnedNull");
			continue;
		}

		LastSpawnPoint = SpawnPoint;
		LastSpawnLocation = OutEnemy->GetActorLocation();
		bHasLastSpawnLocation = true;
		OutSpawnLocation = LastSpawnLocation;

		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP3_SPAWN_POINT | Result=Success Archetype=%s Attempt=%d Point=%s Location=(%.1f,%.1f,%.1f) PlayerDistance=%.1f"),
			GetEnemyArchetypeLabel(Archetype), Attempt, *OutSpawnPointName,
			OutSpawnLocation.X, OutSpawnLocation.Y, OutSpawnLocation.Z, OutPlayerDistance);
		return true;
	}

	LogSpawnFailure(Archetype, AttemptsPerformed, LastFailureReason);
	return false;
}

void ACodexLSEnemySpawner::DebugForceNextSpawnFailures(int32 Count)
{
	ForcedSpawnFailuresRemaining = FMath::Max(0, Count);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_SPAWN_FAILURE_DEBUG_CONFIG | ForcedFailures=%d MaxAttempts=%d"),
		ForcedSpawnFailuresRemaining, MaxSpawnAttempts);
}

TSubclassOf<ACodexLSEnemyCharacter> ACodexLSEnemySpawner::ResolveEnemyClass(
	ECodexLSEnemyArchetype Archetype) const
{
	return Archetype == ECodexLSEnemyArchetype::Grunt ? GruntClass : RunnerClass;
}

void ACodexLSEnemySpawner::LogSpawnFailure(
	ECodexLSEnemyArchetype Archetype,
	int32 Attempts,
	const FString& Reason) const
{
	UE_LOG(LogCodexLastStand, Warning,
		TEXT("CODEX_STEP3_SPAWN_FAILURE | Archetype=%s Attempts=%d Reason=%s"),
		GetEnemyArchetypeLabel(Archetype), Attempts, *Reason);
}
