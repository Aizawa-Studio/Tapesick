// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/TapesickGameMode.h"

#include "Core/TapesickGameState.h"
#include "Core/TapesickPlayerController.h"

ATapesickGameMode::ATapesickGameMode()
{
	// GameState по “« идЄт без BP-обЄртки Ч задаЄм классом в C++.
	GameStateClass = ATapesickGameState::StaticClass();

	// BP_TapesickGameMode переопределит это на BP_TapesickPlayerController;
	// значение нужно как безопасный дефолт дл€ C++-спавна.
	PlayerControllerClass = ATapesickPlayerController::StaticClass();
}
