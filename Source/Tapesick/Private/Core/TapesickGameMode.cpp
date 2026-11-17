// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/TapesickGameMode.h"

#include "Core/TapesickGameState.h"
#include "Core/TapesickPlayerController.h"

ATapesickGameMode::ATapesickGameMode()
{
	// GameState без BP-обёртки: назначаем класс напрямую в C++.
	GameStateClass = ATapesickGameState::StaticClass();

	// BP_TapesickGameMode может переопределить контроллер на BP_TapesickPlayerController.
	// Базовый C++ класс контроллера указываем здесь.
	PlayerControllerClass = ATapesickPlayerController::StaticClass();
}