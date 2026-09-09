// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "Input/UIActionBindingHandle.h"
#include "CodexLSMenuWidgets.generated.h"

class UCodexLSCommonButton;
class UCommonTextBlock;

/** Shared menu input policy: CommonUI Menu mode blocks gameplay input below it. */
UCLASS(Abstract)
class CODEXGAME_API UCodexLSActivatableMenuBase : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UCodexLSActivatableMenuBase();

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	UCodexLSCommonButton* FindMenuButton(const FName& WidgetName) const;
};

UCLASS(Abstract, Blueprintable)
class CODEXGAME_API UCodexLSMainMenuWidget : public UCodexLSActivatableMenuBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

private:
	void HandlePlayClicked();
	void HandleExitClicked();

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCodexLSCommonButton> PlayButton;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCodexLSCommonButton> ExitButton;
};

UCLASS(Abstract, Blueprintable)
class CODEXGAME_API UCodexLSPauseMenuWidget : public UCodexLSActivatableMenuBase
{
	GENERATED_BODY()

public:
	UCodexLSPauseMenuWidget();

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

private:
	void HandleResumeClicked();
	void HandleRestartClicked();
	void HandleMainMenuClicked();

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCodexLSCommonButton> ResumeButton;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCodexLSCommonButton> RestartButton;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCodexLSCommonButton> MainMenuButton;
};

UCLASS(Abstract)
class CODEXGAME_API UCodexLSResultWidgetBase : public UCodexLSActivatableMenuBase
{
	GENERATED_BODY()

public:
	void SetFinalScore(int32 InScore);

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FText GetResultTitle() const PURE_VIRTUAL(UCodexLSResultWidgetBase::GetResultTitle, return FText::GetEmpty(););

private:
	void HandleRestartClicked();
	void HandleMainMenuClicked();

	int32 FinalScore = 0;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ResultTitleText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> FinalScoreText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCodexLSCommonButton> RestartButton;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCodexLSCommonButton> MainMenuButton;
};

UCLASS(Abstract, Blueprintable)
class CODEXGAME_API UCodexLSGameOverWidget : public UCodexLSResultWidgetBase
{
	GENERATED_BODY()

protected:
	virtual FText GetResultTitle() const override;
};

UCLASS(Abstract, Blueprintable)
class CODEXGAME_API UCodexLSVictoryWidget : public UCodexLSResultWidgetBase
{
	GENERATED_BODY()

protected:
	virtual FText GetResultTitle() const override;
};
