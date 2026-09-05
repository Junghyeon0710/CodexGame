// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CodexLSEnemySpawnPoint.generated.h"

class UArrowComponent;
class USceneComponent;

/** Movable, level-authored enemy spawn marker. */
UCLASS()
class CODEXGAME_API ACodexLSEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ACodexLSEnemySpawnPoint();

	UFUNCTION(BlueprintPure, Category = "Last Stand|Spawn")
	FVector GetSpawnLocation() const;

	UFUNCTION(BlueprintPure, Category = "Last Stand|Spawn")
	FRotator GetSpawnRotation() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Spawn")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Spawn")
	TObjectPtr<UArrowComponent> SpawnArrow;
};
