#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/EOMissionTypes.h"
#include "EOPlayerController.generated.h"

class AEOAircraftPawn;
class AEOOperativeCharacter;
class UEOInputConfig;
class UEOSelfTest;
class AEOMissionSite;
class AEOExtractionZone;

/**
 * Owns the player's relationship with the two halves of the game: which pawn is
 * possessed, and which input mapping context is active.
 *
 * The deploy/extract entry points here are the seams M3 and M7 grow into. They
 * currently do the minimum: swap pawn, swap context, move the mission state along.
 */
UCLASS()
class EXECUTIVEOPS_API AEOPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEOPlayerController();

	const UEOInputConfig* GetInputConfig() const { return InputConfig; }

	UFUNCTION(BlueprintPure, Category = "Control")
	EEOControlMode GetControlMode() const { return ControlMode; }

	/** Possess the aircraft, swapping to the flight mapping context. */
	UFUNCTION(BlueprintCallable, Category = "Control")
	bool PossessAircraft();

	/** Possess the operative, swapping to the ground mapping context. */
	UFUNCTION(BlueprintCallable, Category = "Control")
	bool PossessOperative();

	/**
	 * Aircraft -> operative. M0: checks the aircraft reports ready, moves the
	 * operative to the deployment socket, possesses it. M3 adds the real sequence.
	 */
	UFUNCTION(BlueprintCallable, Category = "Control")
	bool RequestDeployment();

	/**
	 * Operative -> aircraft.
	 *
	 * With the objective done this calls the aircraft in and hands control back
	 * once it arrives. At any other time it is the immediate debug handover the
	 * cheat manager and the self-test use.
	 */
	UFUNCTION(BlueprintCallable, Category = "Control")
	bool RequestExtraction();

	/** True while the aircraft is inbound to collect the operative. */
	UFUNCTION(BlueprintPure, Category = "Control")
	bool IsExtractionInbound() const { return bExtractionInbound; }

	/** Metres to the inbound aircraft, or -1 when nothing is inbound. */
	UFUNCTION(BlueprintPure, Category = "Control")
	float GetExtractionDistance() const;

	/**
	 * True when everything the drop needs is satisfied: possessing a stable
	 * aircraft, inside the selected site's hover volume. Drives the HUD prompt as
	 * well as the deployment itself, so the two can never disagree.
	 */
	UFUNCTION(BlueprintPure, Category = "Control")
	bool CanDeploy() const;

	/** Why deployment is unavailable, for the HUD prompt. Empty when it is. */
	UFUNCTION(BlueprintPure, Category = "Control")
	FString GetDeploymentBlocker() const;

	UFUNCTION(BlueprintPure, Category = "Control")
	AEOMissionSite* GetSelectedSite() const;

	/**
	 * Put the mission back to a runnable state after it has closed out.
	 *
	 * Called automatically a beat after Complete or Failed, so the player can fly
	 * another run without typing a console command - which is precisely what M6
	 * means by "repeatedly without debug intervention".
	 */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void RearmMission();

	/**
	 * Wash the screen out to a colour and fade it back.
	 *
	 * Exists to hide a cut. Deployment swaps pawn, camera and HUD in one frame,
	 * and without something over the top the player sees the join.
	 *
	 * Holds at full for HoldSeconds before fading, so the swap itself lands
	 * inside the opaque part rather than during the ramp.
	 */
	UFUNCTION(BlueprintCallable, Category = "Feedback")
	void TriggerScreenFlash(const FLinearColor& Colour, float HoldSeconds, float FadeSeconds);

	/** 0 when nothing is flashing. */
	UFUNCTION(BlueprintPure, Category = "Feedback")
	float GetScreenFlashAlpha() const { return FlashAlpha; }

	UFUNCTION(BlueprintPure, Category = "Feedback")
	FLinearColor GetScreenFlashColour() const { return FlashColour; }

	/** Unwind a deployment in progress and put the player back in the aircraft. */
	UFUNCTION(BlueprintCallable, Category = "Control")
	void AbortDeployment();

	/** Reset everything to the boot state without reloading the level. */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void EOReset();

protected:
	/**
	 * Input config is built here rather than in BeginPlay: the game mode possesses
	 * the default pawn before BeginPlay runs, and that pawn's
	 * SetupPlayerInputComponent needs the config to already exist.
	 */
	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Enables the aircraft's assist while the player is lined up over the site. */
	void UpdateDeploymentAssist();

	/** Watches an inbound aircraft and performs the pickup when it arrives. */
	void UpdateExtraction(float DeltaSeconds);

	void UpdateScreenFlash(float DeltaSeconds);

	/** Starts the scripted arrival over the operative. */
	bool BeginExtractionPickup();

	/** Board, hand control back, and close the mission out. */
	void CompleteExtractionPickup();

	/** Stop an inbound arrival and hand the craft back to the player. */
	void CancelExtraction();

	/** Entry point for the -EOSelfTest command-line switch. */
	void RunSelfTest();

	/**
	 * Grabs a screenshot a few seconds after boot and exits.
	 *
	 * Enabled with -EOScreenshot. The self-test suites run with -nullrhi and can
	 * prove behaviour but never show anything, so this is the only way to check
	 * that something actually looks right.
	 */
	void TakeDebugScreenshot();

	/** Second stage of the screenshot: capture, then exit. */
	void CaptureDebugScreenshot();

	UFUNCTION()
	void HandleMissionStateChanged(EEOMissionState OldState, EEOMissionState NewState);

	/** Seconds the outcome is left on screen before the mission re-arms. */
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	float RearmDelay = 3.f;

	/** Finds the first aircraft/operative in the level, or spawns one if absent. */
	AEOAircraftPawn* ResolveAircraft();
	AEOOperativeCharacter* ResolveOperative();

	void ApplyMappingContext(EEOControlMode Mode);

	/** Downward and forward throw applied to the operative as it leaves the craft. */
	UPROPERTY(EditDefaultsOnly, Category = "Control|Deployment")
	float DeployLaunchDown = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "Control|Deployment")
	float DeployLaunchForward = 450.f;

	/** The deployment wash. White, because the drop is a violent, lit-up moment. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Flash")
	FLinearColor DeployFlashColour = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Flash", meta = (ClampMin = "0"))
	float DeployFlashHold = 0.07f;

	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Flash", meta = (ClampMin = "0"))
	float DeployFlashFade = 0.42f;

	/** How high above the operative the aircraft holds to collect them. */
	UPROPERTY(EditDefaultsOnly, Category = "Control|Extraction")
	float ExtractionHoverHeight = 700.f;

	/** Seconds the pickup itself takes once the craft is overhead. */
	UPROPERTY(EditDefaultsOnly, Category = "Control|Extraction")
	float PickupDuration = 1.2f;

	/** Give up waiting and hand control back rather than stranding the player. */
	UPROPERTY(EditDefaultsOnly, Category = "Control|Extraction")
	float ExtractionTimeout = 25.f;

	/** Classes used when the level contains no placed pawn. */
	UPROPERTY(EditDefaultsOnly, Category = "Control")
	TSubclassOf<AEOAircraftPawn> AircraftClass;

	UPROPERTY(EditDefaultsOnly, Category = "Control")
	TSubclassOf<AEOOperativeCharacter> OperativeClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEOInputConfig> InputConfig;

	UPROPERTY(Transient)
	TObjectPtr<UEOSelfTest> SelfTest;

	FTimerHandle RearmTimer;

	bool bExtractionInbound = false;

	FLinearColor FlashColour = FLinearColor::White;
	float FlashAlpha = 0.f;
	float FlashHoldRemaining = 0.f;
	float FlashFadeSeconds = 0.f;
	float ExtractionElapsed = 0.f;
	float PickupElapsed = 0.f;
	FVector ExtractionPoint = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<AEOAircraftPawn> Aircraft;

	UPROPERTY(Transient)
	TObjectPtr<AEOOperativeCharacter> Operative;

	/** Where each pawn started, so EOReset can put them back. */
	FTransform AircraftStartTransform;
	FTransform OperativeStartTransform;

	EEOControlMode ControlMode = EEOControlMode::None;
};
