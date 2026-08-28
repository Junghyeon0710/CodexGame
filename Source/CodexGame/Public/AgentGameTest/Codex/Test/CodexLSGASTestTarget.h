// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "CodexLSGASTestTarget.generated.h"

class UCodexLSAbilitySystemComponent;
class UCodexLSAttributeSet;
class USceneComponent;
class UStaticMeshComponent;
class UGameplayEffect;

UCLASS()
class CODEXGAME_API ACodexLSGASTestTarget : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACodexLSGASTestTarget();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;

private:
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Test Target")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Test Target")
	TObjectPtr<UStaticMeshComponent> VisibleMesh;

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|GAS")
	TObjectPtr<UCodexLSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCodexLSAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	FDelegateHandle HealthChangedDelegateHandle;
};
