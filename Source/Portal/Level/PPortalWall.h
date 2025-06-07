// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPortalWall.generated.h"

class APGhostPortalBorder;
class APPortal;

UCLASS()
class PORTAL_API APPortalWall : public AActor
{
	GENERATED_BODY()

public:
	APPortalWall();
	
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Portal")
	bool TryGetPortalPos(const FVector& Origin, const APGhostPortalBorder* GhostBorder, bool bIsLeftPortal, FVector& OutPortalPosition) const;

private:
	FVector ConstrainPortalToWall(const FVector& RelativeLocation, float PortalHalfWidth, float PortalHalfHeight) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	float Width;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	float Height;
};
