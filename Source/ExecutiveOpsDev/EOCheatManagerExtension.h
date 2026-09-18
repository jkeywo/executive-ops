#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "EOCheatManagerExtension.generated.h"

/**
 * Console commands for driving the loop by hand while the real triggers do not
 * exist yet. Available via the ` console.
 *
 * An extension rather than a UCheatManager subclass, so the player controller
 * does not have to name it as its CheatClass - the engine hands every cheat
 * manager it creates to whoever registered for them, and this module registers
 * on startup. That is what lets the cheats live outside the game module.
 */
UCLASS()
class EXECUTIVEOPSDEV_API UEOCheatManagerExtension : public UCheatManagerExtension
{
	GENERATED_BODY()

public:
	/** Return pawns to their start transforms and reset mission state. */
	UFUNCTION(Exec)
	void EOReset();

	/** Hard restart: reload the current level. */
	UFUNCTION(Exec)
	void EORestartLevel();

	UFUNCTION(Exec)
	void EOPossessAircraft();

	UFUNCTION(Exec)
	void EOPossessOperative();

	/** Force the deployment transition, bypassing the readiness check. */
	UFUNCTION(Exec)
	void EODeploy();

	UFUNCTION(Exec)
	void EOExtract();

	UFUNCTION(Exec)
	void EOToggleDebugHUD();

	/** Show/hide the player HUD frame. */
	UFUNCTION(Exec)
	void EOToggleHUD();

	/** Show/hide the tactical map. */
	UFUNCTION(Exec)
	void EOToggleMap();

	/** Select the only mission site in the level and begin the approach. */
	UFUNCTION(Exec)
	void EOSelectMission();

	/** Print the current mission state to the log. */
	UFUNCTION(Exec)
	void EOMissionState();
};
