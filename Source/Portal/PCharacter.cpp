// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "PGunComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Helpers/PPortalHelper.h"
#include "Level/PPortal.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

DEFINE_LOG_CATEGORY(LogPortalCharacter);

TAutoConsoleVariable<bool> CVarDebugDrawTrace(TEXT("sm.TraceDebugDraw"), false, TEXT("Enable Debug Lines for Character Traces"), ECVF_Cheat);

APCharacter::APCharacter() : GunSocketName(FName(TEXT("GripPoint"))), CollisionChannel(ECC_WorldDynamic), TraceDistance(150.0f),
                             TraceRadius(15.0f), bIsGrabbingActor(false), bIsGrabbingThroughPortal(false), bReturnToOrientation(false)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent	
	FirstPersonCameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComp->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComp->SetRelativeLocation(FVector(0.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComp->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComp);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	GunComp = CreateDefaultSubobject<UPGunComponent>(TEXT("PortalGun"));
	GunComp->SetupAttachment(Mesh1P, GunSocketName);

	PhysicsHandleComp = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandleComp"));
}

void APCharacter::BeginPlay()
{
	Super::BeginPlay();

	WalkableFloorCos = FMath::Cos(UE_DOUBLE_PI / (180.0) * GetCharacterMovement()->GetWalkableFloorAngle());

	if (IsValid(GunComp) == false)
		return;

	GunComp->Init(this);
}

void APCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled() == false)
		return;

	if (bReturnToOrientation)
		ReturnToOrientation();

	if (bIsGrabbingActor)
	{
		UpdateGrabbedActorPos();
		return;
	}

	FindActorToGrab();
}


void APCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APCharacter::Look);
		EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Triggered, this, &APCharacter::ProcessGrab);
	}
	else
	{
		UE_LOG(LogPortalCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system."
			       " If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void APCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void APCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APCharacter::ProcessGrab()
{
	if (bIsGrabbingActor)
		ReleaseActor();
	else
		GrabActor();
}

void APCharacter::GrabActor()
{
	if (FocusedActor == nullptr)
		return;

	UPrimitiveComponent* CompToGrab = FocusedActor->GetComponentByClass<UPrimitiveComponent>();
	ensure(CompToGrab != nullptr);

	if (bIsGrabbingThroughPortal == false)
		GrabbedRelativeLocation = FirstPersonCameraComp->GetComponentTransform().InverseTransformPositionNoScale(CompToGrab->GetComponentLocation());

	PhysicsHandleComp->GrabComponentAtLocationWithRotation(CompToGrab, NAME_None, CompToGrab->GetComponentLocation(), FRotator::ZeroRotator);
	CompToGrab->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	bIsGrabbingActor = true;

	// Clear focus actor on grab
	FocusedActor = nullptr;
}

void APCharacter::ReleaseActor()
{
	if (bIsGrabbingActor == false)
		return;

	if (UPrimitiveComponent* GrabbedComp = PhysicsHandleComp->GetGrabbedComponent())
	{
		GrabbedComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		PhysicsHandleComp->ReleaseComponent();
	}

	bIsGrabbingActor = false;
}

void APCharacter::FindActorToGrab()
{
	const bool bDrawDebug = CVarDebugDrawTrace.GetValueOnGameThread();

	// Clear previous focus actor
	FocusedActor = nullptr;
	bIsGrabbingThroughPortal = false;

	TArray<FHitResult> HitResults;

	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(CollisionChannel);
	QueryParams.AddObjectTypesToQuery(ECC_Portal);

	const FVector StartLocation = FirstPersonCameraComp->GetComponentLocation();
	const FVector EndLocation = StartLocation + FirstPersonCameraComp->GetForwardVector() * TraceDistance;
	const FCollisionShape ColShape = FCollisionShape::MakeSphere(TraceRadius);

	const bool bBlockingHit = GetWorld()->SweepMultiByObjectType(HitResults, StartLocation, EndLocation, FQuat::Identity, QueryParams, ColShape);

	if (bBlockingHit)
	{
		for (FHitResult Hit : HitResults)
		{
			if (bDrawDebug)
			{
				DrawDebugLine(GetWorld(), StartLocation, Hit.ImpactPoint, FColor::Green, false, 1.0f);
				DrawDebugSphere(GetWorld(), Hit.ImpactPoint, TraceRadius, 32, FColor::Green, false, 0.0f);
			}

			AActor* HitActor = Hit.GetActor();
			if (IsValid(HitActor))
			{
				// Trace through a portal so we can pick up the companion cube relative to the portal
				APPortal* HitPortal = Cast<APPortal>(HitActor);
				if (HitPortal && HitPortal->GetLinkedPortal() != nullptr)
				{
					if (FindActorToGrabThroughPortal(bDrawDebug, EndLocation, ColShape, Hit, HitPortal))
						return;
				}
				else
				{
					FocusedActor = HitActor;
					return;
				}
			}
		}
	}
	else
	{
		if (bDrawDebug)
			DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 1.0f);
	}
}

bool APCharacter::FindActorToGrabThroughPortal(const bool bDrawDebug, const FVector& EndLocation, const FCollisionShape ColShape, const FHitResult& Hit, APPortal* HitPortal)
{
	const FVector NewStartLocation = UPPortalHelper::ConvertLocationToPortalSpace(Hit.ImpactPoint, HitPortal, HitPortal->GetLinkedPortal());
	const FVector NewEndLocation = UPPortalHelper::ConvertLocationToPortalSpace(EndLocation, HitPortal, HitPortal->GetLinkedPortal());

	FCollisionObjectQueryParams NewQueryParams;
	NewQueryParams.AddObjectTypesToQuery(CollisionChannel);

	TArray<FHitResult> NewHitResults;
	const bool bNewHit = GetWorld()->SweepMultiByObjectType(NewHitResults, NewStartLocation, NewEndLocation, FQuat::Identity, NewQueryParams, ColShape);
	if (bNewHit)
	{
		for (FHitResult NewHit : NewHitResults)
		{
			AActor* NewHitActor = NewHit.GetActor();
			if (IsValid(NewHitActor))
			{
				if (bDrawDebug)
				{
					DrawDebugLine(GetWorld(), NewStartLocation, NewHit.ImpactPoint, FColor::Green, false, 1.0f);
					DrawDebugSphere(GetWorld(), NewHit.Location, TraceRadius, 32, FColor::Green, false, 0.0f);
				}

				FocusedActor = NewHitActor;
				bIsGrabbingThroughPortal = true;

				const USceneComponent* FocusedComp = NewHitActor->GetRootComponent();
				const FVector GrabLocation = UPPortalHelper::ConvertLocationToPortalSpace(FocusedComp->GetComponentLocation(), HitPortal, HitPortal->GetLinkedPortal());
				GrabbedRelativeLocation = FirstPersonCameraComp->GetComponentTransform().InverseTransformPositionNoScale(GrabLocation);

				return true;
			}
		}
	}
	else
	{
		if (bDrawDebug)
			DrawDebugLine(GetWorld(), NewStartLocation, NewEndLocation, FColor::Red, false, 1.0f);
	}

	return false;
}

void APCharacter::UpdateGrabbedActorPos()
{
	const FVector NewLocation = FirstPersonCameraComp->GetComponentTransform().TransformPositionNoScale(GrabbedRelativeLocation);

	if (bIsGrabbingThroughPortal)
	{
		FHitResult HitResult;
	
		FCollisionObjectQueryParams QueryParams;
		QueryParams.AddObjectTypesToQuery(ECC_Portal);
	
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(this);
		CollisionQueryParams.AddIgnoredActor(PhysicsHandleComp->GetGrabbedComponent()->GetOwner());
	
		const bool bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, FirstPersonCameraComp->GetComponentLocation(), NewLocation, QueryParams, CollisionQueryParams);
		APPortal* HitPortal = Cast<APPortal>(HitResult.GetActor());
		if (bHit && HitPortal && HitPortal->GetLinkedPortal() != nullptr)
		{
			const FVector Location = UPPortalHelper::ConvertLocationToPortalSpace(NewLocation, HitPortal, HitPortal->GetLinkedPortal());
			PhysicsHandleComp->SetTargetLocation(Location);
		}
	
		bIsGrabbingThroughPortal = bHit;
	}
	else
	{
		PhysicsHandleComp->SetTargetLocation(NewLocation);
	}
}

void APCharacter::OnPortalTeleport()
{
	OrientationReturnTimer = GetWorld()->GetTimeSeconds();
	OrientationAtStart = GetCapsuleComponent()->GetComponentRotation();
	bReturnToOrientation = true;
}

void APCharacter::ReturnToOrientation()
{
	const float Alpha = (GetWorld()->GetTimeSeconds() - OrientationReturnTimer) / 1.0f;
	const FRotator CurrentOrientation = GetCapsuleComponent()->GetComponentRotation();
	const FQuat Target = FRotator(0.0f, CurrentOrientation.Yaw, 0.0f).Quaternion();
	const FQuat NewOrientation = FQuat::Slerp(CurrentOrientation.Quaternion(), Target, Alpha);
	GetCapsuleComponent()->SetWorldRotation(NewOrientation.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);

	if (Alpha >= 1.0f)
		bReturnToOrientation = false;
}
