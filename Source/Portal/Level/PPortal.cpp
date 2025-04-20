// Copyright (c) 2025 Maurel Sagbo


#include "PPortal.h"

#include "PPortalWall.h"
#include "Components/SceneCaptureComponent2D.h"


APPortal::APPortal() : CurrentWall(nullptr), bPortalLeft(true)
{
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComp->SetupAttachment(RootComponent);

	PortalBorderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalBorder"));
	PortalBorderMesh->SetupAttachment(RootComp);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Portal"));
	PortalMesh->SetupAttachment(RootComp);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComp);
}

void APPortal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PortalBorderMesh != nullptr)
	{
		if (bPortalLeft)
			PortalBorderMesh->SetMaterial(0, LeftPortalMaterial);
		else
			PortalBorderMesh->SetMaterial(0, RightPortalMaterial);
	}

	if (SceneCapture != nullptr)
	{
		if (bPortalLeft)
			SceneCapture->TextureTarget = LeftRenderTarget;
		else
			SceneCapture->TextureTarget = RightRenderTarget;
	}
}

void APPortal::Init(const bool bIsLeftPortal)
{
	bPortalLeft = bIsLeftPortal;
}

