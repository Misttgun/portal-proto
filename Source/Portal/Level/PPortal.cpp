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

APPortal::APPortal() : CurrentWall(nullptr), bPortalLeft(true), PortalRenderScale(1.0f), TargetPortal(nullptr), bInitialized(false), ActorsBeingTracked(0)
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
	SceneCapture->LODDistanceFactor = 3;
	SceneCapture->TextureTarget = nullptr;
	SceneCapture->CaptureSource = SCS_SceneColorHDR;

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
}

void APPortal::BeginPlay()
{
	Super::BeginPlay();

	// Save a ref to the player controller and player camera
	APPlayerController* PC = Cast<APPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PC != nullptr)
		PlayerController = PC;

	const APCharacter* Character = Cast<APCharacter>(PC->GetPawn());
	if (Character != nullptr)
		PlayerCamera = Character->GetFirstPersonCameraComponent();

	CreatePortalTexture();

	// Register the secondary post-physics tick function in the world on level start
	PhysicsTick.bCanEverTick = true;
	PhysicsTick.RegisterTickFunction(GetWorld()->PersistentLevel);

	bInitialized = true;

	// If playing game and this is the game world, set up the delegate bindings
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		PortalBox->OnComponentBeginOverlap.AddDynamic(this, &APPortal::OnPortalBoxOverlapStart);
		PortalBox->OnComponentEndOverlap.AddDynamic(this, &APPortal::OnPortalBoxOverlapEnd);
		PortalMesh->OnComponentBeginOverlap.AddDynamic(this, &APPortal::OnPortalMeshOverlapStart);
		PortalMesh->OnComponentEndOverlap.AddDynamic(this, &APPortal::OnPortalMeshOverlapEnd);
	}

	// Check if anything is already overlapping on begin play as overlap start will not be called in this case...
	TSet<AActor*> OverlappingActors;
	PortalBox->GetOverlappingActors(OverlappingActors);
	if (OverlappingActors.Num() > 0)
	{
		// For each found overlapping actor on begin play check if it can move and started overlapping in-front of the portal, if so, track it until it ends its overlap.
		for (AActor* OverlappedActor : OverlappingActors)
		{
			const USceneComponent* OverlappedRootComponent = OverlappedActor->GetRootComponent();
			if (OverlappedRootComponent && OverlappedRootComponent->IsSimulatingPhysics() || OverlappedActor->IsA(APCharacter::StaticClass()))
			{
				// Ensure that the item entering the portal is in-front.
				if (TrackedActors.Contains(OverlappedActor) == false && IsPointInFrontOfPortal(OverlappedRootComponent->GetComponentLocation()))
				{
					AddTrackedActor(OverlappedActor);
				}
			}
		}
	}

	PrimaryActorTick.SetTickFunctionEnable(true);
}

void APPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bInitialized == false)
		return;

	ClearPortalView();

	if (TargetPortal == nullptr)
		return;

	UpdatePortalView();
}

void APPortal::PostPhysicsTick(float DeltaTime)
{
	UpdateTrackedActors();
}

void APPortal::OnPortalBoxOverlapStart(UPrimitiveComponent*, AActor* OverlappedActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	const USceneComponent* OverlappedRootComponent = OverlappedActor->GetRootComponent();
	if (OverlappedRootComponent && OverlappedRootComponent->IsSimulatingPhysics() || OverlappedActor->IsA(APCharacter::StaticClass()))
	{
		// Ensure that the item entering the portal is in-front.
		if (TrackedActors.Contains(OverlappedActor) == false && IsPointInFrontOfPortal(OverlappedRootComponent->GetComponentLocation()))
		{
			AddTrackedActor(OverlappedActor);
		}
	}
}

void APPortal::OnPortalBoxOverlapEnd(UPrimitiveComponent*, AActor* OverlappedActor, UPrimitiveComponent*, int32)
{
	if (TrackedActors.Contains(OverlappedActor))
	{
		RemoveTrackedActor(OverlappedActor);
	}
}

void APPortal::OnPortalMeshOverlapStart(UPrimitiveComponent*, AActor* OverlappedActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	// Show the copied actor once it's overlapping with the portal itself.
	if (TrackedActors.Contains(OverlappedActor))
	{
		if (const AActor* Copy = TrackedActors.FindRef(OverlappedActor).TrackedCopy)
		{
			SetCopyVisibility(Copy, true);
		}
	}
}

void APPortal::OnPortalMeshOverlapEnd(UPrimitiveComponent*, AActor* OverlappedActor, UPrimitiveComponent*, int32)
{
	// Hide the copied actor once it's stopped overlapping with the portal by exiting it and not passing through it.
	if (TrackedActors.Contains(OverlappedActor))
	{
		if (const AActor* Copy = TrackedActors.FindRef(OverlappedActor).TrackedCopy)
		{
			SetCopyVisibility(Copy, false);
		}
	}
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
	RenderTarget->SizeX = ViewportX;
	RenderTarget->SizeY = ViewportY;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->TargetGamma = 2.2f;
	RenderTarget->bNeedsTwoCopies = false;
	RenderTarget->bCanCreateUAV = false;

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

bool APPortal::IsPointCrossingPortal(const FVector& StartPoint, const FVector& Point, FVector& OutIntersectionPoint) const
{
	const FPlane PortalPlane = FPlane(PortalMesh->GetComponentLocation(), PortalMesh->GetForwardVector());
	const bool bIsIntersecting = FMath::SegmentPlaneIntersection(StartPoint, Point, PortalPlane, OutIntersectionPoint);

	return bIsIntersecting;
}

void APPortal::AddTrackedActor(AActor* ActorToAdd)
{
	if (ActorToAdd == nullptr)
		return;

	// If it's the pawn track the camera otherwise track the root component
	FTrackedActor Tracked;
	if (ActorToAdd->IsA<APCharacter>())
	{
		Tracked.TrackedComp = PlayerCamera;
		Tracked.LastTrackedLocation = PlayerCamera->GetComponentLocation();
	}
	else
	{
		Tracked.TrackedComp = ActorToAdd->GetRootComponent();
		Tracked.LastTrackedLocation = ActorToAdd->GetActorLocation();
	}

	TrackedActors.Add(ActorToAdd, Tracked);
	ActorsBeingTracked++;

	// Create a visual copy of the tracked actor
	CopyActor(ActorToAdd);
}

void APPortal::RemoveTrackedActor(const AActor* ActorToRemove)
{
	if (ActorToRemove == nullptr)
		return;

	// Delete copy if there is one
	DeleteCopy(ActorToRemove);

	TrackedActors.Remove(ActorToRemove);
	ActorsBeingTracked--;
}

void APPortal::CopyActor(AActor* ActorToCopy)
{
	// Create a copy of the actor
	if (ActorToCopy == nullptr)
		return;

	// Ignore the player for now
	// TODO Handle player copy
	if (ActorToCopy->IsA<APCharacter>())
		return;

	const FName NewActorName = MakeUniqueObjectName(this, AActor::StaticClass(), "CopiedActor");
	AActor* NewActor = NewObject<AActor>(this, NewActorName, RF_NoFlags, ActorToCopy);
	ensureMsgf(NewActor, TEXT("Failed to create new actor in CopyActor."));
	if (NewActor == nullptr)
		return;

	NewActor->RegisterAllComponents();

	TArray<UActorComponent*> StaticMeshes;
	NewActor->GetComponents(UStaticMeshComponent::StaticClass(), StaticMeshes);

	for (UActorComponent* Comp : StaticMeshes)
	{
		UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Comp);
		StaticMeshComp->SetCollisionResponseToChannel(ECC_PortalBox, ECR_Ignore);
		StaticMeshComp->SetCollisionResponseToChannel(ECC_PortalWall, ECR_Ignore);
		StaticMeshComp->SetCollisionResponseToChannel(ECC_Portal, ECR_Ignore);
		StaticMeshComp->SetCollisionResponseToChannel(ECC_CompanionCube, ECR_Ignore);
		StaticMeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		StaticMeshComp->SetSimulatePhysics(false);
	}

	// Update the actor's tracking info
	FTrackedActor Tracked = TrackedActors.FindRef(ActorToCopy);
	Tracked.TrackedCopy = NewActor;
	TrackedActors.Remove(ActorToCopy);
	TrackedActors.Add(ActorToCopy, Tracked);

	// Set up location and rotation for this frame
	const FVector Location = UPPortalHelper::ConvertLocationToPortalSpace(NewActor->GetActorLocation(), this, TargetPortal);
	const FRotator Rotation = UPPortalHelper::ConvertRotationToPortalSpace(NewActor->GetActorRotation(), this, TargetPortal);
	NewActor->SetActorLocationAndRotation(Location, Rotation);

	// Map copy to the original actor
	CopiedActors.Add(NewActor, ActorToCopy);

	// Hide the copy from the main pass until it is overlapping the portal mesh
	SetCopyVisibility(NewActor, false);
}

void APPortal::DeleteCopy(const AActor* ActorToDelete)
{
	if (TrackedActors.Contains(ActorToDelete) == false)
		return;

	if (AActor* Copy = TrackedActors.FindRef(ActorToDelete).TrackedCopy)
	{
		CopiedActors.Remove(Copy);

		if (IsValid(Copy) == false)
			return;

		if (UWorld* World = GetWorld())
		{
			World->DestroyActor(Copy);
			Copy = nullptr;
			GEngine->ForceGarbageCollection();
		}
	}
}

void APPortal::SetCopyVisibility(const AActor* Actor, const bool IsVisible)
{
	if (IsValid(Actor) == false)
		return;

	TArray<UActorComponent*> Components;
	Actor->GetComponents(UStaticMeshComponent::StaticClass(), Components);
	for (UActorComponent* Comp : Components)
	{
		UStaticMeshComponent* StaticComp = Cast<UStaticMeshComponent>(Comp);
		StaticComp->SetRenderInMainPass(IsVisible);
	}
}

void APPortal::UpdateTrackedActors()
{
	if (TargetPortal == nullptr)
		return;
	
	if (ActorsBeingTracked <= 0)
		return;

	TArray<AActor*> TeleportedActors;
	for (TMap<AActor*, FTrackedActor>::TIterator TrackedPair = TrackedActors.CreateIterator(); TrackedPair; ++TrackedPair)
	{
		AActor* TrackedActor = TrackedPair->Key;

		// Update the positions for the duplicated tracked actors at the target portal
		AActor* Copy = TrackedPair->Value.TrackedCopy;
		if (IsValid(Copy))
		{
			const FVector ConvertedLocation = UPPortalHelper::ConvertLocationToPortalSpace(TrackedActor->GetActorLocation(), this, TargetPortal);
			const FRotator ConvertedRotation = UPPortalHelper::ConvertRotationToPortalSpace(TrackedActor->GetActorRotation(), this, TargetPortal);
			Copy->SetActorLocationAndRotation(ConvertedLocation, ConvertedRotation);
		}

		FTrackedActor TrackedInfo = TrackedPair->Value;
		
		bool bPassedThroughPortal;
		FVector CurrPosition;
		UPrimitiveComponent* Comp = Cast<UPrimitiveComponent>(TrackedActor->GetRootComponent());
		if (TrackedActor->IsA<APCharacter>())
		{
			// We update the collision profile so we can pass through portals, but we do it only when the target portal is active
			if (Comp->GetCollisionProfileName() != FName("PortalPawn"))
				Comp->SetCollisionProfileName(FName("PortalPawn"));
			
			FVector IntersectionPoint;
			CurrPosition = PlayerCamera->GetComponentLocation();
			const bool bIsIntersecting = IsPointCrossingPortal(TrackedInfo.LastTrackedLocation, CurrPosition, IntersectionPoint);
			const FVector RelativeIntersection = PortalMesh->GetComponentTransform().InverseTransformPositionNoScale(IntersectionPoint);
			const FVector PortalSize = PortalBox->GetScaledBoxExtent();
			const bool bWithinPortal = FMath::Abs(RelativeIntersection.Z) <= PortalSize.Z && FMath::Abs(RelativeIntersection.Y) <= PortalSize.Y;
			bPassedThroughPortal = bIsIntersecting && IsPointInFrontOfPortal(TrackedInfo.LastTrackedLocation) && bWithinPortal;
		}
		else
		{
			// We update the collision profile so we can pass through portals, but we do it only when the target portal is active
			if (Comp->GetCollisionProfileName() != FName("PortalCube"))
				Comp->SetCollisionProfileName(FName("PortalCube"));
			
			FVector IntersectionPoint;
			CurrPosition = TrackedInfo.TrackedComp->GetComponentLocation();
			bPassedThroughPortal = IsPointCrossingPortal(TrackedInfo.LastTrackedLocation, CurrPosition, IntersectionPoint);
		}

		if (bPassedThroughPortal)
		{
			TeleportActor(TrackedActor);

			// Add the actor to be removed after teleportation
			TeleportedActors.Add(TrackedActor);

			// Skip to the next actor
			continue;
		}

		// Update the last tracked position if we don't teleport the actor
		TrackedInfo.LastTrackedLocation = CurrPosition;
	}

	// Ensure the tracked actor has been removed, added to the target portal it's been teleported to, and it's copy is not hidden from the render pass
	for (AActor* Actor : TeleportedActors)
	{
		if (IsValid(Actor) == false)
			continue;

		if (TrackedActors.Contains(Actor))
			TrackedActors.Remove(Actor);

		if (TargetPortal->TrackedActors.Contains(Actor) == false)
			TargetPortal->TrackedActors.Add(Actor);

		if (const AActor* Copy = TargetPortal->TrackedActors.FindRef(Actor).TrackedCopy)
			SetCopyVisibility(Copy, true);
	}
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

		//Character->ReleaseActor();
		Character->OnPortalTeleport();
	}
	else
	{
		UPrimitiveComponent* Comp = Cast<UPrimitiveComponent>(ActorToTeleport->GetRootComponent());
		bool bIsGrabbed = false;
		
		Character = Cast<APCharacter>(PlayerController->GetPawn());
		if (Character != nullptr)
		{
			if (const UPrimitiveComponent* GrabbedComp = Character->GetGrabbedComponent())
			{
				if (GrabbedComp == Comp)
					bIsGrabbed = true;
					//Character->ReleaseActor();
			}
		}

		if (bIsGrabbed == false)
		{
			const FVector NewLinearVelocity = UPPortalHelper::ConvertDirectionToPortalSpace(Comp->GetPhysicsLinearVelocity(), this, TargetPortal);
			const FVector NewAngularVelocity = UPPortalHelper::ConvertDirectionToPortalSpace(Comp->GetPhysicsAngularVelocityInDegrees(), this, TargetPortal);
			Comp->SetPhysicsLinearVelocity(NewLinearVelocity);
			Comp->SetPhysicsAngularVelocityInDegrees(NewAngularVelocity);
		}
	}

	// Update the portal view for the target portal
	TargetPortal->UpdatePortalView();

	// Make sure the copy created is not hidden after teleportation
	if (TargetPortal->TrackedActors.Contains(ActorToTeleport))
	{
		if (const AActor* Copy = TargetPortal->TrackedActors.FindRef(ActorToTeleport).TrackedCopy)
			SetCopyVisibility(Copy, true);
	}
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

	SceneCapture->CaptureScene();
}

void APPortal::ClearPortalView() const
{
	// Force portal to be a random color that can be found as a mask.
	if (PortalMaterial != nullptr)
		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), RenderTarget);
}

void APPortal::UpdatePortalBorderCollision(const bool bIsFloorPortal) const
{
	if (bIsFloorPortal)
		PortalBorderMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	else
		PortalBorderMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void FPostPhysicsTick::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	// Call the Portals second tick function for running post tick.
	if (Target)
		Target->PostPhysicsTick(DeltaTime);
}
