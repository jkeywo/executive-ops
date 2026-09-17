#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EOExtractionInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UEOExtractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * The extraction point. M0 defines the contract only; M7 implements the scripted
 * aircraft arrival behind it.
 */
class EXECUTIVEOPS_API IEOExtractionInterface
{
	GENERATED_BODY()

public:
	/** False until the objective is complete. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Extraction")
	bool IsExtractionAvailable() const;

	/** Player pressed extract inside the zone. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Extraction")
	void RequestExtraction(AActor* Requester);

	/** Where the aircraft should arrive / the player should board. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Extraction")
	FTransform GetExtractionTransform() const;
};
