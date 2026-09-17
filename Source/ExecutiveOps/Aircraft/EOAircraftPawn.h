#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "EOAircraftPawn.generated.h"

class UBoxComponent;
class UCameraComponent;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UEOInputConfig;
struct FInputActionValue;

/**
 * Placeholder VTOL. M0 gives it just enough motion to prove possession, the input
 * context swap and the deployment socket. Handling is explicitly M1's job — do not
 * tune this.
 */
UCLASS()
class EXECUTIVEOPS_API AEOAircraftPawn : public APawn, public IEOAircraftControlInterface
{
	GENERATED_BODY()

public:
	AEOAircraftPawn();

	virtual void Tick(float DeltaSeconds) override;

	//~ IEOAircraftControlInterface
	virtual void SetFlightInput_Implementation(const FVector& MoveInput) override;
	virtual void SetYawInput_Implementation(float YawInput) override;
	virtual void SetHoverEnabled_Implementation(bool bEnabled) override;
	virtual bool IsHovering_Implementation() const override;
	virtual float GetCurrentSpeed_Implementation() const override;
	virtual FTransform GetDeploymentSocketTransform_Implementation() const override;
	virtual bool IsReadyForDeployment_Implementation() const override;
	virtual void ResetFlightState_Implementation() override;
	//~ End IEOAircraftControlInterface

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Input_Move(const FInputActionValue& Value);
	void Input_Vertical(const FInputActionValue& Value);
	void Input_Yaw(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_HoverStart(const FInputActionValue& Value);
	void Input_HoverStop(const FInputActionValue& Value);
	void Input_Deploy(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<UStaticMeshComponent> HullMesh;

	/** The single fixed deployment socket referenced by M3. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft|Deployment")
	TObjectPtr<USceneComponent> DeploymentSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> ChaseCamera;

	/** Placeholder handling constants. M1 owns real flight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Placeholder Handling")
	float MaxSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Placeholder Handling")
	float HoverMaxSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Placeholder Handling")
	float Acceleration = 4000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Placeholder Handling")
	float BrakingDeceleration = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Placeholder Handling")
	float YawRate = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float LookSensitivity = 1.f;

private:
	const UEOInputConfig* GetInputConfig() const;

	FVector PendingMoveInput = FVector::ZeroVector;
	float PendingYawInput = 0.f;
	FVector Velocity = FVector::ZeroVector;
	bool bHoverEnabled = false;
};
