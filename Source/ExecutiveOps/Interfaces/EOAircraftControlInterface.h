#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EOAircraftControlInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UEOAircraftControlInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Everything the player controller needs from an aircraft, independent of how that
 * aircraft is actually implemented. M1 replaces the M0 placeholder pawn behind this.
 */
class EXECUTIVEOPS_API IEOAircraftControlInterface
{
	GENERATED_BODY()

public:
	/** X = forward/back, Y = strafe, Z = vertical. Normalised -1..1 per axis. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft")
	void SetFlightInput(const FVector& MoveInput);

	/** Normalised -1..1. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft")
	void SetYawInput(float YawInput);

	/** Hover hold: precision station-keeping rather than forward flight. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft")
	void SetHoverEnabled(bool bEnabled);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft")
	bool IsHovering() const;

	/** cm/s, for the debug speed readout and later the HUD. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft")
	float GetCurrentSpeed() const;

	/**
	 * Where the operative leaves the aircraft from. M0 returns a single fixed socket;
	 * M3 uses this transform to place and launch the character.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft|Deployment")
	FTransform GetDeploymentSocketTransform() const;

	/** False while moving too fast, outside a hover volume, etc. M0 keys this off hover only. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft|Deployment")
	bool IsReadyForDeployment() const;

	/** Drop all input and accumulated momentum. Used by the reset command. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft")
	void ResetFlightState();

	/**
	 * Assist the player into the deployment position: while enabled and the pilot
	 * is not actively fighting it, the craft drifts to the target point and holds.
	 * This is what makes lining up a drop feel like the aircraft helping rather
	 * than a precision hovering exercise.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft|Deployment")
	void SetStationKeepTarget(const FVector& WorldLocation, bool bEnabled);

	/**
	 * Freeze the craft for the duration of the drop. The aircraft is parked, not
	 * flying itself.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft|Deployment")
	void SetDeploymentHold(bool bHeld);

	/**
	 * Fly to a point under the craft's own power, ignoring player input.
	 *
	 * Deliberately a single destination rather than navigation: a constrained
	 * scripted arrival is what the extraction needs, and a general aircraft AI
	 * is a much larger thing to get wrong.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft|Extraction")
	void SetScriptedDestination(const FVector& WorldLocation, bool bEnabled);

	/** True once a scripted arrival has settled on its destination. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aircraft|Extraction")
	bool HasReachedScriptedDestination() const;
};
