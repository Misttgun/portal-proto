// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Level/PPortalWall.h"
#include "PGunComponent.generated.h"

class APGhostPortalBorder;
class APPortal;
class UInputMappingContext;
class APCharacter;
class UInputAction;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PORTAL_API UPGunComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UPGunComponent();

	void Init(APCharacter* TargetCharacter);

protected:
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Fire(bool bIsLeftPortal);

private:
	UFUNCTION()
	void PlaceLeftPortal();

	UFUNCTION()
	void PlaceRightPortal();

	void SpawnPortal(APPortalWall* PortalWall, const UE::Math::TRotator<double>& Rotation, const FVector& PortalLocation, const FVector2D& PortalExtents, bool bIsLeftPortal, bool bIsFloorPortal);
	APPortal* SpawnAndInitializePortal(APPortalWall* PortalWall, const UE::Math::TRotator<double>& Rotation, const FVector& PortalLocation, bool bIsLeftPortal) const;
	void UpdatePortalTransform(APPortal* Portal, const FVector& PortalLocation, const UE::Math::TRotator<double>& Rotation);
	void FinalizePortalSetup(APPortal* Portal, const FVector2D& PortalExtents, APPortalWall* PortalWall, bool bIsFloorPortal);

	bool IsPortalPlacementValid(const APPortalWall* PortalWall, bool bIsLeftPortal, const FVector& PortalLocation, const FVector2D& PortalExtents) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> FireAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, meta = (AllowPrivateAccess = "true", MakeEditWidget = true))
	FVector MuzzleOffset;

	/** The Character holding this weapon*/
	UPROPERTY(BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<APCharacter> OwningCharacter;

	// Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> FireMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LeftPortalAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> RightPortalAction;

	// Portal
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<APPortal> PortalClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<APGhostPortalBorder> PortalBorderGhostClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> PortalWallChannel;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	float MaxPortalDistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<APPortal> LeftPortal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<APPortal> RightPortal;

	UPROPERTY()
	APGhostPortalBorder* GhostBorder;
};
