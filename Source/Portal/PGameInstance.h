// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MenuSystem/MenuInterface.h"
#include "PGameInstance.generated.h"

class UGameMenu;
class UMainMenu;

/**
 * 
 */
UCLASS()
class PORTAL_API UPGameInstance : public UGameInstance, public IMenuInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void LoadMainMenuWidget(TSubclassOf<UUserWidget> MainMenuClass);

	UFUNCTION(BlueprintCallable)
	void LoadGameMenuWidget(TSubclassOf<UUserWidget> GameMenuClass);
	
	UFUNCTION()
	virtual void PlayGame() override;
	
	UFUNCTION()
	virtual void LoadMainMenu() override;

	UFUNCTION()
	virtual void QuitGame() override;

private:
	UPROPERTY()
	TObjectPtr<UMainMenu> MainMenu;

	UPROPERTY()
	TObjectPtr<UGameMenu> GameMenu;
};
