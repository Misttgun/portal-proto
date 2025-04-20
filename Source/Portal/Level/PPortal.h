// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPortal.generated.h"

class APPortalWall;

UCLASS()
class PORTAL_API APPortal : public AActor
{
	GENERATED_BODY()

public:
	APPortal();

	virtual void OnConstruction(const FTransform& Transform) override;

	void Init(bool bIsLeftPortal);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Portal")
	void OnPortalSpawned();

	UPROPERTY()
	APPortalWall* CurrentWall;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PortalBorderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> LeftPortalMaterial;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> RightPortalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextureRenderTarget2D> LeftRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextureRenderTarget2D> RightRenderTarget;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true", ExposeOnSpawn = "true"))
	bool bPortalLeft;
};
