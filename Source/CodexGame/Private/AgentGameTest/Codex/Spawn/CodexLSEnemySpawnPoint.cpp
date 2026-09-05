// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Spawn/CodexLSEnemySpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

ACodexLSEnemySpawnPoint::ACodexLSEnemySpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	Tags.Add(TEXT("Codex.EnemySpawnPoint"));

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = SceneRoot;

	SpawnArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnArrow"));
	SpawnArrow->SetupAttachment(SceneRoot);
	SpawnArrow->SetMobility(EComponentMobility::Movable);
	SpawnArrow->SetArrowColor(FLinearColor(0.05f, 0.8f, 1.0f));
	SpawnArrow->SetArrowSize(1.5f);
}

FVector ACodexLSEnemySpawnPoint::GetSpawnLocation() const
{
	return SpawnArrow ? SpawnArrow->GetComponentLocation() : GetActorLocation();
}

FRotator ACodexLSEnemySpawnPoint::GetSpawnRotation() const
{
	return SpawnArrow ? SpawnArrow->GetComponentRotation() : GetActorRotation();
}
