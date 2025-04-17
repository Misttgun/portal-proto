// Copyright (c) 2025 Maurel Sagbo


#include "PPortalWall.h"
#include "DrawDebugHelpers.h"


APPortalWall::APPortalWall() : Width{100.0f}, Height{100.0f}
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>("SceneRoot");
	SceneRoot->SetupAttachment(RootComponent);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(SceneRoot);
}

void APPortalWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FVector WorldScale = FVector(1.0f, Width / 100, Height / 100); // The mesh is 100 by 100 cm
	MeshComp->SetWorldScale3D(WorldScale);
}

bool APPortalWall::TryAddPortal(const FVector& Origin, const float PortalWidth, const float PortalHeight, const bool bIsLeftPortal) const
{
	if (PortalWidth > Width || PortalHeight > Height)
		return false;
	
	const float PortalHalfWidth = PortalWidth / 2;
	const float PortalHalfHeight = PortalHeight / 2;

	const UE::Math::TVector<double> RelativeLocation = GetTransform().InverseTransformPosition(Origin);
	const bool bIsOutsideWallZ = FMath::Abs(RelativeLocation.Z) + PortalHalfHeight > Height / 2;
	const bool bIsOutsideWallY = FMath::Abs(RelativeLocation.Y) + PortalHalfWidth > Width / 2;

	FVector PortalWorldLocation = Origin;

	if (bIsOutsideWallY || bIsOutsideWallZ)
	{
		const FVector ConstrainedLocation = ConstrainPortalToWall(RelativeLocation, PortalHalfWidth, PortalHalfHeight);
		PortalWorldLocation = GetTransform().TransformPosition(ConstrainedLocation);
	}
	const FColor SphereColor = bIsLeftPortal ? FColor::Green : FColor::Orange;
	DrawDebugSphere(GetWorld(), PortalWorldLocation, 10.0f, 12, SphereColor, false, 1.0f);
	
	return true;
}

FVector APPortalWall::ConstrainPortalToWall(const FVector& RelativeLocation, const float PortalHalfWidth, const float PortalHalfHeight) const
{
	constexpr float MinFloat = -1000000000.0f;

	FVector ConstrainedLocation = RelativeLocation;
	
	// Vertical
	const float ZPosition = RelativeLocation.Z;
	float ZDelta = Height / 2 - PortalHalfHeight - FMath::Abs(ZPosition);
	ZDelta = FMath::Clamp(ZDelta, MinFloat, 0.0f);
	
	if (ZPosition > 0.0f)
		ZDelta *= -1.0f;
	
	ConstrainedLocation.Z -= ZDelta;

	// Horizontal
	const float YPosition = RelativeLocation.Y;
	float YDelta = Width / 2 - PortalHalfWidth - FMath::Abs(YPosition);
	YDelta = FMath::Clamp(YDelta, MinFloat, 0.0f);

	if (YPosition > 0.0f)
		YDelta *= -1.0f;

	ConstrainedLocation.Y -= YDelta;

	return ConstrainedLocation;
}
