// Copyright (c) 2025 Maurel Sagbo


#include "PGunComponent.h"
#include "PCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Level/PPortalWall.h"

UPGunComponent::UPGunComponent() : PortalWallChannel(ECC_WorldStatic), MaxPortalDistance(10000.0f), PortalSize(100, 50)
{
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
}

void UPGunComponent::Init(APCharacter* TargetCharacter)
{
	this->OwningCharacter = TargetCharacter;

	// Set up action bindings
	if (const APlayerController* PlayerController = Cast<APlayerController>(OwningCharacter->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			EnhancedInputComponent->BindAction(LeftPortalAction, ETriggerEvent::Triggered, this, &UPGunComponent::PlaceLeftPortal);
			EnhancedInputComponent->BindAction(RightPortalAction, ETriggerEvent::Triggered, this, &UPGunComponent::PlaceRightPortal);
		}
	}
}

void UPGunComponent::Fire(const bool bIsLeftPortal) const
{
	if (OwningCharacter == nullptr || OwningCharacter->GetController() == nullptr)
		return;

	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, OwningCharacter->GetActorLocation());
	}

	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = OwningCharacter->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

	// Try and place a portal
	const TObjectPtr<UCameraComponent> CameraComp = OwningCharacter->GetFirstPersonCameraComponent();
	if (CameraComp == nullptr)
		return;

	const FVector StartLocation = CameraComp->GetComponentLocation();
	const FVector EndLocation = StartLocation + CameraComp->GetForwardVector() * MaxPortalDistance;

	const FCollisionObjectQueryParams QueryParams(PortalWallChannel);

	FHitResult HitResult;
	
	const bool bBlockingHit = GetWorld()->LineTraceSingleByObjectType(HitResult, StartLocation, EndLocation, QueryParams);
	if (bBlockingHit == false)
		return;

	AActor* HitActor = HitResult.GetActor();
	if (IsValid(HitActor) == false)
		return;

	const APPortalWall* PortalWall = Cast<APPortalWall>(HitActor);
	if (PortalWall != nullptr)
	{
		const FVector PortalOrigin = HitResult.Location + HitResult.ImpactNormal;
		const bool bHasPlaceWall = PortalWall->TryAddPortal(PortalOrigin, PortalSize.X, PortalSize.Y, bIsLeftPortal);

		if (bHasPlaceWall == false)
		{
			UE_LOG(LogTemp, Error, TEXT("'%s' Failed to place a Portal, wall is too small or a portal is already on this wall!"), *GetNameSafe(this));
		}
	}
	else
	{
		// TODO Play a different FX when shooting at non valid walls
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find a Portal Wall!"), *GetNameSafe(this));
	}
}

void UPGunComponent::PlaceLeftPortal()
{
	Fire(true);
}

void UPGunComponent::PlaceRightPortal()
{
	Fire(false);
}

void UPGunComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwningCharacter == nullptr)
		return;

	if (const APlayerController* PlayerController = Cast<APlayerController>(OwningCharacter->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}
