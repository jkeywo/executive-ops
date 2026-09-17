#include "Feedback/EOCameraShakes.h"

UEOImpulseShakePattern::UEOImpulseShakePattern(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEOImpulseShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(Duration);
}

void UEOImpulseShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	Elapsed = 0.f;
}

void UEOImpulseShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params,
	FCameraShakePatternUpdateResult& OutResult)
{
	Elapsed += Params.DeltaTime;

	const float Alpha = FMath::Clamp(Elapsed / FMath::Max(Duration, KINDA_SMALL_NUMBER), 0.f, 1.f);

	// Squared falloff: the transient is nearly all in the first third, which is
	// what makes a kick read as an impact rather than a wobble.
	const float Envelope = FMath::Square(1.f - Alpha);

	// Directionality blends between a one-sided shove (a half sine that never
	// crosses zero) and a symmetric rattle.
	const float Phase = Elapsed * Frequency * 2.f * PI;
	const float Oscillation = FMath::Sin(Phase);
	const float Shove = FMath::Sin(FMath::Min(Elapsed / FMath::Max(Duration, KINDA_SMALL_NUMBER), 1.f) * PI);
	const float Wave = FMath::Lerp(Oscillation, Shove, Directionality);

	const float Amount = Wave * Envelope * Params.GetTotalScale();

	OutResult.Rotation = RotationAmplitude * Amount;
	OutResult.Location = LocationAmplitude * Amount;
	OutResult.FOV = FOVAmplitude * Amount;
}

bool UEOImpulseShakePattern::IsFinishedImpl() const
{
	return Elapsed >= Duration;
}

UEOCameraShake::UEOCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Impulse = ObjectInitializer.CreateDefaultSubobject<UEOImpulseShakePattern>(this, TEXT("Impulse"));
	SetRootShakePattern(Impulse);
}

UEOShake_PistolRecoil::UEOShake_PistolRecoil(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UEOImpulseShakePattern* P = GetImpulsePattern();
	P->Duration = 0.16f;
	P->RotationAmplitude = FRotator(-1.1f, 0.35f, 0.2f);	// nose up, slight drift right
	P->Frequency = 14.f;
	P->Directionality = 1.f;
}

UEOShake_TakedownImpact::UEOShake_TakedownImpact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UEOImpulseShakePattern* P = GetImpulsePattern();
	P->Duration = 0.22f;
	P->RotationAmplitude = FRotator(0.9f, -1.6f, 1.4f);
	P->LocationAmplitude = FVector(-4.f, 3.f, -2.f);
	P->Frequency = 11.f;
	P->Directionality = 0.85f;
}

UEOShake_HardLanding::UEOShake_HardLanding(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UEOImpulseShakePattern* P = GetImpulsePattern();
	P->Duration = 0.3f;
	P->RotationAmplitude = FRotator(1.8f, 0.f, 0.5f);	// view compresses downward
	P->LocationAmplitude = FVector(0.f, 0.f, -6.f);
	P->Frequency = 8.f;
	P->Directionality = 0.9f;
}

UEOShake_DeployLaunch::UEOShake_DeployLaunch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UEOImpulseShakePattern* P = GetImpulsePattern();
	P->Duration = 0.45f;
	P->RotationAmplitude = FRotator(-1.4f, 0.6f, 0.9f);
	P->LocationAmplitude = FVector(-8.f, 0.f, 0.f);
	P->FOVAmplitude = 6.f;								// the signature widening
	P->Frequency = 6.f;
	P->Directionality = 1.f;
}

UEOShake_AircraftCollision::UEOShake_AircraftCollision(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UEOImpulseShakePattern* P = GetImpulsePattern();
	P->Duration = 0.55f;
	P->RotationAmplitude = FRotator(2.2f, 1.8f, 3.4f);
	P->LocationAmplitude = FVector(-10.f, 8.f, 6.f);
	P->Frequency = 16.f;
	P->Directionality = 0.15f;							// genuine rattle, and only here
}

UEOShake_PlayerDamage::UEOShake_PlayerDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UEOImpulseShakePattern* P = GetImpulsePattern();
	P->Duration = 0.18f;
	P->RotationAmplitude = FRotator(0.7f, 0.9f, 0.8f);
	P->Frequency = 13.f;
	P->Directionality = 0.6f;
}

UEOShake_ExtractionBoard::UEOShake_ExtractionBoard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UEOImpulseShakePattern* P = GetImpulsePattern();
	P->Duration = 0.35f;
	P->RotationAmplitude = FRotator(1.3f, 0.5f, 0.7f);
	P->LocationAmplitude = FVector(0.f, 0.f, -5.f);
	P->Frequency = 9.f;
	P->Directionality = 0.95f;
}
