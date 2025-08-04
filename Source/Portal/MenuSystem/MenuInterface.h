// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MenuInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UMenuInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PORTAL_API IMenuInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION()
	virtual void PlayGame() = 0;

	UFUNCTION()
	virtual void LoadMainMenu() = 0;

	UFUNCTION()
	virtual void QuitGame() = 0;
};
