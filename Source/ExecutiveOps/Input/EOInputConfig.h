#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EOInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Input Actions and Mapping Contexts declared in C++ rather than as .uasset files.
 *
 * This keeps M0 entirely in source control as text: a fresh clone boots and is playable
 * with no content to import. If a designer later wants to rebind in-editor, replace this
 * object with equivalent Data Assets — the pawns only ever ask for actions by name.
 */
UCLASS()
class EXECUTIVEOPS_API UEOInputConfig : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Builds the actions and mapping contexts. Must be called explicitly after
	 * NewObject - this cannot happen in the constructor, because UE forbids
	 * unnamed NewObject calls inside a UObject constructor.
	 *
	 * Idempotent.
	 */
	void BuildRuntimeInput();

	/**
	 * Vertical look direction.
	 *
	 * Mouse and stick get separate flags because the two have opposite
	 * conventions and players routinely want one inverted and not the other.
	 */
	UPROPERTY(EditAnywhere, Category = "Input|Look")
	bool bInvertMouseY = false;

	UPROPERTY(EditAnywhere, Category = "Input|Look")
	bool bInvertStickY = false;

	/** Aircraft and operative use separate contexts so only one is ever active. */
	UPROPERTY(VisibleAnywhere, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> AircraftContext;

	UPROPERTY(VisibleAnywhere, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> OperativeContext;

	// Shared
	UPROPERTY(VisibleAnywhere, Category = "Input|Shared")
	TObjectPtr<UInputAction> LookAction;

	/**
	 * Gamepad look, kept separate from the mouse.
	 *
	 * A mouse reports a delta that is already frame-independent; a stick reports
	 * a held position, so it has to be treated as a rate and scaled by delta
	 * time. Feeding both through one action makes stick look frame-rate
	 * dependent and wildly too fast.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Input|Shared")
	TObjectPtr<UInputAction> LookStickAction;

	// Aircraft (M1 owns the handling; M0 only needs the bindings to exist)
	UPROPERTY(VisibleAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> FlightMoveAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> FlightVerticalAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> FlightYawAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> HoverAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> DeployAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Aircraft")
	TObjectPtr<UInputAction> ToggleMapAction;

	// Operative (M4 owns parkour; M0 needs move/look/jump/sprint present)
	UPROPERTY(VisibleAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> SlideAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> TakedownAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(VisibleAnywhere, Category = "Input|Operative")
	TObjectPtr<UInputAction> InteractAction;

private:
	/** Builds the two mapping contexts with keyboard/mouse defaults. */
	void BuildDefaultMappings();

	bool bBuilt = false;
};
