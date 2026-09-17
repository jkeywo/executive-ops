#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/EODeployableInterface.h"
#include "EOOperativeCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UEOInputConfig;
struct FInputActionValue;

/**
 * The operative. M0 is a plain third-person character: move, look, jump, sprint.
 *
 * No parkour, no takedown, no weapon, no perception mode — M4, M5 and M9 add those.
 * The mesh and animation blueprint are assigned in a Blueprint subclass once the
 * production rig and animation packs are imported.
 */
UCLASS()
class EXECUTIVEOPS_API AEOOperativeCharacter : public ACharacter, public IEODeployableInterface
{
	GENERATED_BODY()

public:
	AEOOperativeCharacter();

	//~ IEODeployableInterface
	virtual void OnDeployFrom_Implementation(AActor* SourceAircraft, const FTransform& DeploySocket) override;
	virtual void OnDeployComplete_Implementation() override;
	virtual void OnExtractBegin_Implementation(AActor* TargetAircraft) override;
	virtual void SetStowed_Implementation(bool bStowed) override;
	//~ End IEODeployableInterface

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Landing ends the drop and hands control to the player. */
	virtual void Landed(const FHitResult& Hit) override;
	virtual void UnPossessed() override;

	/** Safety net: a drop that never lands must not lock the player out forever. */
	void OnDropTimedOut();

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Jump(const FInputActionValue& Value);
	void Input_StopJump(const FInputActionValue& Value);
	void Input_SprintStart(const FInputActionValue& Value);
	void Input_SprintStop(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** How long a drop may take before control is forced back to the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Deployment")
	float DropTimeout = 6.f;

	/** Placeholder values. Tuning belongs in M8, not here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 850.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float LookSensitivity = 1.f;

public:
	UFUNCTION(BlueprintPure, Category = "Deployment")
	bool IsStowed() const { return bStowed; }

	/**
	 * Begins the drop: places the operative at the socket, throws it clear of the
	 * aircraft and locks input until it lands.
	 *
	 * The player is possessing the operative for the whole descent, so the camera
	 * rides down with them and the handover reads as one continuous move rather
	 * than a cut.
	 */
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void BeginDeploymentDrop(const FTransform& FromSocket, const FVector& LaunchVelocity);

	/**
	 * Abandon a drop in progress: clears the timeout, unlocks state and leaves
	 * the operative safe to stow. Used when the player is pulled back into the
	 * aircraft or resets mid-descent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void CancelDeploymentDrop();

	/** True from the moment of launch until the operative is on the ground. */
	UFUNCTION(BlueprintPure, Category = "Deployment")
	bool IsDeploying() const { return bDeploying; }

	/** Ground controls are live. */
	UFUNCTION(BlueprintPure, Category = "Deployment")
	bool HasControl() const { return !bStowed && !bDeploying; }

private:
	const UEOInputConfig* GetInputConfig() const;

	bool bStowed = false;
	bool bDeploying = false;
	FTimerHandle DropTimeoutTimer;
};
