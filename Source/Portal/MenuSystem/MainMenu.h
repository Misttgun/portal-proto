// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseMenu.h"
#include "Blueprint/UserWidget.h"
#include "MainMenu.generated.h"

class UEditableTextBox;
class UWidgetSwitcher;
class UButton;

/**
 * 
 */
UCLASS()
class PORTAL_API UMainMenu : public UBaseMenu
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;

private:
	UFUNCTION()
	void PlayGame();

	UFUNCTION()
	void QuitGame();

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UButton> PlayButton;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UWidget> MainMenu;
};
