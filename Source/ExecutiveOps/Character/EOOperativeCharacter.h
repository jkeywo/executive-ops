#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/EODeployableInterface.h"
#include "EOOperativeCharacter.generated.h"

class UAnimSequence;
class UCameraComponent;
class UEOTraversalComponent;
class UEOHealthComponent;
class AEOGuardCharacter;
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
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/** Safety net: a drop that never lands must not lock the player out forever. */
	void OnDropTimedOut();

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Jump(const FInputActionValue& Value);
	void Input_StopJump(const FInputActionValue& Value);
	void Input_SprintStart(const FInputActionValue& Value);
	void Input_SprintStop(const FInputActionValue& Value);
	void Input_SlideStart(const FInputActionValue& Value);
	void Input_SlideStop(const FInputActionValue& Value);
	void Input_Takedown(const FInputActionValue& Value);
	void Input_Fire(const FInputActionValue& Value);
	void Input_AimStart(const FInputActionValue& Value);
	void Input_AimStop(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);

	UFUNCTION()
	void HandleDied(AActor* Killer);

	void TickCombat(float DeltaSeconds);

	/** Chooses and plays the locomotion clip that matches what the body is doing. */
	void UpdateLocomotionAnimation();

	void TickSlide(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UEOTraversalComponent> Traversal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UEOHealthComponent> Health;

	// ---- Weapon: one pistol, kept deliberately simple ------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	float WeaponDamage = 55.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	float WeaponRange = 8000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	float WeaponInterval = 0.28f;

	/** Hip spread in degrees. Aiming removes it, which is the only reason to aim. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	float HipSpread = 4.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	float AimSpread = 0.4f;

	// ---- Takedown ------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Takedown")
	float TakedownDuration = 0.7f;

	/** Upper bound on interaction range; each interactable may ask for less. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	float MaxInteractionRange = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> TakedownAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> AimAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> FireAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> DeathAnim;

	// ---- Slide ----------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideEntrySpeed = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideImpulse = 1150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideFriction = 700.f;

	/** Below this the slide ends and the operative stands back up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideExitSpeed = 320.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideMaxDuration = 1.6f;

	// ---- Locomotion animation, assigned in the Blueprint from the owned packs --

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	TObjectPtr<UAnimSequence> IdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	TObjectPtr<UAnimSequence> WalkAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	TObjectPtr<UAnimSequence> JogAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	TObjectPtr<UAnimSequence> SprintAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	TObjectPtr<UAnimSequence> FallAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	TObjectPtr<UAnimSequence> SlideAnim;

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

	UFUNCTION(BlueprintPure, Category = "Movement")
	UEOTraversalComponent* GetTraversal() const { return Traversal; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSprinting() const { return bSprinting; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSliding() const { return bSliding; }

	/** Sprinting into a crouch press starts a slide; otherwise it just crouches. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool TryStartSlide();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSlide();

	UFUNCTION(BlueprintPure, Category = "Combat")
	UEOHealthComponent* GetHealth() const { return Health; }

	/** The guard currently positioned for a silent kill, if any. */
	UFUNCTION(BlueprintPure, Category = "Combat")
	AEOGuardCharacter* FindTakedownTarget() const;

	/** Kill a valid takedown target. Returns false if there is not one. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool TryTakedown();

	/** Hitscan from the camera. Returns true if a shot was actually fired. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool FireWeapon();

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAiming() const { return bAiming; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsDead() const;

	/**
	 * Return to a live, controllable state after death.
	 *
	 * Reviving health alone is not enough: the death path disables movement, so
	 * a revived operative would stand frozen in its death pose forever.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ResetOperative();

	/**
	 * The nearest thing in range that is willing to be interacted with, or null.
	 * Drives both the prompt and the action, so the HUD can never offer something
	 * the key would then refuse.
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* FindInteractable() const;

	/** Interact with whatever FindInteractable returns. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	/** True during the takedown animation, when the player is committed. */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsPerformingTakedown() const { return TakedownRemaining > 0.f; }

private:
	const UEOInputConfig* GetInputConfig() const;

	bool bStowed = false;
	bool bDeploying = false;
	bool bSprinting = false;
	bool bSliding = false;
	float SlideElapsed = 0.f;
	FVector SlideDirection = FVector::ZeroVector;

	/**
	 * The slide owns its own speed. Reading it back off the movement component
	 * each frame instead would measure whatever ground braking had already taken
	 * away, and the slide would die in a fraction of a second.
	 */
	float SlideSpeed = 0.f;

	/** Movement settings restored when the slide ends. */
	float CachedGroundFriction = 8.f;
	float CachedBrakingDeceleration = 2000.f;

	bool bAiming = false;
	float FireCooldown = 0.f;
	float TakedownRemaining = 0.f;

	/** The clip currently playing, so the same one is not restarted every frame. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentAnim;
	FTimerHandle DropTimeoutTimer;
};
