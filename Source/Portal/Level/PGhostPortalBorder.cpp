// Copyright (c) 2025 Maurel Sagbo


#include "PGhostPortalBorder.h"

APGhostPortalBorder::APGhostPortalBorder()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("SceneRoot");

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetRelativeRotation(FRotator(-90.0f, 0, 0.0f));
	MeshComp->SetupAttachment(RootComponent);
}

void APGhostPortalBorder::BeginPlay()
{
	Super::BeginPlay();

	if (const TObjectPtr<UStaticMesh> Mesh = MeshComp->GetStaticMesh())
	{
		const FStaticMeshLODResources& LOD = Mesh->GetRenderData()->LODResources[0];

		const FStaticMeshSection& Section = LOD.Sections[0];
		const uint32 OnePastLastIndex = Section.FirstIndex + Section.NumTriangles * 3;
		FIndexArrayView Indices = LOD.IndexBuffer.GetArrayView();

		for (uint32 i = Section.FirstIndex; i < OnePastLastIndex; i++)
		{
			const uint32 MeshVertIndex = Indices[i];
			Vertices.Add(FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(MeshVertIndex)));
		}
	}
}
