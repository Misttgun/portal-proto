// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseMenu.h"

void UBaseMenu::Setup()
{
	AddToViewport();

	const auto World = GetWorld();
	ensure(World != nullptr);

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (ensure(PlayerController != nullptr) == false)
		return;

	FInputModeUIOnly InputModeData;
	InputModeData.SetWidgetToFocus(TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(InputModeData);
	PlayerController->bShowMouseCursor = true;
}

void UBaseMenu::TearDown()
{
	const auto World = GetWorld();
	ensure(World != nullptr);

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (ensure(PlayerController != nullptr) == false)
		return;

	const FInputModeGameOnly InputModeData;

	PlayerController->SetInputMode(InputModeData);
	PlayerController->bShowMouseCursor = false;

	RemoveFromParent();
}

void UBaseMenu::SetMenuInterface(IMenuInterface* NewMenuInterface)
{
	if (NewMenuInterface == nullptr)
		return;

	MenuInterface = NewMenuInterface;
}