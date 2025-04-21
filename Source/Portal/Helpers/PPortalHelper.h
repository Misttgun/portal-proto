// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PPortalHelper.generated.h"

/**
 * 
 */
UCLASS()
class PORTAL_API UPPortalHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Portal")
	static bool IsPortalColliding(const FVector& OriginPortalA, const FVector& OriginPortalB, const FVector2D& PortalSize);

	UFUNCTION(BlueprintCallable, Category = "Portal")
	static void ResizeRenderTarget(UTextureRenderTarget2D* RenderTarget, float SizeX, float SizeY);
};
