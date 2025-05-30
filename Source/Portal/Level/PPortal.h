// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPortal.generated.h"

class APCharacter;
class APPlayerController;
class APPortalWall;
class UBoxComponent;

/* Logging category for this class. */
DECLARE_LOG_CATEGORY_EXTERN(LogPortal, Log, All);

/* Post-physics update tick for updating position of physics driven actors. 
 * NOTE: This is irrelevant for a pawn that is not physics driven.
 * NOTE: This is always a relevant way of tracking actors that are moving via physics.
 */
USTRUCT()
struct FPostPhysicsTick : public FActorTickFunction
{
	GENERATED_BODY()

	UPROPERTY()
	class APPortal* Target;

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
};

template <>
struct TStructOpsTypeTraits<FPostPhysicsTick> : public TStructOpsTypeTraitsBase2<FPostPhysicsTick>
{
	enum { WithCopy = false };
};

UCLASS()
class PORTAL_API APPortal : public AActor
{
	GENERATED_BODY()

	/* Make post physics friend so it can access the tick function. */
	friend FPostPhysicsTick;
	
public:
	APPortal();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	void Init(bool bIsLeftPortal);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Portal")
	void OnPortalSpawned();

	UFUNCTION(Blueprintcallable, Category = "Portal")
	void LinkPortal(APPortal* OtherPortal);

	/* Update the render texture for this portal using the scene capture component. */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void UpdatePortalView();

	/* Clears the current render texture on the portal meshes dynamic material instance. */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void ClearPortalView() const;

	/* Updates the world offset in the dynamic material instance for the vertexes on the portal mesh when the camera gets too close.
	 * NOTE: Fix for near clipping plane clipping with the portal plane mesh. */
	void UpdateWorldOffset() const;

	UFUNCTION(BlueprintCallable, Category="Portal")
	void TeleportActor(AActor* ActorToTeleport);

	//Helpers
	UFUNCTION(BlueprintCallable, Category="Portal")
	bool IsPointInFrontOfPortal(const FVector& Point) const;

	UFUNCTION(BlueprintCallable, Category="Portal")
	bool IsPointCrossingPortal(const FVector& Point, FVector& OutIntersectionPoint) const;

	UFUNCTION(BlueprintCallable, Category = "Portal")
	bool IsPointInsidePortal(const FVector& Point) const;

	UStaticMeshComponent* GetPortalMesh() const { return PortalMesh; };

	UPROPERTY()
	APPortalWall* CurrentWall;

protected:
	virtual void BeginPlay() override;

	/* Delayed setup function. */
	UFUNCTION()
	void Setup();

	/* Post-physics ticking function. */
	void PostPhysicsTick(float DeltaTime);
	
	/* Create a render texture target for this portal. */
	void CreatePortalTexture();

	/* Updates the pawns tracking for going through portals. Cannot rely on detecting overlaps. */
	void UpdatePawnTracking();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PortalBorderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> PortalBox;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> LeftPortalBorderMaterial;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> RightPortalBorderMaterial;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> PortalMaterialInstance;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> DefaultPortalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true", ExposeOnSpawn = "true"))
	bool bPortalLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	float PortalRenderScale;

	FPostPhysicsTick PhysicsTick;

	UPROPERTY()
	APPortal* TargetPortal;

	UPROPERTY()
	APPlayerController* PlayerController;

	UPROPERTY()
	APCharacter* PlayerCharacter;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY()
	UMaterialInstanceDynamic* PortalMaterial;

private:
	FVector LastPawnPosition; /* The pawns last tracked location for calculating when to teleport the player. */
	
	bool bInitialized;
};
