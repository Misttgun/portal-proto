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
#include "Helpers/PPortalHelper.h"
#include "Level/PPortal.h"
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

void UPGunComponent::Fire(const bool bIsLeftPortal)
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

	APPortalWall* PortalWall = Cast<APPortalWall>(HitActor);
	if (PortalWall != nullptr)
	{
		const auto Rotation = HitResult.ImpactNormal.Rotation();
		const FVector Origin = HitResult.Location + HitResult.ImpactNormal;
		FVector PortalLocation;
		const bool bHasSpace = PortalWall->TryGetPortalPos(Origin, PortalSize.X, PortalSize.Y, bIsLeftPortal, PortalLocation);

		// Not enough space on the wall to spawn a portal
		if (bHasSpace == false)
		{
			// TODO Play failed portal placement FX
			UE_LOG(LogTemp, Error, TEXT("'%s' Failed to place a Portal, wall is too small. or a portal is already on this wall!"), *GetNameSafe(this));
			return;
		}

		// Check collision with the other portal
		const bool bCanPlacePortal = IsPortalPlacementValid(PortalWall, bIsLeftPortal, PortalLocation);
		if (bCanPlacePortal == false)
		{
			// TODO Play failed portal placement FX
			UE_LOG(LogTemp, Error, TEXT("'%s' Failed to place a Portal, a portal is already on this wall!"), *GetNameSafe(this));
			return;
		}

		SpawnPortal(PortalWall, Rotation, PortalLocation, bIsLeftPortal);
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

void UPGunComponent::SpawnPortal(APPortalWall* PortalWall, const UE::Math::TRotator<double>& Rotation, const FVector& PortalLocation, const bool bIsLeftPortal)
{
	if (bIsLeftPortal)
	{
		if (LeftPortal == nullptr)
			LeftPortal = SpawnAndInitializePortal(PortalWall, Rotation, PortalLocation, bIsLeftPortal);
		else
			UpdatePortalTransform(LeftPortal, PortalLocation, Rotation);

		FinalizePortalSetup(LeftPortal, PortalWall);
	}
	else
	{
		if (RightPortal == nullptr)
			RightPortal = SpawnAndInitializePortal(PortalWall, Rotation, PortalLocation, bIsLeftPortal);
		else
			UpdatePortalTransform(RightPortal, PortalLocation, Rotation);

		FinalizePortalSetup(RightPortal, PortalWall);
	}

	if (LeftPortal)
		LeftPortal->LinkPortal(RightPortal);

	if (RightPortal)
		RightPortal->LinkPortal(LeftPortal);
}

APPortal* UPGunComponent::SpawnAndInitializePortal(APPortalWall* PortalWall, const UE::Math::TRotator<double>& Rotation, const FVector& PortalLocation, const bool bIsLeftPortal) const
{
	const FTransform SpawnTransform = FTransform(Rotation, PortalLocation);
	APPortal* SpawnedPortal = GetWorld()->SpawnActorDeferred<APPortal>(PortalClass, SpawnTransform, PortalWall, OwningCharacter, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (SpawnedPortal)
	{
		SpawnedPortal->Init(bIsLeftPortal);
		UGameplayStatics::FinishSpawningActor(SpawnedPortal, SpawnTransform);
	}

	return SpawnedPortal;
}

void UPGunComponent::UpdatePortalTransform(APPortal* Portal, const FVector& PortalLocation, const UE::Math::TRotator<double>& Rotation)
{
	Portal->SetActorRotation(Rotation);
	Portal->SetActorLocation(PortalLocation);
}

void UPGunComponent::FinalizePortalSetup(APPortal* Portal, APPortalWall* PortalWall)
{
	Portal->CurrentWall = PortalWall;
	Portal->OnPortalSpawned();
}

bool UPGunComponent::IsPortalPlacementValid(const APPortalWall* PortalWall, const bool bIsLeftPortal, const FVector& PortalLocation) const
{
	if (bIsLeftPortal)
	{
		if (RightPortal && RightPortal->CurrentWall == PortalWall)
		{
			const auto RightPortalRelativeLocation = PortalWall->GetTransform().InverseTransformPosition(RightPortal->GetActorLocation());
			const auto LeftPortalRelativeLocation = PortalWall->GetTransform().InverseTransformPosition(PortalLocation);
			return UPPortalHelper::IsPortalColliding(RightPortalRelativeLocation, LeftPortalRelativeLocation, PortalSize) == false;
		}
	}
	else
	{
		if (LeftPortal && LeftPortal->CurrentWall == PortalWall)
		{
			const auto LeftPortalRelativeLocation = PortalWall->GetTransform().InverseTransformPosition(LeftPortal->GetActorLocation());
			const auto RightPortalRelativeLocation = PortalWall->GetTransform().InverseTransformPosition(PortalLocation);
			return UPPortalHelper::IsPortalColliding(LeftPortalRelativeLocation, RightPortalRelativeLocation, PortalSize) == false;
		}
	}

	return true;
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
