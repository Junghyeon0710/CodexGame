// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h"
#include "InputActionValue.h"
#include "CodexLSPlayerCharacter.generated.h"

class UArrowComponent;
class ACodexLSPlayerCharacter;
class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
class USpringArmComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FCodexLSPlayerDeathSignature, ACodexLSPlayerCharacter*, Player);

UCLASS()
class CODEXGAME_API ACodexLSPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACodexLSPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	FVector GetAimDirection() const { return AimDirection; }
	FVector GetAimWorldPosition() const { return AimWorldPosition; }
	bool TracePrimaryAttack(float Range, FHitResult& OutHitResult) const;
	FVector PerformDash(float DashSpeed);
	void StopDashMovement();
	bool IsDead() const { return bDead; }
	bool IsGameplayInputEnabled() const { return bGameplayInputEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Player")
	void SetGameplayInputEnabled(bool bEnabled);

	UPROPERTY(BlueprintAssignable, Category = "Last Stand|Player")
	FCodexLSPlayerDeathSignature OnPlayerDeath;

protected:
	virtual void BeginPlay() override;

private:
	void InitializeAbilitySystem();
	void LoadInputAssets();
	void ApplyInputMappingContext();
	void UpdateMouseAim(float DeltaSeconds);
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);

	void HandleMove(const FInputActionValue& Value);
	void HandleMoveCompleted(const FInputActionValue& Value);
	void HandlePrimaryAttackPressed();
	void HandlePrimaryAttackReleased();
	void HandleDashPressed();
	void HandleDashReleased();

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Visual")
	TObjectPtr<UStaticMeshComponent> TestVisual;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Visual")
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Last Stand|Visual")
	TObjectPtr<UArrowComponent> AimArrow;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PrimaryAttackAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	FVector AimDirection = FVector::ForwardVector;
	FVector AimWorldPosition = FVector::ZeroVector;
	FVector LastMovementWorldDirection = FVector::ZeroVector;
	FVector2D LastLoggedMoveInput = FVector2D::ZeroVector;
	FDelegateHandle HealthChangedDelegateHandle;
	bool bGameplayInputEnabled = true;
	bool bDead = false;
};
