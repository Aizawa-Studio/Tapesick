// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TapesickCharacter.generated.h"

UCLASS()
class TAPESICK_API ATapesickCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATapesickCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
