#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EOTraversalComponent.generated.h"

class ACharacter;
class UAnimSequence;

/** What the obstacle ahead turned out to be. */
UENUM(BlueprintType)
enum class EEOTraversalType : uint8
{
	None		UMETA(DisplayName = "None"),

	/** Low and thin: go over it and land on the far side without stopping. */
	Vault		UMETA(DisplayName = "Vault"),

	/** Chest height with a surface on top: pull up and stand on it. */
	Mantle		UMETA(DisplayName = "Mantle"),

	/** Tall: scramble up the face and end on top. */
	Climb		UMETA(DisplayName = "Climb")
};

/** A traversal the scan found and the motion that would execute it. */
USTRUCT(BlueprintType)
struct FEOTraversalQuery
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	EEOTraversalType Type = EEOTraversalType::None;

	/** Where the character ends up. */
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector EndLocation = FVector::ZeroVector;

	/** Mid-point the motion arcs through, so a vault goes over rather than into. */
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector ApexLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	float ObstacleHeight = 0.f;

	bool IsValid() const { return Type != EEOTraversalType::None; }
};

/**
 * Contextual parkour: one input, and the component decides what the obstacle in
 * front of the operative actually is.
 *
 * Deliberately not a general traversal framework. It shape-traces forward, finds
 * the top of whatever it hit, classifies it by height, and drives the character
 * along a fixed arc for a fixed duration. That is enough to answer M4's question
 * - can the player decide where to go and get there without wrestling the system
 * - and it stays cheap to retune.
 *
 * Detection is deliberately generous. Failing to vault a waist-high box because
 * the player was 10cm off is exactly the feel this is meant to avoid.
 */
UCLASS(ClassGroup = (ExecutiveOps), meta = (BlueprintSpawnableComponent))
class EXECUTIVEOPS_API UEOTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEOTraversalComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Look for something to traverse ahead. Does not change any state. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	FEOTraversalQuery Scan() const;

	/** Scan and, if something is found, start traversing it. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	bool TryTraverse();

	UFUNCTION(BlueprintPure, Category = "Traversal")
	bool IsTraversing() const { return bTraversing; }

	UFUNCTION(BlueprintPure, Category = "Traversal")
	EEOTraversalType GetActiveType() const { return ActiveQuery.Type; }

	/**
	 * Remember a traversal press for a short while. Pressing just before reaching
	 * an obstacle should still vault it rather than being swallowed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void BufferInput();

	/** Abandon any traversal in progress and restore normal movement. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void Cancel();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Deactivate() override;

	/** How far ahead an obstacle is noticed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float ScanDistance = 170.f;

	/** Radius of the forward probe. Wider is more forgiving of imperfect aim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float ScanRadius = 34.f;

	/** Obstacles at or below this are vaulted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float VaultMaxHeight = 130.f;

	/** Above vault height and up to here, the operative pulls up onto the top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float MantleMaxHeight = 230.f;

	/** The tallest thing that can be scrambled up at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float ClimbMaxHeight = 420.f;

	/** Ignore lips and kerbs; the character movement step-up handles those. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float MinObstacleHeight = 45.f;

	/** How far past the obstacle a vault looks for somewhere to land. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float VaultLandingProbe = 190.f;

	/** The furthest a vault may drop on the far side before it is refused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float MaxVaultDrop = 220.f;

	/** Seconds a traversal press stays live while the player closes on an obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	float InputBufferTime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Timing")
	float VaultDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Timing")
	float MantleDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Timing")
	float ClimbDuration = 1.05f;

	/** Animations, assigned in the Blueprint from the owned packs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Animation")
	TObjectPtr<UAnimSequence> VaultAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Animation")
	TObjectPtr<UAnimSequence> MantleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Animation")
	TObjectPtr<UAnimSequence> ClimbAnim;

private:
	void Begin(const FEOTraversalQuery& Query);
	void Finish();

	/**
	 * Traces down onto a surface. Rejects steep faces, penetrating starts, and -
	 * when RequiredActor is set - anything belonging to a different actor, which
	 * is what stops a ceiling overhead being mistaken for the top of a crate.
	 */
	bool FindGround(const FVector& From, float Depth, const AActor* RequiredActor,
		FVector& OutGround) const;

	/** True if the standing capsule fits at this location. */
	bool IsCapsuleClear(const FVector& AtLocation) const;

	/** Sweeps the arc itself, so a traversal cannot pass through a wall. */
	bool IsPathClear(const FVector& Start, const FVector& Apex, const FVector& End) const;

	/** Control point placing the curve's midpoint exactly on the desired apex. */
	static FVector ControlPointFor(const FVector& Start, const FVector& Apex, const FVector& End);

	static FVector EvaluateArc(const FVector& Start, const FVector& Control,
		const FVector& End, float T);

	/** False while stowed, mid-drop or unpossessed. */
	bool IsTraversalAllowed() const;

	ACharacter* GetCharacter() const;
	float DurationFor(EEOTraversalType Type) const;
	UAnimSequence* AnimationFor(EEOTraversalType Type) const;

	UPROPERTY(Transient)
	FEOTraversalQuery ActiveQuery;

	FVector StartLocation = FVector::ZeroVector;
	FVector ControlPoint = FVector::ZeroVector;

	bool bTraversing = false;
	float Elapsed = 0.f;
	float Duration = 0.f;
	float BufferRemaining = 0.f;
};
