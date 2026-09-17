#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/EODeployableInterface.h"
#include "EOOperativeCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UEOInputConfig;
struct FInputActionValue;

/**
 * The operative. M0 is a plain third-person character: move, look, jump, sprint.
 *
 * No parkour, no takedown, no weapon, no perception mode — M4, M5 and M9 add those.
 * The mesh and animation blueprint are assigned in a Blueprint subclass once the
 * production rig and animation packs are imported.
 */
UCLASS()
class EXECUTIVEOPS_API AEOOperativeCharacter : public ACharacter, public IEODeployableInterface
{
	GENERATED_BODY()

public:
	AEOOperativeCharacter();

	//~ IEODeployableInterface
	virtual void OnDeployFrom_Implementation(AActor* SourceAircraft, const FTransform& DeploySocket) override;
	virtual void OnDeployComplete_Implementation() override;
	virtual void OnExtractBegin_Implementation(AActor* TargetAircraft) override;
	//~ End IEODeployableInterface

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_SprintStart(const FInputActionValue& Value);
	void Input_SprintStop(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Placeholder values. Tuning belongs in M8, not here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 850.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float LookSensitivity = 1.f;

private:
	const UEOInputConfig* GetInputConfig() const;
};
