#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Interfaces/EODeployableInterface.h"
#include "EOOperativeCharacter.generated.h"

class UAnimMontage;
class UAnimSequence;
class UCameraComponent;
class UStaticMeshComponent;
class UEOTraversalComponent;
class UEOHealthComponent;
class UEOWeaponComponent;
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
	void Input_LookStick(const FInputActionValue& Value);
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

	/** Damage is the one event the player must be able to locate, not just hear. */
	UFUNCTION()
	void HandleDamaged(float Amount, AActor* DamageInstigator, const UDamageType* DamageType);

	/** Picks the impact preset for what the shot actually hit. */
	static FGameplayTag SurfaceEventFor(const FHitResult& Hit, bool bHitCharacter);

	void TickCombat(float DeltaSeconds);

	/** Chooses and plays the locomotion clip that matches what the body is doing. */
	void UpdateLocomotionAnimation();

	/**
	 * True when this character drives clips itself, false when an Animation
	 * Blueprint owns the pose. Every direct PlayAnimation call is gated on it.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation")
	bool UsesDirectAnimationPlayback() const;


	void TickSlide(float DeltaSeconds);

	/**
	 * Eases the view round behind the direction of travel once the player stops
	 * steering it. Without this the operative turns to face a strafe and runs off
	 * sideways while the camera stares at where they used to be.
	 */
	void UpdateFollowCamera(float DeltaSeconds);

	/** Switches between the free-movement and aiming control schemes. */
	void SetAiming(bool bNewAiming);

	/** Decides each frame whether the pistol belongs in the hand or on the hip. */
	void UpdateWeaponAttachment(float DeltaSeconds);

	/** Moves the pistol between the two sockets. No-op if it is already there. */
	void ApplyWeaponAttachment(bool bDrawn);

	/** Suspends the auto-follow for a moment after any deliberate look input. */
	void MarkLookInput(const FVector2D& Axis);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UEOTraversalComponent> Traversal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UEOHealthComponent> Health;

	/** The shot itself: cooldown, spread, trace, damage. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<class UEOWeaponComponent> Weapon;

	/**
	 * Registers the operative as something AI sight can pick up.
	 *
	 * Explicit rather than auto-registering every pawn, so what is meant to be
	 * seen is stated rather than inferred - the aircraft is a pawn too.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception")
	TObjectPtr<class UAIPerceptionStimuliSourceComponent> PerceptionSource;

	/** The pistol itself. Rides a bone at all times; only which bone changes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	TObjectPtr<UStaticMeshComponent> PistolMesh;

	// ---- Weapon carry ----------------------------------------------------------
	// Bone names rather than sockets, so the pistol works on any mannequin-derived
	// skeleton without someone first authoring sockets on it.

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	FName HolsterSocket = TEXT("thigh_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	FName GripSocket = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	FVector HolsterOffset = FVector(4.f, 6.f, -12.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	FRotator HolsterRotation = FRotator(0.f, 0.f, 90.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	FVector GripOffset = FVector(-2.f, 4.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon")
	FRotator GripRotation = FRotator(0.f, 90.f, 0.f);

	/** How long the pistol stays out after a shot fired without aiming. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Weapon", meta = (ClampMin = "0"))
	float HolsterDelay = 3.f;

	// ---- Takedown ------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Takedown")
	float TakedownDuration = 0.7f;

	/** Upper bound on interaction range; each interactable may ask for less. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	float MaxInteractionRange = 600.f;

	// Montages are what the Animation Blueprint plays; the sequences below remain
	// as the fallback for a project with no graph prepared yet.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> TakedownMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> TakedownAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> AimAnim;

	/**
	 * Aiming holds the body toward the crosshair and strafes, so the legs need
	 * a clip per direction. Without these, aiming froze the operative in a
	 * standing idle that slid around the floor.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> AimStrafeForward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> AimStrafeBackward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> AimStrafeLeft;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> AimStrafeRight;

	/**
	 * Fallback ground speed for a locomotion clip, used only when the clip has no
	 * root travel to measure.
	 *
	 * Clips play at a fixed rate, so a run cycle authored for 3.3m/s looks like
	 * slow motion while the operative is still accelerating. Scaling the play
	 * rate by actual speed keeps the feet with the ground - but only if the
	 * authored speed is right, and hand-entered numbers here were wrong by up to
	 * a factor of two. GetClipSpeed measures the real figure off the asset; these
	 * remain for in-place clips, which have nothing to measure.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	float WalkAnimSpeed = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	float JogAnimSpeed = 450.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	float SprintAnimSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	float StrafeAnimSpeed = 300.f;

	/** Clamp, so a crawl does not stall the clip and a sprint does not gabble it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	float MinAnimPlayRate = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement|Animation")
	float MaxAnimPlayRate = 1.6f;

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

	// ---- Follow camera ---------------------------------------------------------

	/** Degrees per second the view swings behind the heading, at full sprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Follow", meta = (ClampMin = "0"))
	float CameraFollowRate = 110.f;

	/** Below this speed the operative is shuffling, and the camera stays put. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Follow", meta = (ClampMin = "0"))
	float CameraFollowMinSpeed = 180.f;

	/** Degrees of slack around the heading, so the view does not hunt. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Follow", meta = (ClampMin = "0"))
	float CameraFollowDeadZone = 6.f;

	/** Seconds the auto-follow stays out of the way after a look input. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Follow", meta = (ClampMin = "0"))
	float LookHoldTime = 0.6f;

	/** How long a movement press keeps the auto-follow live after it is released. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Follow", meta = (ClampMin = "0"))
	float MoveInputHoldTime = 0.15f;

	/** Vertical impact speed above which a landing counts as hard. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0"))
	float HardLandingSpeed = 900.f;

	/** How long a drop may take before control is forced back to the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Deployment")
	float DropTimeout = 6.f;

	/**
	 * Matched to the locomotion blend space, not chosen freely: its samples are
	 * authored at 180 (walk), 500 (jog) and 950 (run) cm/s, the speeds the clips
	 * were captured at. Land on a sample and the feet do not slide; sit between
	 * two and they do. 500 is the jog sample, 950 the run.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 950.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float LookSensitivity = 1.f;

	/** Degrees per second at full stick deflection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float StickLookRate = 140.f;

public:
	/**
	 * Every clip driven by ground speed, for checks that care about all of them.
	 *
	 * These are the clips whose root motion has to stay locked: they are the ones
	 * with metres of travel baked into the root track.
	 */
	TArray<UAnimSequence*> GetLocomotionClips() const;

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

	/** The pistol. Exposed so the harness can assert the shot without the dice. */
	UFUNCTION(BlueprintPure, Category = "Combat")
	UEOWeaponComponent* GetWeapon() const { return Weapon; }

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

	/**
	 * Plays a one-shot action: as a montage through the graph's slot when there
	 * is a graph, and as a direct clip when there is not.
	 *
	 * Public because the traversal component plays its own clips through it,
	 * rather than keeping a second copy of the same rule.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool PlayActionAnimation(UAnimMontage* Montage, UAnimSequence* Fallback);

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
	bool bWeaponDrawn = false;
	float HolsterDelayRemaining = 0.f;
	float LookHoldRemaining = 0.f;
	float MoveInputHoldRemaining = 0.f;

	/** The last move axis, so the follow can tell steering from strafing. */
	FVector2D LastMoveAxis = FVector2D::ZeroVector;
	float TakedownRemaining = 0.f;

	/**
	 * Ground speed a clip actually travels at, measured off its root track.
	 *
	 * Measured once per clip and kept, because extraction decompresses the whole
	 * root track and this runs every frame.
	 */
	float GetClipSpeed(UAnimSequence* Clip, float Fallback);

	UPROPERTY(Transient)
	TMap<TObjectPtr<UAnimSequence>, float> MeasuredClipSpeeds;

	/** The clip currently playing, so the same one is not restarted every frame. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentAnim;
	FTimerHandle DropTimeoutTimer;

	/**
	 * Ends a drop: clears the timeout and the flag, and reports whether one was
	 * actually running.
	 *
	 * The pair used to be cleared in three places, and stowing - which is how a
	 * drop is abandoned when the player flies off - cleared neither. That left
	 * bDeploying stuck true with a live timer, and HasControl() false forever.
	 * One writer, so the two cannot drift apart.
	 */
	bool EndDeploymentDrop();
};
