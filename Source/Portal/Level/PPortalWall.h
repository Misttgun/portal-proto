// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPortalWall.generated.h"

UCLASS()
class PORTAL_API APPortalWall : public AActor
{
	GENERATED_BODY()

public:
	APPortalWall();
	
	virtual void OnConstruction(const FTransform& Transform) override;

	bool TryAddPortal(const FVector& Origin, float PortalWidth, float PortalHeight, bool bIsLeftPortal) const;

private:
	FVector ConstrainPortalToWall(const FVector& RelativeLocation, float PortalHalfWidth, float PortalHalfHeight) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Wall, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Wall, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wall, meta = (AllowPrivateAccess = "true"))
	float Width;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wall, meta = (AllowPrivateAccess = "true"))
	float Height;
};
