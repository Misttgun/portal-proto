// Copyright (c) 2025 Maurel Sagbo


#include "PDoor.h"

#include "PDoorTrigger.h"


APDoor::APDoor() : bIsOpen(false)
{
	PrimaryActorTick.bCanEverTick = true;
	
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComp->SetupAttachment(RootComponent);
	
	LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoor"));
	LeftDoorMesh->SetupAttachment(RootComp);
	
	RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoor"));
	RightDoorMesh->SetupAttachment(RootComp);
}

void APDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ensureMsgf(Triggers.Num() > 0, TEXT("Door Triggers is empty.")) == false)
		return;
	
	for (const APDoorTrigger* Trigger : Triggers)
	{
		if (Trigger->IsActivated() == false)
		{
			if (bIsOpen)
			{
				bIsOpen = false;
				CloseDoor();
			}
			
			return;
		}
	}

	if (bIsOpen)
		return;

	bIsOpen = true;
	OpenDoor();
}
