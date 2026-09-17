#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "EOTakedownDamageType.generated.h"

/**
 * Marks damage as a takedown rather than a shot.
 *
 * A guard that is shot looks around for whoever shot it. A guard that is taken
 * down silently must not, or stealth stops working - so the two have to be
 * distinguishable at the point damage arrives.
 *
 * That distinction used to be which function was called: Kill() bypassed the
 * damage path entirely so it never fired OnDamaged. Now that everything goes
 * through Unreal's damage pipeline, it is carried as data on the event instead,
 * which is what damage types are for. See Docs/adr/0004.
 */
UCLASS()
class EXECUTIVEOPS_API UEOTakedownDamageType : public UDamageType
{
	GENERATED_BODY()
};
