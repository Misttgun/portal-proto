// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseMenu.generated.h"

class IMenuInterface;
/**
 * 
 */
UCLASS()
class PORTAL_API UBaseMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMenuInterface(IMenuInterface* NewMenuInterface);

	void Setup();
	void TearDown();

protected:
	IMenuInterface* MenuInterface;
};
