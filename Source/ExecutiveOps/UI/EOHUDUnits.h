#pragma once

/**
 * Unit conversions shared by the HUD classes.
 *
 * Both HUDs previously kept private copies in their own anonymous namespaces,
 * which collide the moment a unity build merges the two translation units.
 */
namespace EOHUD
{
	/** Unreal works in centimetres; the HUD reports metres. */
	constexpr float CmToM = 0.01f;

	/** cm/s to km/h. */
	constexpr float CmsToKph = 0.036f;
}
