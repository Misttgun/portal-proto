// Copyright (c) 2025 Maurel Sagbo


#include "PPortal.h"

#include "PPortalWall.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/GameplayStatics.h"
#include "Portal/Helpers/PPortalHelper.h"


APPortal::APPortal() : CurrentWall(nullptr), bPortalLeft(true), PortalRenderScale(1.0f), LinkedPortal(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComp->SetupAttachment(RootComponent);

	PortalBorderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalBorder"));
	PortalBorderMesh->SetupAttachment(RootComp);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Portal"));
	PortalMesh->SetupAttachment(RootComp);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComp);

	BackFacingComp = CreateDefaultSubobject<UArrowComponent>(TEXT("BackFacing"));
	BackFacingComp->SetupAttachment(RootComp);
	BackFacingComp->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f)); // Rotate so it looks into the portal like a player would
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

void APPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsValid(LinkedPortal) == false)
		return;

	USceneCaptureComponent2D* LinkedSceneCapture = LinkedPortal->SceneCapture;

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	UPPortalHelper::ResizeRenderTarget(LinkedSceneCapture->TextureTarget, ViewportSize.X * PortalRenderScale, ViewportSize.Y * PortalRenderScale);

	const APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	const FTransform CameraTransform = PlayerCameraManager->GetTransformComponent()->GetComponentTransform();

	const float DistanceToCamera = FVector::Distance(CameraTransform.GetLocation(), GetActorLocation()) + 1.0f;
	LinkedSceneCapture->CustomNearClippingPlane = DistanceToCamera;

	const FTransform BackFacingTransform = BackFacingComp->GetComponentTransform();
	const FTransform NewTransform =  CameraTransform.GetRelativeTransform(BackFacingTransform);
	LinkedSceneCapture->SetRelativeLocationAndRotation(NewTransform.GetLocation(), NewTransform.GetRotation());
}

void APPortal::Init(const bool bIsLeftPortal)
{
	bPortalLeft = bIsLeftPortal;
}

void APPortal::LinkPortal(APPortal* OtherPortal)
{
	if (LinkedPortal != nullptr && LinkedPortal == OtherPortal)
		return;
	
	if (IsValid(OtherPortal) == false)
	{
		PortalMesh->SetMaterial(0, DefaultPortalMaterial);
		return;
	}

	LinkedPortal = OtherPortal;

	if (bPortalLeft)
		PortalMesh->SetMaterial(0, RightPortalRTMaterial);
	else
		PortalMesh->SetMaterial(0, LeftPortalRTMaterial);
}
