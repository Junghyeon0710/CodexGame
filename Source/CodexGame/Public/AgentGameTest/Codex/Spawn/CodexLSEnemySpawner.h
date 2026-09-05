// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgentGameTest/Codex/Enemy/CodexLSEnemyCharacter.h"
#include "GameFramework/Actor.h"
#include "CodexLSEnemySpawner.generated.h"

class ACodexLSEnemySpawnPoint;

/**
 * Bounded, navigation-aware enemy spawn service for STEP 3.
 * Spawn points are discovered once and may be explicitly refreshed after level changes.
 */
UCLASS()
class CODEXGAME_API ACodexLSEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	ACodexLSEnemySpawner();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Spawn")
	void RefreshSpawnPoints();

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Spawn")
	bool SpawnEnemy(
		ECodexLSEnemyArchetype Archetype,
		const AActor* PlayerActor,
		ACodexLSEnemyCharacter*& OutEnemy,
		FString& OutSpawnPointName,
		FVector& OutSpawnLocation,
		float& OutPlayerDistance);

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Spawn|Debug")
	void DebugForceNextSpawnFailures(int32 Count);

	UFUNCTION(BlueprintPure, Category = "Last Stand|Spawn")
	int32 GetDiscoveredSpawnPointCount() const { return SpawnPoints.Num(); }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Spawn")
	int32 GetMaxSpawnAttempts() const { return MaxSpawnAttempts; }

private:
	TSubclassOf<ACodexLSEnemyCharacter> ResolveEnemyClass(ECodexLSEnemyArchetype Archetype) const;
	void LogSpawnFailure(ECodexLSEnemyArchetype Archetype, int32 Attempts, const FString& Reason) const;

	UPROPERTY(EditAnywhere, Category = "Last Stand|Spawn|Classes")
	TSubclassOf<ACodexLSEnemyCharacter> GruntClass;

	UPROPERTY(EditAnywhere, Category = "Last Stand|Spawn|Classes")
	TSubclassOf<ACodexLSEnemyCharacter> RunnerClass;

	UPROPERTY(EditAnywhere, Category = "Last Stand|Spawn", meta = (ClampMin = "0.0"))
	float MinimumPlayerDistance = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Last Stand|Spawn", meta = (ClampMin = "0.0"))
	float SpawnOffsetRadius = 180.0f;

	UPROPERTY(EditAnywhere, Category = "Last Stand|Spawn")
	FVector NavigationProjectionExtent = FVector(250.0f, 250.0f, 300.0f);

	UPROPERTY(EditAnywhere, Category = "Last Stand|Spawn", meta = (ClampMin = "0.0"))
	float MinimumLocationSeparation = 160.0f;

	UPROPERTY(EditAnywhere, Category = "Last Stand|Spawn", meta = (ClampMin = "0.0"))
	float SpawnHeightPadding = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Last Stand|Spawn", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxSpawnAttempts = 12;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ACodexLSEnemySpawnPoint>> SpawnPoints;

	TWeakObjectPtr<ACodexLSEnemySpawnPoint> LastSpawnPoint;
	FVector LastSpawnLocation = FVector::ZeroVector;
	bool bHasLastSpawnLocation = false;
	int32 ForcedSpawnFailuresRemaining = 0;
};
