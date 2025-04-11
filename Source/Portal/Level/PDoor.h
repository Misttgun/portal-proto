// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDoor.generated.h"

class APDoorTrigger;

UCLASS()
class PORTAL_API APDoor : public AActor
{
	GENERATED_BODY()

public:
	APDoor();

	virtual void Tick(float DeltaTime) override;

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OpenDoor();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void CloseDoor();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> RootComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LeftDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> RightDoorMesh;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Door", meta = (AllowPrivateAccess = "true"))
	TArray<APDoorTrigger*> Triggers;
	
	bool bIsOpen;
};
