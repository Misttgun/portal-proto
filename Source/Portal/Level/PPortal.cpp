// Copyright (c) 2025 Maurel Sagbo


#include "PPortal.h"

#include "PPortalWall.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Portal/PCharacter.h"
#include "Portal/PPlayerController.h"
#include "Portal/Helpers/PPortalHelper.h"

DEFINE_LOG_CATEGORY(LogPortal);

APPortal::APPortal() : CurrentWall(nullptr), bPortalLeft(true), PortalRenderScale(1.0f), TargetPortal(nullptr), bInitialized(false)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	PortalBorderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalBorder"));
	PortalBorderMesh->SetupAttachment(RootComponent);
	PortalBorderMesh->CastShadow = false;

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Portal"));
	PortalMesh->SetupAttachment(RootComponent);
	PortalMesh->CastShadow = false;

	PortalBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PortalBox"));
	PortalBox->SetUseCCD(true);
	PortalBox->SetupAttachment(RootComponent);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComponent);
	SceneCapture->bEnableClipPlane = true;
	SceneCapture->bUseCustomProjectionMatrix = false;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	//SceneCapture->LODDistanceFactor = 3;
	SceneCapture->TextureTarget = nullptr;
	SceneCapture->CaptureSource = SCS_SceneColorHDRNoAlpha; // Stores Scene Depth in A channel.

	// Add post-physics ticking to this actor
	PhysicsTick.bCanEverTick = true;
	PhysicsTick.Target = this;
	PhysicsTick.TickGroup = TG_PostPhysics;
}

void APPortal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PortalBorderMesh != nullptr)
	{
		if (bPortalLeft)
			PortalBorderMesh->SetMaterial(0, LeftPortalBorderMaterial);
		else
			PortalBorderMesh->SetMaterial(0, RightPortalBorderMaterial);
	}

	// if (SceneCapture != nullptr)
	// {
	// 	if (bPortalLeft)
	// 		SceneCapture->TextureTarget = LeftRenderTarget;
	// 	else
	// 		SceneCapture->TextureTarget = RightRenderTarget;
	// }
}

void APPortal::BeginPlay()
{
	Super::BeginPlay();

	// Save a ref to the player controller and player character
	APPlayerController* PC = Cast<APPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PC != nullptr)
		PlayerController = PC;

	APCharacter* Character = Cast<APCharacter>(PC->GetPawn());
	if (Character != nullptr)
		PlayerCharacter = Character;

	CreatePortalTexture();

	// Register the secondary post-physics tick function in the world on level start
	PhysicsTick.bCanEverTick = true;
	PhysicsTick.RegisterTickFunction(GetWorld()->PersistentLevel);

	bInitialized = true;

	LastPawnPosition = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();

	PrimaryActorTick.SetTickFunctionEnable(true);

	// Delay setup for 1 second
	// PrimaryActorTick.SetTickFunctionEnable(false);
	// FTimerHandle TimerHandle;
	// FTimerDelegate TimerDelegate;
	// TimerDelegate.BindUFunction(this, FName("Setup"));
	// GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 1.0f, false);
}

void APPortal::Setup()
{
	// Register the secondary post-physics tick function in the world on level start
	PhysicsTick.bCanEverTick = true;
	PhysicsTick.RegisterTickFunction(GetWorld()->PersistentLevel);

	bInitialized = true;

	LastPawnPosition = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();

	PrimaryActorTick.SetTickFunctionEnable(true);
}

void APPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bInitialized == false)
		return;

	PortalMaterial->SetScalarParameterValue(FName("ScaleOffset"), 0);
	ClearPortalView();

	if (TargetPortal == nullptr)
		return;

	UpdatePortalView();

	if (IsPointInsidePortal(PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation()))
		PortalMaterial->SetScalarParameterValue(FName("ScaleOffset"), 1.0f);
}

void APPortal::PostPhysicsTick(float DeltaTime)
{
	UpdatePawnTracking();
}

void APPortal::Init(const bool bIsLeftPortal)
{
	bPortalLeft = bIsLeftPortal;
}

void APPortal::CreatePortalTexture()
{
	int32 ViewportX, ViewportY;
	PlayerController->GetViewportSize(ViewportX, ViewportY);
	ViewportX *= PortalRenderScale;
	ViewportY *= PortalRenderScale;

	RenderTarget = NewObject<UTextureRenderTarget2D>(this, UTextureRenderTarget2D::StaticClass(), FName("PortalRenderTarget"));
	check(RenderTarget);

	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	RenderTarget->Filter = TF_Bilinear;
	RenderTarget->SizeX = ViewportX;
	RenderTarget->SizeY = ViewportY;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->TargetGamma = 2.2f;
	RenderTarget->bNeedsTwoCopies = false;
	RenderTarget->AddressX = TA_Clamp;
	RenderTarget->AddressY = TA_Clamp;

	// Not needed since the texture is displayed on screen directly
	RenderTarget->bAutoGenerateMips = false;

	// This forces the engine to create the render target with the parameters we defined just above
	RenderTarget->UpdateResource();

	PortalMaterial = PortalMesh->CreateDynamicMaterialInstance(0, PortalMaterialInstance);
	PortalMaterial->SetTextureParameterValue(FName("RenderTarget"), RenderTarget);

	SceneCapture->TextureTarget = RenderTarget;
}

void APPortal::LinkPortal(APPortal* OtherPortal)
{
	if (TargetPortal != nullptr && TargetPortal == OtherPortal)
		return;

	if (IsValid(OtherPortal) == false)
	{
		PortalMesh->SetMaterial(0, DefaultPortalMaterial);;
		return;
	}

	TargetPortal = OtherPortal;
	PortalMesh->SetMaterial(0, PortalMaterial);
}

bool APPortal::IsPointInFrontOfPortal(const FVector& Point) const
{
	const FPlane PortalPlane = FPlane(PortalMesh->GetComponentLocation(), PortalMesh->GetForwardVector());
	const float PortalDot = PortalPlane.PlaneDot(Point);

	// If less than 0, we are behind the Plane
	return PortalDot >= 0.0f;
}

bool APPortal::IsPointCrossingPortal(const FVector& Point, FVector& OutIntersectionPoint) const
{
	const FPlane PortalPlane = FPlane(PortalMesh->GetComponentLocation(), PortalMesh->GetForwardVector());
	const bool bIsIntersecting = FMath::SegmentPlaneIntersection(LastPawnPosition, Point, PortalPlane, OutIntersectionPoint);

	return bIsIntersecting;
}

bool APPortal::IsPointInsidePortal(const FVector& Point) const
{
	const FVector HalfHeight = PortalBox->GetScaledBoxExtent();
	const FVector Direction = Point - PortalBox->GetComponentLocation();

	const bool bWithinX = FMath::Abs(FVector::DotProduct(Direction, PortalBox->GetForwardVector())) <= HalfHeight.X;
	const bool bWithinY = FMath::Abs(FVector::DotProduct(Direction, PortalBox->GetRightVector())) <= HalfHeight.Y;
	const bool bWithinZ = FMath::Abs(FVector::DotProduct(Direction, PortalBox->GetUpVector())) <= HalfHeight.Z;

	return bWithinX && bWithinY && bWithinZ;
}

void APPortal::UpdatePawnTracking()
{
	FVector IntersectionPoint;
	const FVector PawnPosition = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();
	const bool bIsIntersecting = IsPointCrossingPortal(PawnPosition, IntersectionPoint);
	const FVector RelativeIntersection = PortalMesh->GetComponentTransform().InverseTransformPositionNoScale(IntersectionPoint);
	const FVector PortalSize = PortalBox->GetScaledBoxExtent();
	const bool bPassedWithinPortal = FMath::Abs(RelativeIntersection.Z) <= PortalSize.Z && FMath::Abs(RelativeIntersection.Y) <= PortalSize.Y;
	
	if (bIsIntersecting && IsPointInFrontOfPortal(LastPawnPosition) && bPassedWithinPortal)
		TeleportActor(PlayerCharacter);

	LastPawnPosition = PawnPosition;
}


void APPortal::TeleportActor(AActor* ActorToTeleport)
{
	if (ActorToTeleport == nullptr || TargetPortal == nullptr)
		return;

	UE_LOG(LogPortal, Log, TEXT("Teleporting Actor %s"), *ActorToTeleport->GetName());
	
	// Perform a camera cut so the teleportation is seamless with the render functions
	SceneCapture->bCameraCutThisFrame = true;

	FVector SavedVelocity = FVector::ZeroVector;
	APCharacter* Character = nullptr;

	// Retrieve and save Player velocity
	if (ActorToTeleport->IsA<APCharacter>())
	{
		Character = Cast<APCharacter>(ActorToTeleport);
		SavedVelocity = Character->GetCharacterMovement()->Velocity;
	}

	// Compute and apply the new location
	const FVector NewLocation = UPPortalHelper::ConvertLocationToPortalSpace(ActorToTeleport->GetActorLocation(), this, TargetPortal);
	ActorToTeleport->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	// Compute and apply new rotation
	FRotator NewRotation = UPPortalHelper::ConvertRotationToPortalSpace(ActorToTeleport->GetActorRotation(), this, TargetPortal);
	ActorToTeleport->SetActorRotation(NewRotation);

	// Update controller and reapply velocity to teleported character
	if (Character != nullptr)
	{
		APPlayerController* PC = Cast<APPlayerController>(Character->GetController());
		if (PC != nullptr)
		{
			NewRotation = UPPortalHelper::ConvertRotationToPortalSpace(PC->GetControlRotation(), this, TargetPortal);
			NewRotation.Roll = 0.0f; // Cancel roll
			PC->SetControlRotation(NewRotation);
		}

		const FVector NewVelocity = UPPortalHelper::ConvertDirectionToPortalSpace(SavedVelocity, this, TargetPortal);
		Character->GetCharacterMovement()->Velocity = NewVelocity;

		Character->OnPortalTeleport();
	}

	// Update the portal view and world offset for the target portal
	TargetPortal->UpdateWorldOffset();
	TargetPortal->UpdatePortalView();
	TargetPortal->LastPawnPosition = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();
}

void APPortal::UpdatePortalView()
{
	// Check if we should resize the render target.
	// NOTE: Maybe use an event if too expensive to check viewport size every frame.
	int32 ViewportX, ViewportY;
	PlayerController->GetViewportSize(ViewportX, ViewportY);
	ViewportX *= PortalRenderScale;
	ViewportY *= PortalRenderScale;
	UPPortalHelper::ResizeRenderTarget(RenderTarget, ViewportX, ViewportY);

	// Get the camera post-processing settings
	const UCameraComponent* PlayerCamera = PlayerCharacter->GetFirstPersonCameraComponent();
	SceneCapture->PostProcessSettings = PlayerCamera->PostProcessSettings;

	// Setup clip plane to cut out objects between the camera and the back of the portal
	SceneCapture->bEnableClipPlane = true;
	SceneCapture->bOverride_CustomNearClippingPlane = true;
	SceneCapture->ClipPlaneNormal = TargetPortal->PortalMesh->GetForwardVector();
	SceneCapture->ClipPlaneBase = TargetPortal->PortalMesh->GetComponentLocation() - (SceneCapture->ClipPlaneNormal * 1.0f);

	// Get the projection matrix from the player's camera view settings
	SceneCapture->bUseCustomProjectionMatrix = true;
	SceneCapture->CustomProjectionMatrix = PlayerController->GetCameraProjectionMatrix();

	// Get the position of the main camera relative to the target portal
	const FVector NewCameraLocation = UPPortalHelper::ConvertLocationToPortalSpace(PlayerCamera->GetComponentLocation(), this, TargetPortal);
	const FRotator NewCameraRotation = UPPortalHelper::ConvertRotationToPortalSpace(PlayerCamera->GetComponentRotation(), this, TargetPortal);

	// Update the scene capture position and rotation
	SceneCapture->SetWorldLocationAndRotation(NewCameraLocation, NewCameraRotation);

	//DrawDebugBox(GetWorld(), NewCameraLocation, FVector(10.0f), NewCameraRotation.Quaternion(), FColor::Red, false, 0.05f, 0.0f, 2.0f);

	SceneCapture->CaptureScene();
}

void APPortal::ClearPortalView() const
{
	// Force portal to be a random color that can be found as a mask.
	if (PortalMaterial != nullptr)
		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), RenderTarget);
}

void APPortal::UpdateWorldOffset() const
{
	if (IsPointInsidePortal(PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation()))
		PortalMaterial->SetScalarParameterValue(FName("ScaleOffset"), 1.0f);
	else
		PortalMaterial->SetScalarParameterValue(FName("ScaleOffset"), 0.0f);
}

void FPostPhysicsTick::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	// Call the Portals second tick function for running post tick.
	if (Target)
		Target->PostPhysicsTick(DeltaTime);
}
