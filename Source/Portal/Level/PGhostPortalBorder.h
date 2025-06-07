// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PGhostPortalBorder.generated.h"

/*
 Actor used to get the mesh's vertices info and calculate portal bound when placing portal on walls.
 */
UCLASS()
class PORTAL_API APGhostPortalBorder : public AActor
{
	GENERATED_BODY()

public:
	APGhostPortalBorder();

	TArray<FVector> GetVertices() const { return Vertices; }
	FRotator GetRelativeRotation() const { return MeshComp->GetRelativeRotation(); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComp;

	TArray<FVector> Vertices;
};
