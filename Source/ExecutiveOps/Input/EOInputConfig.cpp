#include "Input/EOInputConfig.h"

#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace
{
	/** Negate modifier helper — used to turn a key press into a negative axis value. */
	UInputModifierNegate* MakeNegate(UObject* Outer, bool bX, bool bY, bool bZ)
	{
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Outer);
		Negate->bX = bX;
		Negate->bY = bY;
		Negate->bZ = bZ;
		return Negate;
	}

	/** Moves a 1D key value onto the Y axis of a 2D action, matching the engine templates. */
	UInputModifierSwizzleAxis* MakeSwizzleYXZ(UObject* Outer)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Outer);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		return Swizzle;
	}

	UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = ValueType;
		return Action;
	}
}

void UEOInputConfig::BuildRuntimeInput()
{
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;

	LookAction           = MakeAction(this, TEXT("IA_Look"),           EInputActionValueType::Axis2D);
	LookStickAction      = MakeAction(this, TEXT("IA_LookStick"),      EInputActionValueType::Axis2D);

	FlightMoveAction     = MakeAction(this, TEXT("IA_FlightMove"),     EInputActionValueType::Axis2D);
	FlightVerticalAction = MakeAction(this, TEXT("IA_FlightVertical"), EInputActionValueType::Axis1D);
	FlightYawAction      = MakeAction(this, TEXT("IA_FlightYaw"),      EInputActionValueType::Axis1D);
	HoverAction          = MakeAction(this, TEXT("IA_Hover"),          EInputActionValueType::Boolean);
	DeployAction         = MakeAction(this, TEXT("IA_Deploy"),         EInputActionValueType::Boolean);
	ToggleMapAction      = MakeAction(this, TEXT("IA_ToggleMap"),      EInputActionValueType::Boolean);
	ToggleViewAction     = MakeAction(this, TEXT("IA_ToggleView"),     EInputActionValueType::Boolean);

	MoveAction           = MakeAction(this, TEXT("IA_Move"),           EInputActionValueType::Axis2D);
	JumpAction           = MakeAction(this, TEXT("IA_Jump"),           EInputActionValueType::Boolean);
	SprintAction         = MakeAction(this, TEXT("IA_Sprint"),         EInputActionValueType::Boolean);
	SlideAction          = MakeAction(this, TEXT("IA_Slide"),          EInputActionValueType::Boolean);
	TakedownAction       = MakeAction(this, TEXT("IA_Takedown"),       EInputActionValueType::Boolean);
	FireAction           = MakeAction(this, TEXT("IA_Fire"),           EInputActionValueType::Boolean);
	AimAction            = MakeAction(this, TEXT("IA_Aim"),            EInputActionValueType::Boolean);
	InteractAction       = MakeAction(this, TEXT("IA_Interact"),       EInputActionValueType::Boolean);

	AircraftContext  = NewObject<UInputMappingContext>(this, TEXT("IMC_Aircraft"));
	OperativeContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Operative"));

	BuildDefaultMappings();
}

void UEOInputConfig::BuildDefaultMappings()
{
	// --- Aircraft -------------------------------------------------------------
	// WASD translates, Space/Ctrl climbs and descends, Q/E yaws, Shift holds hover.
	AircraftContext->MapKey(FlightMoveAction, EKeys::W).Modifiers.Add(MakeSwizzleYXZ(this));
	{
		FEnhancedActionKeyMapping& Back = AircraftContext->MapKey(FlightMoveAction, EKeys::S);
		Back.Modifiers.Add(MakeSwizzleYXZ(this));
		Back.Modifiers.Add(MakeNegate(this, true, true, true));
	}
	AircraftContext->MapKey(FlightMoveAction, EKeys::A).Modifiers.Add(MakeNegate(this, true, true, true));
	AircraftContext->MapKey(FlightMoveAction, EKeys::D);

	AircraftContext->MapKey(FlightVerticalAction, EKeys::SpaceBar);
	AircraftContext->MapKey(FlightVerticalAction, EKeys::LeftControl).Modifiers.Add(MakeNegate(this, true, true, true));

	AircraftContext->MapKey(FlightYawAction, EKeys::E);
	AircraftContext->MapKey(FlightYawAction, EKeys::Q).Modifiers.Add(MakeNegate(this, true, true, true));

	AircraftContext->MapKey(HoverAction, EKeys::LeftShift);
	AircraftContext->MapKey(DeployAction, EKeys::F);
	AircraftContext->MapKey(ToggleMapAction, EKeys::M);
	AircraftContext->MapKey(ToggleViewAction, EKeys::V);

	// Mouse Y is negated only when the player asks for it. Previously this was
	// hard-negated, which is what made vertical look feel inverted.
	{
		FEnhancedActionKeyMapping& Look = AircraftContext->MapKey(LookAction, EKeys::Mouse2D);
		if (bInvertMouseY)
		{
			Look.Modifiers.Add(MakeNegate(this, false, true, false));
		}
	}

	// ---- Gamepad -------------------------------------------------------------
	// Left stick flies, right stick looks, triggers climb and descend, shoulders
	// yaw. Deliberately mirrors the keyboard layout rather than inventing a
	// second control scheme.
	AircraftContext->MapKey(FlightMoveAction, EKeys::Gamepad_Left2D);

	AircraftContext->MapKey(FlightVerticalAction, EKeys::Gamepad_RightTriggerAxis);
	AircraftContext->MapKey(FlightVerticalAction, EKeys::Gamepad_LeftTriggerAxis)
		.Modifiers.Add(MakeNegate(this, true, true, true));

	AircraftContext->MapKey(FlightYawAction, EKeys::Gamepad_RightShoulder);
	AircraftContext->MapKey(FlightYawAction, EKeys::Gamepad_LeftShoulder)
		.Modifiers.Add(MakeNegate(this, true, true, true));

	AircraftContext->MapKey(HoverAction, EKeys::Gamepad_FaceButton_Left);
	AircraftContext->MapKey(DeployAction, EKeys::Gamepad_FaceButton_Bottom);
	AircraftContext->MapKey(ToggleMapAction, EKeys::Gamepad_FaceButton_Top);
	AircraftContext->MapKey(ToggleViewAction, EKeys::Gamepad_RightThumbstick);

	{
		FEnhancedActionKeyMapping& Stick =
			AircraftContext->MapKey(LookStickAction, EKeys::Gamepad_Right2D);
		if (bInvertStickY)
		{
			Stick.Modifiers.Add(MakeNegate(this, false, true, false));
		}
	}

	// --- Operative ------------------------------------------------------------
	OperativeContext->MapKey(MoveAction, EKeys::W).Modifiers.Add(MakeSwizzleYXZ(this));
	{
		FEnhancedActionKeyMapping& Back = OperativeContext->MapKey(MoveAction, EKeys::S);
		Back.Modifiers.Add(MakeSwizzleYXZ(this));
		Back.Modifiers.Add(MakeNegate(this, true, true, true));
	}
	OperativeContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(MakeNegate(this, true, true, true));
	OperativeContext->MapKey(MoveAction, EKeys::D);

	OperativeContext->MapKey(JumpAction, EKeys::SpaceBar);
	OperativeContext->MapKey(SprintAction, EKeys::LeftShift);

	// Jump is the contextual parkour button: it vaults, mantles or climbs when
	// there is something there, and jumps when there is not. Two bindings on one
	// key would both fire, so the decision lives in one handler instead.
	OperativeContext->MapKey(SlideAction, EKeys::LeftControl);
	OperativeContext->MapKey(SlideAction, EKeys::C);

	OperativeContext->MapKey(TakedownAction, EKeys::F);
	OperativeContext->MapKey(FireAction, EKeys::LeftMouseButton);
	OperativeContext->MapKey(AimAction, EKeys::RightMouseButton);
	OperativeContext->MapKey(InteractAction, EKeys::E);

	{
		FEnhancedActionKeyMapping& Look = OperativeContext->MapKey(LookAction, EKeys::Mouse2D);
		if (bInvertMouseY)
		{
			Look.Modifiers.Add(MakeNegate(this, false, true, false));
		}
	}

	// ---- Gamepad -------------------------------------------------------------
	OperativeContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	OperativeContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	OperativeContext->MapKey(SlideAction, EKeys::Gamepad_FaceButton_Right);
	OperativeContext->MapKey(TakedownAction, EKeys::Gamepad_FaceButton_Top);
	OperativeContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
	OperativeContext->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);

	OperativeContext->MapKey(AimAction, EKeys::Gamepad_LeftTrigger);
	OperativeContext->MapKey(FireAction, EKeys::Gamepad_RightTrigger);

	{
		FEnhancedActionKeyMapping& Stick =
			OperativeContext->MapKey(LookStickAction, EKeys::Gamepad_Right2D);
		if (bInvertStickY)
		{
			Stick.Modifiers.Add(MakeNegate(this, false, true, false));
		}
	}
}
