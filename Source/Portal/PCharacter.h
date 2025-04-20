// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "PCharacter.generated.h"

class UPGunComponent;
class UPhysicsHandleComponent;
class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogPortalCharacter, Log, All);

UCLASS(config=Game)
class APCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APCharacter();
	virtual void Tick(float DeltaSeconds) override;

	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComp; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ProcessGrab();

private:
	void GrabActor();
	void ReleaseActor();
	void FindActorToGrab();
	
	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Mesh, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComp;

	// Portal Gun
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPGunComponent> GunComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	FName GunSocketName;

	// Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> GrabAction;

	// Grab
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Grab, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicsHandleComponent> PhysicsHandleComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Grab, meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> CollisionChannel;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Grab, meta = (AllowPrivateAccess = "true"))
	float GrabDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Grab, meta = (AllowPrivateAccess = "true"))
	float TraceDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Grab, meta = (AllowPrivateAccess = "true"))
	float TraceRadius;

	UPROPERTY()
	AActor* FocusedActor;

	bool bIsGrabbingActor;
};
