#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EOInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * The actions and mapping contexts the two pawns bind to.
 *
 * A data asset, built by Scripts/build_input_assets.py, so the bindings are
 * authorable in the editor and persist. They used to be constructed in C++ with
 * NewObject on every run, which is why two shipped settings on this object
 * could never take effect: the object the editor let you edit was thrown away
 * before anything consulted it. See Docs/adr/0001 and 0008.
 *
 * The generator is the source of truth and the asset is its output, which keeps
 * the bindings readable in a diff - the thing the C++ version was protecting.
 */
UCLASS(BlueprintType)
class EXECUTIVEOPS_API UEOInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Aircraft and operative use separate contexts so only one is ever active. */
	UPROPERTY(EditAnywhere, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> AircraftContext;

	UPROPERTY(EditAnywhere, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> OperativeContext;

	// Shared
	UPROPERTY(EditAnywhere, Category = "Input|Shared")
	TObjectPtr<UInputAction> LookAction;

	/**
	 * Gamepad look, kept separate from the mouse.
	 *
	 * A mouse reports a delta that is already frame-independent; a stick reports
	 * a held position, so it has to be treated as a rate and scaled by delta
	 * time. Feeding both through one action makes stick look frame-rate
	 * dependent and wildly too fast.
	 */
	UPROPERTY(EditAnywhere, Category = "Input|Shared")
	TObjectPtr<UInputAction> LookStickAction;

	// Aircraft. The keys keep their names across both modes and the pawn
	// decides what they mean: A/D strafe in flight and turn in hover, Q/E roll
	// in flight and strafe in hover.
	/** W/S and A/D, as held axes. W/S only count in hover; in flight they are the throttle taps below. */
	UPROPERTY(EditAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> FlightMoveAction;

	/** W tapped: one throttle step up. Shares the key with FlightMoveAction. */
	UPROPERTY(EditAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> ThrottleUpAction;

	/** S tapped: one throttle step down. */
	UPROPERTY(EditAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> ThrottleDownAction;

	/** Shift/Ctrl. */
	UPROPERTY(EditAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> FlightVerticalAction;

	/** Q/E. */
	UPROPERTY(EditAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> FlightRollAction;

	/** Toggles between flight and hover; the view follows. */
	UPROPERTY(EditAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> HoverAction;

	UPROPERTY(EditAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> DeployAction;

	UPROPERTY(EditAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> ToggleMapAction;

	// Operative (M4 owns parkour; M0 needs move/look/jump/sprint present)
	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> SlideAction;

	/** Toggles the crouch stance. Separate from slide, which is a sprint that goes low. */
	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> TakedownAction;

	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> InteractAction;

};
