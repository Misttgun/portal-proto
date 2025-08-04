// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseMenu.h"
#include "GameMenu.generated.h"

class UButton;

/**
 * 
 */
UCLASS()
class PORTAL_API UGameMenu : public UBaseMenu
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

private:
	UFUNCTION()
	void ResumeGame();

	UFUNCTION()
	void QuitGame();
	
};
