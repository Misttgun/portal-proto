// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenu.h"

#include "MenuInterface.h"
#include "Components/Button.h"

bool UMainMenu::Initialize()
{
	if (Super::Initialize() == false)
		return false;

	if (ensure(PlayButton != nullptr))
		PlayButton->OnClicked.AddUniqueDynamic(this, &UMainMenu::PlayGame);

	if (ensure(QuitButton != nullptr))
		QuitButton->OnClicked.AddUniqueDynamic(this, &UMainMenu::QuitGame);

	return true;
}

void UMainMenu::PlayGame()
{
	if (MenuInterface == nullptr)
		return;

	MenuInterface->PlayGame();
}

void UMainMenu::QuitGame()
{
	if (MenuInterface == nullptr)
		return;

	MenuInterface->QuitGame();
}
