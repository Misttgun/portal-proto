// Copyright (c) 2025 Maurel Sagbo


#include "PPortalWall.h"
#include "DrawDebugHelpers.h"
#include "PGhostPortalBorder.h"

extern TAutoConsoleVariable<bool> CVarDebugDrawTrace;

APPortalWall::APPortalWall() : Width{100.0f}, Height{100.0f}
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>("SceneRoot");
	RootComponent = SceneRoot;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(SceneRoot);
}

void APPortalWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FVector WorldScale = FVector(1.0f, Width / 100, Height / 100); // The mesh is 100 by 100 cm
	MeshComp->SetWorldScale3D(WorldScale);
}

bool APPortalWall::TryGetPortalPos_Implementation(const FVector& Origin, const APGhostPortalBorder* GhostBorder, const bool bIsLeftPortal, FVector& OutPortalPosition, FVector2D& OutPortalExtents) const
{
	const bool bDrawDebug = CVarDebugDrawTrace.GetValueOnGameThread();

	TArray<FVector> Vertices = GhostBorder->GetVertices();
	ensureMsgf(Vertices.Num() > 0, TEXT("Vertices is empty."));

	TArray<FVector> WorldVertices;
	const int32 NumVertices = Vertices.Num();
	for (int32 i = 0; i < NumVertices; i++)
	{
		FVector RotatedVertex = GhostBorder->GetRelativeRotation().RotateVector(Vertices[i]);
		FVector WorldVertex = GhostBorder->GetActorTransform().TransformPosition(RotatedVertex);

		WorldVertices.Add(WorldVertex);
	}

	FVector LocalVertex = GetTransform().InverseTransformPosition(WorldVertices[0]);

	float MaxZ, MaxY;
	float MinZ = MaxZ = LocalVertex.Z;
	float MinY = MaxY = LocalVertex.Y;

	for (int i = 1; i < WorldVertices.Num(); ++i)
	{
		LocalVertex = GetTransform().InverseTransformPosition(WorldVertices[i]);

		MinZ = FMath::Min(MinZ, LocalVertex.Z);
		MinY = FMath::Min(MinY, LocalVertex.Y);
		MaxZ = FMath::Max(MaxZ, LocalVertex.Z);
		MaxY = FMath::Max(MaxY, LocalVertex.Y);
	}

	const float PortalWidth = MaxY - MinY;
	const float PortalHeight = MaxZ - MinZ;

	if (PortalWidth > Width || PortalHeight > Height)
		return false;

	const float PortalHalfWidth = PortalWidth / 2;
	const float PortalHalfHeight = PortalHeight / 2;

	OutPortalExtents = FVector2D(PortalHalfWidth, PortalHalfHeight);

	const FVector RelativeLocation = GetTransform().InverseTransformPosition(Origin);
	const bool bIsOutsideWallZ = FMath::Abs(RelativeLocation.Z) + PortalHalfHeight > Height / 2;
	const bool bIsOutsideWallY = FMath::Abs(RelativeLocation.Y) + PortalHalfWidth > Width / 2;

	OutPortalPosition = Origin;

	if (bIsOutsideWallY || bIsOutsideWallZ)
	{
		const FVector ConstrainedLocation = ConstrainPortalToWall(RelativeLocation, PortalHalfWidth, PortalHalfHeight);
		OutPortalPosition = GetTransform().TransformPosition(ConstrainedLocation);
	}

	if (bDrawDebug)
	{
		const FColor SphereColor = bIsLeftPortal ? FColor::Green : FColor::Orange;
		DrawDebugSphere(GetWorld(), OutPortalPosition, 10.0f, 12, SphereColor, false, 1.0f);
	}

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
