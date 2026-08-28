// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffect.h"
#include "CodexLSGameplayEffects.generated.h"

UCLASS()
class CODEXGAME_API UCodexLSGE_DefaultAttributes : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UCodexLSGE_DefaultAttributes();
};

UCLASS()
class CODEXGAME_API UCodexLSGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UCodexLSGE_Damage();
};

UCLASS()
class CODEXGAME_API UCodexLSGE_PrimaryAttackCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UCodexLSGE_PrimaryAttackCooldown();
};

UCLASS()
class CODEXGAME_API UCodexLSGE_DashCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UCodexLSGE_DashCooldown();
};

UCLASS()
class CODEXGAME_API UCodexLSGE_EnemyDefaultAttributes : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UCodexLSGE_EnemyDefaultAttributes();
};

UCLASS()
class CODEXGAME_API UCodexLSGE_EnemyMeleeCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UCodexLSGE_EnemyMeleeCooldown();
};
