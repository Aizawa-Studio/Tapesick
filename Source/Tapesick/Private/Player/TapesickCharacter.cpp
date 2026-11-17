// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/TapesickCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Player/InteractableInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogTapesickCharacter, Log, All);

namespace TapesickCharacterConstants
{
	/** Порог «персонаж действительно движется», см/с. */
	constexpr float MovingSpeedThreshold = 10.f;

	/** Гистерезис, чтобы не спамить UI одинаковыми значениями стамины. */
	constexpr float StaminaBroadcastEpsilon = 0.01f;

	/** Капсула игрока по ТЗ. */
	constexpr float CapsuleRadius = 34.f;
	constexpr float CapsuleHalfHeight = 90.f;
	constexpr float CrouchedHalfHeight = 60.f;
}

ATapesickCharacter::ATapesickCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(
		TapesickCharacterConstants::CapsuleRadius,
		TapesickCharacterConstants::CapsuleHalfHeight);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = false;

	FNavAgentProperties& NavAgent = Movement->GetNavAgentPropertiesRef();
	NavAgent.bCanCrouch = true;
	NavAgent.bCanJump = false;

	Movement->SetCrouchedHalfHeight(TapesickCharacterConstants::CrouchedHalfHeight);
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	Movement->MaxStepHeight = 30.f;
	Movement->SetWalkableFloorAngle(38.f);
	Movement->JumpZVelocity = 0.f;
	Movement->AirControl = 0.f;

	// SpringArm без длины и коллизии = первое лицо.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetRelativeLocation(FVector(
		0.f,
		0.f,
		CameraHeightStanding - TapesickCharacterConstants::CapsuleHalfHeight));
	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 85.f;
}

// ---------------------------------------------------------------------------
// Жизненный цикл
// ---------------------------------------------------------------------------

void ATapesickCharacter::BeginPlay()
{
	Super::BeginPlay();

	Stamina = StaminaMax;
	TimeSinceLastStaminaDrain = StaminaRegenDelay;

	BroadcastStamina(true);
	SetMovementState(ETapesickMovementState::Idle);
	UpdateCameraHeight(0.f);
}

void ATapesickCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateStamina(DeltaSeconds);
	UpdateMovementState();
	UpdateCameraHeight(DeltaSeconds);
	UpdateInteractionTarget(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Ввод
// ---------------------------------------------------------------------------

void ATapesickCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			else
			{
				UE_LOG(LogTapesickCharacter, Warning,
					TEXT("%s: DefaultMappingContext is not set (BP_TapesickCharacter -> Class Defaults -> Tapesick|Input)."),
					*GetName());
			}
		}
	}

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTapesickCharacter, Error,
			TEXT("%s: EnhancedInputComponent not found. Check DefaultInputComponentClass in DefaultInput.ini."),
			*GetName());
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATapesickCharacter::Move);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATapesickCharacter::Look);
	}
	if (SprintAction)
	{
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &ATapesickCharacter::SprintStarted);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &ATapesickCharacter::SprintCompleted);
	}
	if (CrouchAction)
	{
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &ATapesickCharacter::CrouchStarted);
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ATapesickCharacter::CrouchCompleted);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ATapesickCharacter::InteractPressed);
	}
}

void ATapesickCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	LastMoveInput = MovementVector;

	if (!Controller)
	{
		return;
	}

	AddMovementInput(GetActorForwardVector(), MovementVector.Y);
	AddMovementInput(GetActorRightVector(), MovementVector.X);
}

void ATapesickCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void ATapesickCharacter::SprintStarted()
{
	bSprintHeld = true;
}

void ATapesickCharacter::SprintCompleted()
{
	bSprintHeld = false;
}

void ATapesickCharacter::CrouchStarted()
{
	if (bCrouchToggle)
	{
		if (IsCrouched())
		{
			UnCrouch();
		}
		else
		{
			Crouch();
		}
	}
	else
	{
		Crouch();
	}
}

void ATapesickCharacter::CrouchCompleted()
{
	if (!bCrouchToggle)
	{
		UnCrouch();
	}
}

// ---------------------------------------------------------------------------
// Стамина
// ---------------------------------------------------------------------------

bool ATapesickCharacter::IsMoving() const
{
	return GetVelocity().SizeSquared2D() >
		FMath::Square(TapesickCharacterConstants::MovingSpeedThreshold);
}

bool ATapesickCharacter::CanSprint() const
{
	if (!bSprintHeld || IsCrouched() || bExhausted || Stamina <= 0.f)
	{
		return false;
	}

	const FVector2D MoveInput = LastMoveInput.GetSafeNormal();
	if (MoveInput.IsNearlyZero())
	{
		return false;
	}

	return MoveInput.Y >= SprintMinForwardDot;
}

void ATapesickCharacter::UpdateStamina(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f)
	{
		return;
	}

	const float RecoverThreshold = FMath::Min(ExhaustedRecoverThreshold, StaminaMax);
	const bool bSprinting = CanSprint() && IsMoving();

	if (bSprinting)
	{
		Stamina = FMath::Max(0.f, Stamina - StaminaDrainPerSecond * DeltaSeconds);
		TimeSinceLastStaminaDrain = 0.f;

		if (Stamina <= 0.f && !bExhausted)
		{
			bExhausted = true;
			UE_LOG(LogTapesickCharacter, Log,
				TEXT("%s: exhausted, sprint blocked until stamina >= %.0f."),
				*GetName(), RecoverThreshold);
		}
	}
	else
	{
		TimeSinceLastStaminaDrain += DeltaSeconds;

		if (TimeSinceLastStaminaDrain >= StaminaRegenDelay && Stamina < StaminaMax)
		{
			const float RegenRate =
				(IsCrouched() || !IsMoving()) ? StaminaRegenIdle : StaminaRegenWalking;

			Stamina = FMath::Min(StaminaMax, Stamina + RegenRate * DeltaSeconds);
		}
	}

	if (bExhausted && Stamina >= RecoverThreshold)
	{
		bExhausted = false;
		UE_LOG(LogTapesickCharacter, Log, TEXT("%s: stamina recovered, sprint unlocked."), *GetName());
	}

	BroadcastStamina();
}

void ATapesickCharacter::BroadcastStamina(bool bForce)
{
	if (!bForce && FMath::IsNearlyEqual(
		Stamina,
		LastBroadcastStamina,
		TapesickCharacterConstants::StaminaBroadcastEpsilon))
	{
		return;
	}

	LastBroadcastStamina = Stamina;
	OnStaminaChanged.Broadcast(Stamina, StaminaMax);
	BP_OnStaminaChanged(Stamina, StaminaMax);
}

// ---------------------------------------------------------------------------
// Состояние и скорость
// ---------------------------------------------------------------------------

void ATapesickCharacter::UpdateMovementState()
{
	const bool bMoving = IsMoving();
	const bool bSprintAllowed = CanSprint() && bMoving;

	ETapesickMovementState NewState;
	float TargetSpeed;

	if (IsCrouched())
	{
		NewState = ETapesickMovementState::Crouch;
		TargetSpeed = CrouchSpeed;
	}
	else if (bSprintAllowed)
	{
		NewState = ETapesickMovementState::Sprint;
		TargetSpeed = SprintSpeed;
	}
	else if (bExhausted)
	{
		NewState = ETapesickMovementState::Exhausted;
		TargetSpeed = ExhaustedSpeed;
	}
	else
	{
		NewState = bMoving ? ETapesickMovementState::Walk : ETapesickMovementState::Idle;
		TargetSpeed = WalkSpeed;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = TargetSpeed;
		Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	}

	SetMovementState(NewState);
}

void ATapesickCharacter::SetMovementState(ETapesickMovementState NewState)
{
	if (MovementState == NewState)
	{
		return;
	}

	MovementState = NewState;

	UE_LOG(LogTapesickCharacter, Log, TEXT("%s: movement state -> %s"),
		*GetName(),
		*StaticEnum<ETapesickMovementState>()->GetNameStringByValue(
			static_cast<int64>(MovementState)));

	OnMovementStateChanged.Broadcast(MovementState);
	BP_OnMovementStateChanged(MovementState);
}

// ---------------------------------------------------------------------------
// Камера
// ---------------------------------------------------------------------------

void ATapesickCharacter::UpdateCameraHeight(float DeltaSeconds)
{
	if (!CameraBoom)
	{
		return;
	}

	const float TargetAlpha = IsCrouched() ? 1.f : 0.f;

	if (CameraBlendTime <= KINDA_SMALL_NUMBER)
	{
		CrouchCameraAlpha = TargetAlpha;
	}
	else
	{
		CrouchCameraAlpha = FMath::FInterpConstantTo(
			CrouchCameraAlpha,
			TargetAlpha,
			DeltaSeconds,
			1.f / CameraBlendTime);
	}

	const float EyeHeightFromFeet =
		FMath::Lerp(CameraHeightStanding, CameraCrouched, CrouchCameraAlpha);

	FVector BoomLocation = CameraBoom->GetRelativeLocation();
	BoomLocation.Z = EyeHeightFromFeet - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	CameraBoom->SetRelativeLocation(BoomLocation);
}

// ---------------------------------------------------------------------------
// Взаимодействие
// ---------------------------------------------------------------------------

void ATapesickCharacter::InteractPressed()
{
	TryInteract();
}

void ATapesickCharacter::TryInteract()
{
	// На нажатие выполняем свежий trace, а не используем цель из прошлого кадра.
	AActor* Target = FindInteractionTarget();
	if (!Target)
	{
		return;
	}

	if (!Cast<IInteractableInterface>(Target))
	{
		return;
	}

	if (!IInteractableInterface::Execute_CanInteract(Target, this))
	{
		UE_LOG(LogTapesickCharacter, Verbose,
			TEXT("%s: %s refused interaction (CanInteract = false)."),
			*GetName(), *Target->GetName());
		return;
	}

	UE_LOG(LogTapesickCharacter, Log,
		TEXT("%s: interact with %s."),
		*GetName(), *Target->GetName());

	IInteractableInterface::Execute_Interact(Target, this);
}

AActor* ATapesickCharacter::FindInteractionTarget() const
{
	if (!FollowCamera || !GetWorld())
	{
		return nullptr;
	}

	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + FollowCamera->GetForwardVector() * InteractDistance;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(InteractTraceRadius);

	// Первый блокирующий hit: стена или другой проп перекрывает взаимодействие.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TapesickInteract), false, this);

	FHitResult Hit;
	if (!GetWorld()->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		InteractTraceChannel,
		Shape,
		Params))
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor || !Cast<IInteractableInterface>(HitActor))
	{
		return nullptr;
	}

	return HitActor;
}

void ATapesickCharacter::UpdateInteractionTarget(float DeltaSeconds)
{
	if (InteractTargetUpdateInterval > 0.f)
	{
		TimeSinceInteractionTargetUpdate += DeltaSeconds;

		if (TimeSinceInteractionTargetUpdate < InteractTargetUpdateInterval)
		{
			return;
		}

		TimeSinceInteractionTargetUpdate = 0.f;
	}

	AActor* NewTarget = FindInteractionTarget();

	// Подсказку вычисляем заранее и сохраняем ДО Broadcast:
	// BP-подписчик должен получить текст именно новой цели.
	FText NewPrompt = FText::GetEmpty();

	if (NewTarget && IInteractableInterface::Execute_CanInteract(NewTarget, this))
	{
		NewPrompt = IInteractableInterface::Execute_GetInteractionPrompt(NewTarget);
	}

	const bool bTargetChanged = NewTarget != InteractionTarget.Get();

	InteractionTarget = NewTarget;
	InteractionPrompt = NewPrompt;

	if (!bTargetChanged)
	{
		return;
	}

	UE_LOG(LogTapesickCharacter, Verbose,
		TEXT("%s: interaction target -> %s"),
		*GetName(),
		NewTarget ? *NewTarget->GetName() : TEXT("none"));

	OnInteractionTargetChanged.Broadcast(NewTarget);
	BP_OnInteractionTargetChanged(NewTarget);
}