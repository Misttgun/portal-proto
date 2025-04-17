// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "PGunComponent.generated.h"

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

	void Fire(bool bIsLeftPortal) const;

private:
	UFUNCTION()
	void PlaceLeftPortal();

	UFUNCTION()
	void PlaceRightPortal();
	
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
	TEnumAsByte<ECollisionChannel> PortalWallChannel;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	float MaxPortalDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	FVector2D PortalSize;
};
