// Copyright (c) 2025 Maurel Sagbo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPortal.generated.h"

class APCharacter;
class UCameraComponent;
class APPlayerController;
class APPortalWall;
class UBoxComponent;

/* Logging category for this class. */
DECLARE_LOG_CATEGORY_EXTERN(LogPortal, Log, All);

/* Structure to hold important tracking information with each overlapping actor. */
USTRUCT(BlueprintType)
struct FTrackedActor
{
	GENERATED_BODY()

	FVector LastTrackedLocation;

	UPROPERTY()
	USceneComponent* TrackedComp;

	UPROPERTY()
	AActor* TrackedCopy;

	FTrackedActor() : LastTrackedLocation(FVector::ZeroVector), TrackedComp(nullptr), TrackedCopy(nullptr)
	{
	}
};

/* Post-physics update tick for updating position of physics driven actors. 
 NOTE: This is irrelevant for a pawn that is not physics driven.
 NOTE: This is always a relevant way of tracking actors that are moving via physics.
 */
USTRUCT()
struct FPostPhysicsTick : public FActorTickFunction
{
	GENERATED_BODY()

	UPROPERTY()
	class APPortal* Target = nullptr;

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

	UFUNCTION(BlueprintCallable, Category="Portal")
	void TeleportActor(AActor* ActorToTeleport);

	//Helpers
	UFUNCTION(BlueprintCallable, Category="Portal")
	bool IsPointInFrontOfPortal(const FVector& Point) const;

	UFUNCTION(BlueprintCallable, Category="Portal")
	bool IsPointCrossingPortal(const FVector& StartPoint, const FVector& Point, FVector& OutIntersectionPoint) const;

	UStaticMeshComponent* GetPortalMesh() const { return PortalMesh; };
	APPortal* GetLinkedPortal() const { return TargetPortal; };

	UPROPERTY()
	APPortalWall* CurrentWall;

	void UpdatePortalBorderCollision(bool bIsFloorPortal) const;

	// Overlap
	UFUNCTION(Category = "Portal")
	void OnPortalBoxOverlapStart(UPrimitiveComponent* PortalMeshHit, AActor* OverlappedActor, UPrimitiveComponent* OverlappedComp, int32 OtherBodyIndex, bool FromSweep, const FHitResult& PortalHit);

	UFUNCTION(Category = "Portal")
	void OnPortalBoxOverlapEnd(UPrimitiveComponent* PortalMeshHit, AActor* OverlappedActor, UPrimitiveComponent* OverlappedComp, int32 OtherBodyIndex);

	UFUNCTION(Category = "Portal")
	void OnPortalMeshOverlapStart(UPrimitiveComponent* PortalMeshHit, AActor* OverlappedActor, UPrimitiveComponent* OverlappedComp, int32 OtherBodyIndex, bool FromSweep, const FHitResult& PortalHit);

	UFUNCTION(Category = "Portal")
	void OnPortalMeshOverlapEnd(UPrimitiveComponent* PortalMeshHit, AActor* OverlappedActor, UPrimitiveComponent* OverlappedComp, int32 OtherBodyIndex);

	// Portal extents when placed on a wall
	FVector2D Extents;

protected:
	virtual void BeginPlay() override;

	/* Post-physics ticking function. */
	void PostPhysicsTick(float DeltaTime);

	/* Create a render texture target for this portal. */
	void CreatePortalTexture();

	void AddTrackedActor(AActor* ActorToAdd);
	void RemoveTrackedActor(const AActor* ActorToRemove);

	/* Hides a copied version of an actor from the main render pass so it still casts shadows. */
	static void SetCopyVisibility(const AActor* Actor, bool IsVisible);

	void CopyActor(AActor* ActorToCopy);
	void DeleteCopy(const AActor* ActorToDelete);

	void UpdateTrackedActors();

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
	UCameraComponent* PlayerCamera;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY()
	UMaterialInstanceDynamic* PortalMaterial;

	UPROPERTY()
	TMap<AActor*, FTrackedActor> TrackedActors; 

	UPROPERTY()
	TMap<AActor*, AActor*> CopiedActors; 
	
	bool bInitialized;
	int ActorsBeingTracked;
};
