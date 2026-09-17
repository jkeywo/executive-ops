#include "Combat/EOWeaponComponent.h"

#include "Combat/EOHealthComponent.h"
#include "Feedback/EOFeedbackEvents.h"
#include "Feedback/EOFeedbackSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UEOWeaponComponent::UEOWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	FireEvent = EOFeedbackEvents::Pistol_Fire;
}

void UEOWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Cooldown = FMath::Max(Cooldown - DeltaTime, 0.f);
}

FEOShotResult UEOWeaponComponent::TryFire(const FVector& Muzzle, const FVector& AimPoint,
	bool bAccurate)
{
	FEOShotResult Result;

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || Cooldown > 0.f)
	{
		return Result;
	}

	Cooldown = Interval;
	Result.bFired = true;

	const FVector ToAim = (AimPoint - Muzzle).GetSafeNormal();
	Result.Direction = FMath::VRandCone(ToAim,
		FMath::DegreesToRadians(bAccurate ? AimSpread : HipSpread));

	if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		FEOFeedbackContext Shot = FEOFeedbackContext::At(Muzzle);
		Shot.Rotation = Result.Direction.Rotation();
		Shot.AttachTo = MuzzleAttachment;
		Feedback->Play(FireEvent, Shot);
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOWeaponShot), false, Owner);
	// The impact preset is chosen by surface, so the trace has to bring one back.
	Params.bReturnPhysicalMaterial = true;

	// ECC_Pawn, not ECC_Visibility: the default Pawn collision profile ignores the
	// Visibility channel, so a Visibility trace goes straight through the
	// character it is aimed at and buries itself in the scenery behind them.
	Result.bBlockingHit = World->LineTraceSingleByChannel(Result.Hit, Muzzle,
		Muzzle + Result.Direction * Range, ECC_Pawn, Params);

	if (!Result.bBlockingHit)
	{
		return Result;
	}

	AActor* HitActor = Result.Hit.GetActor();

	// Asked, rather than assumed, because callers need to know - the guard's near
	// miss and the operative's impact preset both turn on it. Damage itself does
	// not depend on the answer.
	Result.bHitDamageable = HitActor
		&& HitActor->FindComponentByClass<UEOHealthComponent>() != nullptr;

	APawn* OwnerPawn = Cast<APawn>(Owner);
	UGameplayStatics::ApplyPointDamage(HitActor, Damage, Result.Direction, Result.Hit,
		OwnerPawn ? OwnerPawn->GetController() : Owner->GetInstigatorController(),
		Owner, nullptr);

	return Result;
}
