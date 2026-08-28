// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/CodexLSGameMode.h"

#include "AgentGameTest/Codex/Player/CodexLSPlayerCharacter.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerController.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerState.h"

ACodexLSGameMode::ACodexLSGameMode()
{
	DefaultPawnClass = ACodexLSPlayerCharacter::StaticClass();
	PlayerControllerClass = ACodexLSPlayerController::StaticClass();
	PlayerStateClass = ACodexLSPlayerState::StaticClass();
}
