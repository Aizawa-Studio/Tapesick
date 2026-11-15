// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/TapesickCharacter.h"

ATapesickCharacter::ATapesickCharacter()
{
	// Tick is driven by CharacterMovementComponent; the character itself does not need it.
	PrimaryActorTick.bCanEverTick = false;
}

void ATapesickCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ATapesickCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
