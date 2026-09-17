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

	FlightMoveAction     = MakeAction(this, TEXT("IA_FlightMove"),     EInputActionValueType::Axis2D);
	FlightVerticalAction = MakeAction(this, TEXT("IA_FlightVertical"), EInputActionValueType::Axis1D);
	FlightYawAction      = MakeAction(this, TEXT("IA_FlightYaw"),      EInputActionValueType::Axis1D);
	HoverAction          = MakeAction(this, TEXT("IA_Hover"),          EInputActionValueType::Boolean);
	DeployAction         = MakeAction(this, TEXT("IA_Deploy"),         EInputActionValueType::Boolean);

	MoveAction           = MakeAction(this, TEXT("IA_Move"),           EInputActionValueType::Axis2D);
	JumpAction           = MakeAction(this, TEXT("IA_Jump"),           EInputActionValueType::Boolean);
	SprintAction         = MakeAction(this, TEXT("IA_Sprint"),         EInputActionValueType::Boolean);

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

	AircraftContext->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(MakeNegate(this, false, true, false));

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

	OperativeContext->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(MakeNegate(this, false, true, false));
}
