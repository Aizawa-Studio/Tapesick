// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TapesickCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

/** Логическое состояние движения: источник для UI, анимаций и будущего шума для AI. */
UENUM(BlueprintType)
enum class ETapesickMovementState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Walk		UMETA(DisplayName = "Walk"),
	Crouch		UMETA(DisplayName = "Crouch"),
	Sprint		UMETA(DisplayName = "Sprint"),
	Exhausted	UMETA(DisplayName = "Exhausted")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTapesickMovementStateChanged, ETapesickMovementState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTapesickStaminaChanged, float, CurrentStamina, float, MaxStamina);

/**
 * Игрок Tapesick: вид от первого лица, присед, спринт со стаминой, без прыжка.
 * Вся математика движения/стамины — здесь; ассеты ввода и визуал — в BP.
 */
UCLASS()
class TAPESICK_API ATapesickCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATapesickCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Текущее логическое состояние движения. */
	UFUNCTION(BlueprintPure, Category = "Tapesick|Player")
	ETapesickMovementState GetMovementState() const { return MovementState; }

	/** Текущая стамина в единицах 0..StaminaMax. */
	UFUNCTION(BlueprintPure, Category = "Tapesick|Player")
	float GetStamina() const { return Stamina; }

	/** Стамина 0..1 — удобно для Progress Bar. */
	UFUNCTION(BlueprintPure, Category = "Tapesick|Player")
	float GetStaminaNormalized() const { return StaminaMax > 0.f ? Stamina / StaminaMax : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Tapesick|Player")
	bool IsExhausted() const { return bExhausted; }

	/** Для UI: смена состояния. */
	UPROPERTY(BlueprintAssignable, Category = "Tapesick|Player")
	FTapesickMovementStateChanged OnMovementStateChanged;

	/** Для UI: стамина изменилась. */
	UPROPERTY(BlueprintAssignable, Category = "Tapesick|Player")
	FTapesickStaminaChanged OnStaminaChanged;

protected:
	virtual void BeginPlay() override;

	// ------------------------------------------------------------------
	// Настройка: локомоция
	// ------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Locomotion", meta = (ClampMin = "0.0", Units = "cm/s"))
	float WalkSpeed = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Locomotion", meta = (ClampMin = "0.0", Units = "cm/s"))
	float CrouchSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Locomotion", meta = (ClampMin = "0.0", Units = "cm/s"))
	float SprintSpeed = 340.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Locomotion", meta = (ClampMin = "0.0", Units = "cm/s"))
	float ExhaustedSpeed = 120.f;

	/** false — присед по удержанию (как в классических survival horror), true — переключение по нажатию. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Locomotion")
	bool bCrouchToggle = false;

	/**
	 * Минимальная проекция нормализованного ввода на «вперёд», при которой разрешён спринт.
	 *  0.0  — разрешено всё, кроме движения назад (боком и вперёд можно);
	 *  0.64 — только вперёд (cos 50°), как было раньше;
	 * -1.0  — любое направление, включая назад.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Locomotion", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float SprintMinForwardDot = 0.f;

	// ------------------------------------------------------------------
	// Настройка: камера (высоты — от ступней, см)
	// ------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Camera", meta = (ClampMin = "0.0", Units = "cm"))
	float CameraHeightStanding = 165.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Camera", meta = (ClampMin = "0.0", Units = "cm"))
	float CameraCrouched = 105.f;

	/** Время бленда камеры при приседе, с. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Camera", meta = (ClampMin = "0.0", Units = "s"))
	float CameraBlendTime = 0.25f;

	// ------------------------------------------------------------------
	// Настройка: стамина
	// ------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Stamina", meta = (ClampMin = "1.0"))
	float StaminaMax = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Stamina", meta = (ClampMin = "0.0"))
	float StaminaDrainPerSecond = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Stamina", meta = (ClampMin = "0.0", Units = "s"))
	float StaminaRegenDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Stamina", meta = (ClampMin = "0.0"))
	float StaminaRegenWalking = 12.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Stamina", meta = (ClampMin = "0.0"))
	float StaminaRegenIdle = 20.f;

	/** Порог, после которого снимается блок спринта (Exhausted). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Stamina", meta = (ClampMin = "0.0"))
	float ExhaustedRecoverThreshold = 40.f;

	// ------------------------------------------------------------------
	// Настройка: ввод (ассеты создаются в BP, назначаются в Class Defaults)
	// ------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Input")
	TObjectPtr<UInputAction> CrouchAction;

	// ------------------------------------------------------------------
	// События для BP
	// ------------------------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = "Tapesick|Player", meta = (DisplayName = "On Movement State Changed"))
	void BP_OnMovementStateChanged(ETapesickMovementState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tapesick|Player", meta = (DisplayName = "On Stamina Changed"))
	void BP_OnStaminaChanged(float CurrentStamina, float MaxStamina);

	// ------------------------------------------------------------------
	// Компоненты камеры (создаются в C++, видны в BP)
	// ------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tapesick|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tapesick|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

private:
	// --- Обработчики Enhanced Input ---
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SprintStarted();
	void SprintCompleted();
	void CrouchStarted();
	void CrouchCompleted();

	// --- Внутренняя логика ---
	void UpdateStamina(float DeltaSeconds);
	void UpdateMovementState();
	void UpdateCameraHeight(float DeltaSeconds);
	void SetMovementState(ETapesickMovementState NewState);
	void BroadcastStamina(bool bForce = false);

	bool IsMoving() const;
	bool CanSprint() const;

	// --- Состояние ---
	FVector2D LastMoveInput = FVector2D::ZeroVector;

	float Stamina = 0.f;
	float LastBroadcastStamina = -1.f;
	float TimeSinceLastStaminaDrain = 0.f;

	/** 0 = стоим, 1 = присели; блендится за CameraBlendTime. */
	float CrouchCameraAlpha = 0.f;

	bool bSprintHeld = false;
	bool bExhausted = false;

	ETapesickMovementState MovementState = ETapesickMovementState::Idle;
};
