// Copyright (c) 2025 Maurel Sagbo


#include "PGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "MenuSystem/GameMenu.h"
#include "MenuSystem/MainMenu.h"

void UPGameInstance::LoadMainMenuWidget(const TSubclassOf<UUserWidget> MainMenuClass)
{
	if (ensure(MainMenuClass != nullptr) == false)
		return;
	
	MainMenu = CreateWidget<UMainMenu>(this, *MainMenuClass);

	if (ensure(MainMenu != nullptr) == false)
		return;
	
	MainMenu->Setup();
	MainMenu->SetMenuInterface(this);
}

void UPGameInstance::LoadGameMenuWidget(const TSubclassOf<UUserWidget> GameMenuClass)
{
	if (ensure(GameMenuClass != nullptr) == false)
		return;
	
	GameMenu = CreateWidget<UGameMenu>(this, *GameMenuClass);

	if (ensure(GameMenu != nullptr) == false)
		return;
	
	GameMenu->Setup();
	GameMenu->SetMenuInterface(this);
}

void UPGameInstance::PlayGame()
{
	if (MainMenu != nullptr)
		MainMenu->TearDown();
	
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (ensure(PlayerController != nullptr) == false)
		return;
	
	PlayerController->ClientTravel("/Game/Portal/Maps/PlaygroundMap", TRAVEL_Absolute);
}

void UPGameInstance::LoadMainMenu()
{
	if (GameMenu != nullptr)
		GameMenu->TearDown();

	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (ensure(PlayerController != nullptr) == false)
		return;
	
	PlayerController->ClientTravel("/Game/MenuSystem/MainMenuMap", TRAVEL_Absolute);
}

void UPGameInstance::QuitGame()
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (ensure(PlayerController != nullptr) == false)
		return;

	PlayerController->ConsoleCommand("quit");
}
