// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Player/CodexLSPlayerCharacter.h"

#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/GAS/CodexLSAbilitySystemComponent.h"
#include "AgentGameTest/Codex/GAS/CodexLSGameplayAbilities.h"
#include "AgentGameTest/Codex/GAS/CodexLSGameplayEffects.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Math/RotationMatrix.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* PlayerMappingContextPath =
		TEXT("/Game/AgentGameTest/Codex/Input/IMC_Player.IMC_Player");
	const TCHAR* MoveActionPath =
		TEXT("/Game/AgentGameTest/Codex/Input/IA_Move.IA_Move");
	const TCHAR* PrimaryAttackActionPath =
		TEXT("/Game/AgentGameTest/Codex/Input/IA_PrimaryAttack.IA_PrimaryAttack");
	const TCHAR* DashActionPath =
		TEXT("/Game/AgentGameTest/Codex/Input/IA_Dash.IA_Dash");
}

ACodexLSPlayerCharacter::ACodexLSPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = 500.0f;
	Movement->BrakingDecelerationWalking = 1800.0f;
	Movement->GroundFriction = 8.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1100.0f;
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	TestVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TestVisual"));
	TestVisual->SetupAttachment(RootComponent);
	TestVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestVisual->SetRelativeLocation(FVector(0.0f, 0.0f, -35.0f));
	TestVisual->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.1f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlayerMesh(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (PlayerMesh.Succeeded())
	{
		TestVisual->SetStaticMesh(PlayerMesh.Object);
	}

	AimArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("AimArrow"));
	AimArrow->SetupAttachment(RootComponent);
	AimArrow->SetRelativeLocation(FVector(45.0f, 0.0f, 0.0f));
	AimArrow->ArrowColor = FColor::Cyan;
	AimArrow->ArrowSize = 1.5f;
	AimArrow->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PrimaryAttackSystemFinder(
		TEXT("/Game/AgentGameTest/Codex/VFX/Player/NS_Player_Attack_Codex.NS_Player_Attack_Codex"));
	PrimaryAttackSystem = PrimaryAttackSystemFinder.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> WorldImpactSystemFinder(
		TEXT("/Game/AgentGameTest/Codex/VFX/Environment/NS_World_Impact_Codex.NS_World_Impact_Codex"));
	WorldImpactSystem = WorldImpactSystemFinder.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DashSystemFinder(
		TEXT("/Game/AgentGameTest/Codex/VFX/Player/NS_Player_Dash_Codex.NS_Player_Dash_Codex"));
	DashSystem = DashSystemFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> PrimaryAttackSoundFinder(
		TEXT("/Game/AgentGameTest/Codex/Audio/Player/S_Player_Attack_Codex.S_Player_Attack_Codex"));
	PrimaryAttackSound = PrimaryAttackSoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> DashSoundFinder(
		TEXT("/Game/AgentGameTest/Codex/Audio/Player/S_Player_Dash_Codex.S_Player_Dash_Codex"));
	DashSound = DashSoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> DamageSoundFinder(
		TEXT("/Game/AgentGameTest/Codex/Audio/Player/S_Player_Damage_Codex.S_Player_Damage_Codex"));
	DamageSound = DamageSoundFinder.Object;

	DefaultAttributesEffect = UCodexLSGE_DefaultAttributes::StaticClass();
	DefaultAbilities.Add(UCodexLSGA_PrimaryAttack::StaticClass());
	DefaultAbilities.Add(UCodexLSGA_Dash::StaticClass());
}

void ACodexLSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (VisualMaterial)
	{
		TestVisual->SetMaterial(0, VisualMaterial);
	}

	LoadInputAssets();
	ApplyInputMappingContext();
	AimWorldPosition = GetActorLocation() + AimDirection * 500.0f;
}

void ACodexLSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMouseAim(DeltaSeconds);
}

void ACodexLSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	LoadInputAssets();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput || !MoveAction || !PrimaryAttackAction || !DashAction)
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("Enhanced Input setup failed | Component=%s Move=%s Attack=%s Dash=%s"),
			*GetNameSafe(EnhancedInput), *GetNameSafe(MoveAction),
			*GetNameSafe(PrimaryAttackAction), *GetNameSafe(DashAction));
		return;
	}

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMove);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::HandleMoveCompleted);
	EnhancedInput->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this, &ThisClass::HandlePrimaryAttackPressed);
	EnhancedInput->BindAction(PrimaryAttackAction, ETriggerEvent::Completed, this, &ThisClass::HandlePrimaryAttackReleased);
	EnhancedInput->BindAction(DashAction, ETriggerEvent::Started, this, &ThisClass::HandleDashPressed);
	EnhancedInput->BindAction(DashAction, ETriggerEvent::Completed, this, &ThisClass::HandleDashReleased);

	UE_LOG(LogCodexLastStand, Log, TEXT("Enhanced Input Bound: Move, PrimaryAttack, Dash"));
}

void ACodexLSPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
	ApplyInputMappingContext();
}

void ACodexLSPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

void ACodexLSPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (HealthChangedDelegateHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UCodexLSAttributeSet::GetHealthAttribute())
				.Remove(HealthChangedDelegateHandle);
			HealthChangedDelegateHandle.Reset();
		}

		ASC->CancelAllAbilities();
		if (ASC->GetAvatarActor() == this)
		{
			ASC->ClearActorInfo();
		}
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* ACodexLSPlayerCharacter::GetAbilitySystemComponent() const
{
	const ACodexLSPlayerState* CodexPlayerState = GetPlayerState<ACodexLSPlayerState>();
	return CodexPlayerState ? CodexPlayerState->GetAbilitySystemComponent() : nullptr;
}

bool ACodexLSPlayerCharacter::TracePrimaryAttack(float Range, FHitResult& OutHitResult) const
{
	// Start inside the ignored player capsule so point-blank enemies cannot sit
	// behind the muzzle offset (notably the smaller Runner collision capsule).
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + AimDirection * (Range + 65.0f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CodexLSPrimaryAttack), true, this);
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	return bHit;
}

void ACodexLSPlayerCharacter::PlayPrimaryAttackFeedback(
	const FHitResult& HitResult, bool bHit, bool bHitGameplayTarget)
{
	const FVector MuzzleLocation = GetActorLocation() + AimDirection * 72.0f + FVector(0.0f, 0.0f, 20.0f);
	const FRotator AttackRotation = AimDirection.Rotation();

	if (PrimaryAttackSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, PrimaryAttackSystem, MuzzleLocation, AttackRotation,
			FVector(1.0f), true, true, ENCPoolMethod::AutoRelease, true);
	}

	if (PrimaryAttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PrimaryAttackSound, MuzzleLocation, 0.58f, 1.0f);
	}

	if (bHit && !bHitGameplayTarget && WorldImpactSystem)
	{
		const FVector ImpactNormal = HitResult.ImpactNormal.IsNearlyZero()
			? FVector::UpVector : HitResult.ImpactNormal;
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, WorldImpactSystem, HitResult.ImpactPoint, ImpactNormal.Rotation(),
			FVector(1.0f), true, true, ENCPoolMethod::AutoRelease, true);
	}
}

FVector ACodexLSPlayerCharacter::PerformDash(float DashSpeed)
{
	FVector DashDirection =
		LastMovementWorldDirection.IsNearlyZero() ? AimDirection : LastMovementWorldDirection;
	DashDirection.Z = 0.0f;
	DashDirection.Normalize();

	LaunchCharacter(DashDirection * DashSpeed, true, false);
	return DashDirection;
}

void ACodexLSPlayerCharacter::PlayDashFeedback(const FVector& DashDirection, bool bEnding)
{
	FVector FlatDirection = DashDirection.GetSafeNormal2D();
	if (FlatDirection.IsNearlyZero())
	{
		FlatDirection = AimDirection;
	}

	if (DashSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, DashSystem, GetActorLocation() + FVector(0.0f, 0.0f, 15.0f),
			(-FlatDirection).Rotation(), bEnding ? FVector(0.72f) : FVector(1.0f),
			true, true, ENCPoolMethod::AutoRelease, true);
	}

	if (!bEnding && DashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DashSound, GetActorLocation(), 0.62f, 1.0f);
	}
}

void ACodexLSPlayerCharacter::StopDashMovement()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
}

void ACodexLSPlayerCharacter::SetGameplayInputEnabled(bool bEnabled)
{
	bGameplayInputEnabled = bEnabled && !bDead;

	if (!bGameplayInputEnabled)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			ASC->CancelAllAbilities();
		}

		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_PLAYER_INPUT Enabled=%s Dead=%s"),
		bGameplayInputEnabled ? TEXT("true") : TEXT("false"),
		bDead ? TEXT("true") : TEXT("false"));
}

void ACodexLSPlayerCharacter::InitializeAbilitySystem()
{
	ACodexLSPlayerState* CodexPlayerState = GetPlayerState<ACodexLSPlayerState>();
	if (!CodexPlayerState)
	{
		return;
	}

	CodexPlayerState->InitializeAbilitySystem(this);
	if (!HealthChangedDelegateHandle.IsValid())
	{
		HealthChangedDelegateHandle =
			CodexPlayerState->GetCodexAbilitySystemComponent()
				->GetGameplayAttributeValueChangeDelegate(UCodexLSAttributeSet::GetHealthAttribute())
				.AddUObject(this, &ThisClass::HandleHealthChanged);
	}

	if (HasAuthority())
	{
		CodexPlayerState->GrantAbilities(DefaultAbilities);
		CodexPlayerState->ApplyDefaultAttributes(DefaultAttributesEffect);
	}
}

void ACodexLSPlayerCharacter::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!bDead && DamageSound && ChangeData.NewValue < ChangeData.OldValue && ChangeData.OldValue > 0.0f)
	{
		UGameplayStatics::PlaySound2D(this, DamageSound, 0.72f, 1.0f);
	}

	if (bDead || ChangeData.NewValue > 0.0f || ChangeData.OldValue <= 0.0f)
	{
		return;
	}

	bDead = true;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();
		ASC->AddLooseGameplayTag(CodexLSGameplayTags::State_Player_Dead);
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	bGameplayInputEnabled = false;
	OnPlayerDeath.Broadcast(this);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("Player Health Reached Zero | Player=%s DeadTag=present DeathEvent=BroadcastOnce EnemyAttacksWillStop=true"),
		*GetName());
}

void ACodexLSPlayerCharacter::LoadInputAssets()
{
	if (!PlayerMappingContext)
	{
		PlayerMappingContext = LoadObject<UInputMappingContext>(nullptr, PlayerMappingContextPath);
	}
	if (!MoveAction)
	{
		MoveAction = LoadObject<UInputAction>(nullptr, MoveActionPath);
	}
	if (!PrimaryAttackAction)
	{
		PrimaryAttackAction = LoadObject<UInputAction>(nullptr, PrimaryAttackActionPath);
	}
	if (!DashAction)
	{
		DashAction = LoadObject<UInputAction>(nullptr, DashActionPath);
	}
}

void ACodexLSPlayerCharacter::ApplyInputMappingContext()
{
	LoadInputAssets();

	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		InputSubsystem->RemoveMappingContext(PlayerMappingContext);
		InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
		UE_LOG(LogCodexLastStand, Log, TEXT("Input Mapping Context Applied: %s"),
			*PlayerMappingContext->GetName());
	}
}

void ACodexLSPlayerCharacter::UpdateMouseAim(float DeltaSeconds)
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	FVector RayOrigin;
	FVector RayDirection;
	if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		return;
	}

	const FVector RayEnd = RayOrigin + RayDirection * 100000.0f;
	const FPlane AimPlane(GetActorLocation(), FVector::UpVector);
	const FVector PlaneIntersection = FMath::LinePlaneIntersection(RayOrigin, RayEnd, AimPlane);

	FVector NewAimDirection = PlaneIntersection - GetActorLocation();
	NewAimDirection.Z = 0.0f;
	if (!NewAimDirection.Normalize())
	{
		return;
	}

	AimWorldPosition = PlaneIntersection;
	AimDirection = NewAimDirection;
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), AimDirection.Rotation(), DeltaSeconds, 24.0f));
}

void ACodexLSPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	if (!bGameplayInputEnabled || bDead)
	{
		return;
	}

	FVector2D MoveInput = Value.Get<FVector2D>();
	MoveInput = MoveInput.GetClampedToMaxSize(1.0f);

	const FRotator CameraYaw(0.0f, TopDownCamera->GetComponentRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y);

	LastMovementWorldDirection =
		(ForwardDirection * MoveInput.Y + RightDirection * MoveInput.X).GetClampedToMaxSize(1.0f);

	AddMovementInput(ForwardDirection, MoveInput.Y);
	AddMovementInput(RightDirection, MoveInput.X);

	if (!MoveInput.Equals(LastLoggedMoveInput, 0.05f))
	{
		LastLoggedMoveInput = MoveInput;
		UE_LOG(LogCodexLastStand, Verbose,
			TEXT("Move Input: X=%.2f Y=%.2f | WorldDirection=(%.2f, %.2f)"),
			MoveInput.X, MoveInput.Y, LastMovementWorldDirection.X, LastMovementWorldDirection.Y);
	}
}

void ACodexLSPlayerCharacter::HandleMoveCompleted(const FInputActionValue& Value)
{
	LastMovementWorldDirection = FVector::ZeroVector;
	LastLoggedMoveInput = FVector2D::ZeroVector;
}

void ACodexLSPlayerCharacter::HandlePrimaryAttackPressed()
{
	if (!bGameplayInputEnabled || bDead)
	{
		return;
	}

	if (UCodexLSAbilitySystemComponent* ASC =
		Cast<UCodexLSAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		ASC->AbilityInputTagPressed(CodexLSGameplayTags::InputTag_Ability_PrimaryAttack);
	}
}

void ACodexLSPlayerCharacter::HandlePrimaryAttackReleased()
{
	if (UCodexLSAbilitySystemComponent* ASC =
		Cast<UCodexLSAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		ASC->AbilityInputTagReleased(CodexLSGameplayTags::InputTag_Ability_PrimaryAttack);
	}
}

void ACodexLSPlayerCharacter::HandleDashPressed()
{
	if (!bGameplayInputEnabled || bDead)
	{
		return;
	}

	if (UCodexLSAbilitySystemComponent* ASC =
		Cast<UCodexLSAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		ASC->AbilityInputTagPressed(CodexLSGameplayTags::InputTag_Ability_Dash);
	}
}

void ACodexLSPlayerCharacter::HandleDashReleased()
{
	if (UCodexLSAbilitySystemComponent* ASC =
		Cast<UCodexLSAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		ASC->AbilityInputTagReleased(CodexLSGameplayTags::InputTag_Ability_Dash);
	}
}
