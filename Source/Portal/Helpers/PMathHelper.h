// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PMathHelper.generated.h"

/**
 * 
 */
UCLASS()
class PORTAL_API UPMathHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool IsPortalColliding(const FVector& OriginPortalA, const FVector& OriginPortalB, const FVector2D& PortalSize);
};
