// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Player/CodexLSPlayerController.h"

#include "AgentGameTest/Codex/CodexLSLog.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"

ACodexLSPlayerController::ACodexLSPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ACodexLSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ACodexLSPlayerController::CodexDebugInputChord(FString Chord, bool bDash, float HoldSeconds)
{
	if (!IsLocalController() || !GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(DebugReleaseTimer);
	GetWorld()->GetTimerManager().ClearTimer(DebugDashPressTimer);
	GetWorld()->GetTimerManager().ClearTimer(DebugDashReleaseTimer);
	ReleaseDebugDash();
	ReleaseDebugKeys();

	Chord.ToUpperInline();
	if (Chord == TEXT("LMB"))
	{
		PressDebugKey(EKeys::LeftMouseButton);
	}
	else
	{
		for (const TCHAR KeyCharacter : Chord)
		{
			const FKey* Key = nullptr;
			switch (KeyCharacter)
			{
			case TEXT('W'): Key = &EKeys::W; break;
			case TEXT('A'): Key = &EKeys::A; break;
			case TEXT('S'): Key = &EKeys::S; break;
			case TEXT('D'): Key = &EKeys::D; break;
			default: break;
			}

			if (Key && !DebugHeldKeys.Contains(*Key))
			{
				PressDebugKey(*Key);
			}
		}
	}

	if (bDash)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DebugDashPressTimer, this, &ThisClass::PressDebugDash, 0.05f, false);
		GetWorld()->GetTimerManager().SetTimer(
			DebugDashReleaseTimer, this, &ThisClass::ReleaseDebugDash, 0.11f, false);
	}

	GetWorld()->GetTimerManager().SetTimer(
		DebugReleaseTimer,
		this,
		&ThisClass::ReleaseDebugKeys,
		FMath::Max(0.12f, HoldSeconds),
		false);

	UE_LOG(LogCodexLastStand, Log, TEXT("Debug Input Chord: %s | Dash=%s | Hold=%.2f"),
		*Chord, bDash ? TEXT("true") : TEXT("false"), HoldSeconds);
}

void ACodexLSPlayerController::CodexDebugSetMouse(int32 ScreenX, int32 ScreenY)
{
	SetMouseLocation(ScreenX, ScreenY);
	UE_LOG(LogCodexLastStand, Log, TEXT("Debug Mouse Position: X=%d Y=%d"), ScreenX, ScreenY);
}

void ACodexLSPlayerController::PressDebugKey(const FKey& Key)
{
	InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.0f));
	DebugHeldKeys.AddUnique(Key);
}

void ACodexLSPlayerController::ReleaseDebugKeys()
{
	for (const FKey& Key : DebugHeldKeys)
	{
		InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.0f));
	}
	DebugHeldKeys.Reset();
}

void ACodexLSPlayerController::PressDebugDash()
{
	InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Pressed, 1.0f));
}

void ACodexLSPlayerController::ReleaseDebugDash()
{
	InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Released, 0.0f));
}
