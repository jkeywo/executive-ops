#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EOInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UEOInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Something the operative can walk up to and press interact on.
 *
 * Two implementations in the prototype - the objective terminal and the
 * extraction point - which is exactly why it is an interface rather than two
 * bespoke proximity checks that drift apart.
 */
class EXECUTIVEOPS_API IEOInteractableInterface
{
	GENERATED_BODY()

public:
	/** False when out of range, or when the mission state makes it meaningless. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;

	/** Returns true if the interaction actually did something. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool Interact(AActor* Interactor);

	/** What the HUD offers the player, e.g. "DOWNLOAD [E]". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;

	/** How close the operative has to be. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	float GetInteractionRange() const;
};
