#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/EOMissionTypes.h"
#include "EOPlayerController.generated.h"

class AEOAircraftPawn;
class AEOOperativeCharacter;
class UEOInputConfig;

/**
 * Owns the player's relationship with the two halves of the game: which pawn is
 * possessed, and which input mapping context is active.
 *
 * The deploy/extract entry points here are the seams M3 and M7 grow into. They
 * currently do the minimum: swap pawn, swap context, move the mission state along.
 */
UCLASS()
class EXECUTIVEOPS_API AEOPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEOPlayerController();

	const UEOInputConfig* GetInputConfig() const { return InputConfig; }

	UFUNCTION(BlueprintPure, Category = "Control")
	EEOControlMode GetControlMode() const { return ControlMode; }

	/** Possess the aircraft, swapping to the flight mapping context. */
	UFUNCTION(BlueprintCallable, Category = "Control")
	bool PossessAircraft();

	/** Possess the operative, swapping to the ground mapping context. */
	UFUNCTION(BlueprintCallable, Category = "Control")
	bool PossessOperative();

	/**
	 * Aircraft -> operative. M0: checks the aircraft reports ready, moves the
	 * operative to the deployment socket, possesses it. M3 adds the real sequence.
	 */
	UFUNCTION(BlueprintCallable, Category = "Control")
	bool RequestDeployment();

	/** Operative -> aircraft. M7 adds the aircraft arrival and boarding. */
	UFUNCTION(BlueprintCallable, Category = "Control")
	bool RequestExtraction();

	/** Reset everything to the boot state without reloading the level. */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void EOReset();

protected:
	/**
	 * Input config is built here rather than in BeginPlay: the game mode possesses
	 * the default pawn before BeginPlay runs, and that pawn's
	 * SetupPlayerInputComponent needs the config to already exist.
	 */
	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	/** Entry point for the -EOSelfTest command-line switch. */
	void RunSelfTest();

	/** Finds the first aircraft/operative in the level, or spawns one if absent. */
	AEOAircraftPawn* ResolveAircraft();
	AEOOperativeCharacter* ResolveOperative();

	void ApplyMappingContext(EEOControlMode Mode);

	/** Classes used when the level contains no placed pawn. */
	UPROPERTY(EditDefaultsOnly, Category = "Control")
	TSubclassOf<AEOAircraftPawn> AircraftClass;

	UPROPERTY(EditDefaultsOnly, Category = "Control")
	TSubclassOf<AEOOperativeCharacter> OperativeClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEOInputConfig> InputConfig;

	UPROPERTY(Transient)
	TObjectPtr<AEOAircraftPawn> Aircraft;

	UPROPERTY(Transient)
	TObjectPtr<AEOOperativeCharacter> Operative;

	/** Where each pawn started, so EOReset can put them back. */
	FTransform AircraftStartTransform;
	FTransform OperativeStartTransform;

	EEOControlMode ControlMode = EEOControlMode::None;
};
