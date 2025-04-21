// Copyright (c) 2025 Maurel Sagbo


#include "PPortalHelper.h"

#include "Engine/TextureRenderTarget2D.h"

bool UPPortalHelper::IsPortalColliding(const FVector& OriginPortalA, const FVector& OriginPortalB, const FVector2D& PortalSize)
{
	const float HalfWidth = PortalSize.X / 2;
	const float HalfHeight = PortalSize.Y / 2;
	
	const bool bIsCollidingHorizontally = OriginPortalA.Y + HalfWidth > OriginPortalB.Y - HalfWidth && OriginPortalA.Y - HalfWidth < OriginPortalB.Y + HalfWidth;
	const bool bIsCollidingVertically = OriginPortalA.Z + HalfHeight > OriginPortalB.Z - HalfHeight && OriginPortalA.Z - HalfHeight < OriginPortalB.Z + HalfHeight;

	return bIsCollidingHorizontally && bIsCollidingVertically;
}

void UPPortalHelper::ResizeRenderTarget(UTextureRenderTarget2D* RenderTarget, float SizeX, float SizeY)
{
	if (RenderTarget == nullptr)
		return;

	RenderTarget->ResizeTarget(SizeX, SizeY);
}
