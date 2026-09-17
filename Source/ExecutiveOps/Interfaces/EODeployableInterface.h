#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EODeployableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UEODeployableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by the operative. The deployment sequence talks to the character through
 * this so M3 can change the launch/drop without touching the controller.
 */
class EXECUTIVEOPS_API IEODeployableInterface
{
	GENERATED_BODY()

public:
	/** Called immediately before possession, with the aircraft's deployment socket. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Deployment")
	void OnDeployFrom(AActor* SourceAircraft, const FTransform& DeploySocket);

	/** Called once the operative is on the ground and ground controls should be live. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Deployment")
	void OnDeployComplete();

	/** Called when the extraction pickup begins, before returning to aircraft control. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Deployment")
	void OnExtractBegin(AActor* TargetAircraft);
};
