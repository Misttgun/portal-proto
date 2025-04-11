// Copyright (c) 2025 Maurel Sagbo


#include "PDoorTrigger.h"

#include "Components/BoxComponent.h"


APDoorTrigger::APDoorTrigger() : bIsActivated(false)
{
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	TriggerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TriggerPlate"));
	TriggerMesh->SetupAttachment(BaseMesh);
	
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	TriggerBox->SetupAttachment(TriggerMesh);
}

