// Copyright (c) 2025 Maurel Sagbo


#include "PPortalHelper.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Portal/Level/PPortal.h"

bool UPPortalHelper::IsPortalColliding(const FVector& OriginPortalA, const FVector& OriginPortalB, const FVector2D& PortalSize)
{
	const float HalfWidth = PortalSize.X / 2;
	const float HalfHeight = PortalSize.Y / 2;

	const bool bIsCollidingHorizontally = OriginPortalA.Y + HalfWidth > OriginPortalB.Y - HalfWidth && OriginPortalA.Y - HalfWidth < OriginPortalB.Y + HalfWidth;
	const bool bIsCollidingVertically = OriginPortalA.Z + HalfHeight > OriginPortalB.Z - HalfHeight && OriginPortalA.Z - HalfHeight < OriginPortalB.Z + HalfHeight;

	return bIsCollidingHorizontally && bIsCollidingVertically;
}

void UPPortalHelper::ResizeRenderTarget(UTextureRenderTarget2D* RenderTarget, const float SizeX, const float SizeY)
{
	if (RenderTarget == nullptr)
		return;

	RenderTarget->ResizeTarget(SizeX, SizeY);
}

FVector UPPortalHelper::ConvertLocationToPortalSpace(const FVector Location, APPortal* OriginPortal, APPortal* TargetPortal)
{
	if (OriginPortal == nullptr || TargetPortal == nullptr)
		return FVector::ZeroVector;

	const FTransform OriginTransform = OriginPortal->GetPortalMesh()->GetComponentTransform();
	const FTransform TargetTransform = TargetPortal->GetPortalMesh()->GetComponentTransform();

	FVector RelativeLocation = OriginTransform.InverseTransformPositionNoScale(Location);
	RelativeLocation.X *= -1; // Flip forward axis.
	RelativeLocation.Y *= -1; // Flip right axis.
	const FVector NewLocation = TargetTransform.TransformPositionNoScale(RelativeLocation);

	return NewLocation;
}

FVector UPPortalHelper::ConvertDirectionToPortalSpace(const FVector Direction, APPortal* OriginPortal, APPortal* TargetPortal)
{
	if (TargetPortal == nullptr)
		return FVector::ZeroVector;

	FVector Dots;
	Dots.X = FVector::DotProduct(Direction, OriginPortal->GetPortalMesh()->GetForwardVector());
	Dots.Y = FVector::DotProduct(Direction, OriginPortal->GetPortalMesh()->GetRightVector());
	Dots.Z = FVector::DotProduct(Direction, OriginPortal->GetPortalMesh()->GetUpVector());

	const FVector NewDirection = Dots.X * -TargetPortal->GetPortalMesh()->GetForwardVector()
								+ Dots.Y * -TargetPortal->GetPortalMesh()->GetRightVector()
								+ Dots.Z * TargetPortal->GetPortalMesh()->GetUpVector();

	return NewDirection;
}

FRotator UPPortalHelper::ConvertRotationToPortalSpace(const FRotator Rotation, APPortal* OriginPortal, APPortal* TargetPortal)
{
	if (OriginPortal == nullptr || TargetPortal == nullptr)
		return FRotator::ZeroRotator;

	const FTransform OriginTransform = OriginPortal->GetPortalMesh()->GetComponentTransform();
	const FTransform TargetTransform = TargetPortal->GetPortalMesh()->GetComponentTransform();
	const FQuat QuatRotation = FQuat(Rotation);

	FRotator RelativeRotation = OriginTransform.InverseTransformRotation(QuatRotation).Rotator();
	RelativeRotation.Yaw += 180.0f; // Flip rotation
	const FQuat NewWorldQuat = TargetTransform.TransformRotation(RelativeRotation.Quaternion());

	return NewWorldQuat.Rotator();
}
