// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

DEFINE_LOG_CATEGORY(LogPortalCharacter);

static TAutoConsoleVariable<bool> CVarDebugDrawTrace(TEXT("sm.TraceDebugDraw"), false, TEXT("Enable Debug Lines for Character Traces"), ECVF_Cheat);

APCharacter::APCharacter() : CollisionChannel(ECC_WorldDynamic), GrabDistance(150.0f), TraceDistance(300.0f), TraceRadius(30.0f), bIsGrabbingActor(false)
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
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	PhysicsHandleComp = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandleComp"));
}

void APCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled() == false)
		return;

	if (bIsGrabbingActor)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		GetActorEyesViewPoint(EyeLocation, EyeRotation);
		
		const FVector GrabLocation = EyeLocation + EyeRotation.Vector() * GrabDistance;
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
	bIsGrabbingActor = true;

	// Clear focus actor on grab
	FocusedActor = nullptr;
}

void APCharacter::ReleaseActor()
{
	if (bIsGrabbingActor == false)
		return;

	PhysicsHandleComp->ReleaseComponent();
	bIsGrabbingActor = false;
}

void APCharacter::FindActorToGrab()
{
	const bool bDrawDebug = CVarDebugDrawTrace.GetValueOnGameThread();

	// Clear previous focus actor
	FocusedActor = nullptr;

	TArray<FHitResult> HitResults;

	const FCollisionObjectQueryParams QueryParams(CollisionChannel);
	
	FVector StartLocation;
	FRotator StartRotation;
	GetActorEyesViewPoint(StartLocation, StartRotation);

	const FVector EndLocation = StartLocation + StartRotation.Vector() * TraceDistance;
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
