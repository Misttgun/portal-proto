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
#include "PhysicsEngine/PhysicsHandleComponent.h"

DEFINE_LOG_CATEGORY(LogPortalCharacter);

TAutoConsoleVariable<bool> CVarDebugDrawTrace(TEXT("sm.TraceDebugDraw"), false, TEXT("Enable Debug Lines for Character Traces"), ECVF_Cheat);

APCharacter::APCharacter() : GunSocketName(FName(TEXT("GripPoint"))), CollisionChannel(ECC_WorldDynamic), GrabDistance(150.0f), TraceDistance(300.0f),
                             TraceRadius(30.0f), bIsGrabbingActor(false), bReturnToOrientation(false)
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

	WalkableFloorCos = FMath::Cos(UE_DOUBLE_PI/(180.0) * GetCharacterMovement()->GetWalkableFloorAngle());

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
		const FVector GrabLocation = FirstPersonCameraComp->GetComponentLocation() + FirstPersonCameraComp->GetForwardVector() * GrabDistance;
		PhysicsHandleComp->SetTargetLocation(GrabLocation);

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
	PhysicsHandleComp->GrabComponentAtLocationWithRotation(CompToGrab, FName(), CompToGrab->GetComponentLocation(), FRotator::ZeroRotator);
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

	TArray<FHitResult> HitResults;

	const FCollisionObjectQueryParams QueryParams(CollisionChannel);

	const FVector StartLocation = FirstPersonCameraComp->GetComponentLocation();
	const FVector EndLocation = StartLocation + FirstPersonCameraComp->GetForwardVector() * TraceDistance;
	const FCollisionShape ColShape = FCollisionShape::MakeSphere(TraceRadius);

	const bool bBlockingHit = GetWorld()->SweepMultiByObjectType(HitResults, StartLocation, EndLocation, FQuat::Identity, QueryParams, ColShape);

	if (bBlockingHit)
	{
		for (FHitResult Hit : HitResults)
		{
			if (bDrawDebug)
				DrawDebugSphere(GetWorld(), Hit.Location, TraceRadius, 32, FColor::Green, false, 0.0f);

			AActor* HitActor = Hit.GetActor();
			if (IsValid(HitActor))
			{
				FocusedActor = HitActor;
				break;
			}
		}
	}

	if (bDrawDebug)
	{
		const FColor LineColor = bBlockingHit ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, LineColor, false, 1.0f);
	}
}

void APCharacter::OnPortalTeleport()
{
	OrientationReturnTimer = GetWorld()->GetTimeSeconds();
	OrientationAtStart = GetCapsuleComponent()->GetComponentRotation();
	bReturnToOrientation = true;

	// TODO Implement a solution where we can keep holding actor through a portal and release it if we can't raycast to the thing we are holding.
	// Will need to implement ray casting through portals.
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
