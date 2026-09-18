#pragma once

/**
 * Unit conversions and bearing helpers shared by everything that draws the HUD.
 *
 * Both HUDs previously kept private copies in their own anonymous namespaces,
 * which collide the moment a unity build merges the two translation units - and
 * the view model, which is neither HUD, needs the same arithmetic.
 */
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace EOHUD
{
	/** Unreal works in centimetres; the HUD reports metres. */
	constexpr float CmToM = 0.01f;

	/** cm/s to km/h. */
	constexpr float CmsToKph = 0.036f;

	/** Degrees clockwise from ReferenceYaw to the direction of To from From, -180..180. */
	inline float RelativeBearing(const FVector& From, const FVector& To, float ReferenceYaw)
	{
		const FVector Offset = To - From;
		const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
		return FMath::FindDeltaAngleDegrees(ReferenceYaw, TargetYaw);
	}

	/**
	 * The yaw everything on screen is measured against.
	 *
	 * The craft's nose is the heading while flying - free-look swings the camera
	 * without changing where the aircraft is going. On foot the two are the same
	 * thing, and the view is the one the arcs have to agree with.
	 */
	inline float ReferenceYaw(const APawn& Pawn, const APlayerController& Controller, bool bFlying)
	{
		return bFlying ? Pawn.GetActorRotation().Yaw : Controller.GetControlRotation().Yaw;
	}
}
