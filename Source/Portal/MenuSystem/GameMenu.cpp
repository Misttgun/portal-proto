// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMenu.h"

#include "MenuInterface.h"
#include "Components/Button.h"

bool UGameMenu::Initialize()
{
	if (Super::Initialize() == false)
		return false;

	if (ensure(ResumeButton != nullptr))
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UGameMenu::ResumeGame);
	
	if (ensure(QuitButton != nullptr))
		QuitButton->OnClicked.AddUniqueDynamic(this, &UGameMenu::QuitGame);

	return true;
}

void UGameMenu::ResumeGame()
{
	TearDown();
}

void UGameMenu::QuitGame()
{
	if (MenuInterface == nullptr)
		return;

	MenuInterface->LoadMainMenu();
}
