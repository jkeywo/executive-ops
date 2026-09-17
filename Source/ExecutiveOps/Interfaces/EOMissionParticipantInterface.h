#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interfaces/EOMissionTypes.h"
#include "EOMissionParticipantInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UEOMissionParticipantInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything that needs to react to the mission state machine: the aircraft, the operative,
 * the HUD, and later guards, objectives and the extraction zone.
 *
 * Actors implementing this are found via the mission subsystem's delegates; they do not
 * need to register themselves.
 */
class EXECUTIVEOPS_API IEOMissionParticipantInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mission")
	void OnMissionStateChanged(EEOMissionState OldState, EEOMissionState NewState);
};
