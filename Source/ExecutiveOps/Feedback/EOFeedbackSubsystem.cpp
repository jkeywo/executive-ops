#include "Feedback/EOFeedbackSubsystem.h"

#include "ExecutiveOps.h"
#include "Feedback/EOFeedbackEvents.h"
#include "Feedback/EOFeedbackPresetSet.h"
#include "Feedback/EOFeedbackSettings.h"

#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

FEOFeedbackContext FEOFeedbackContext::AtActor(AActor* InActor)
{
	FEOFeedbackContext Context;
	if (InActor)
	{
		Context.Location = InActor->GetActorLocation();
		Context.Rotation = InActor->GetActorRotation();
		Context.Target = InActor;
	}
	return Context;
}

UEOFeedbackSubsystem* UEOFeedbackSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)
		: nullptr;

	return World ? World->GetSubsystem<UEOFeedbackSubsystem>() : nullptr;
}

void UEOFeedbackSubsystem::Deinitialize()
{
	ClearScreenState();
	LoadedPresets = nullptr;
	bPresetLoadAttempted = false;

	Super::Deinitialize();
}

TStatId UEOFeedbackSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UEOFeedbackSubsystem, STATGROUP_Tickables);
}

const FEOFeedbackPreset* UEOFeedbackSubsystem::FindPreset(FName Event)
{
	// Loaded once, on the first event of the session. Synchronous is fine: the
	// set is a few kilobytes of structs, and the referenced assets are soft.
	if (!bPresetLoadAttempted)
	{
		bPresetLoadAttempted = true;

		const TSoftObjectPtr<UEOFeedbackPresetSet>& SetRef = UEOFeedbackSettings::Get().PresetSet;
		if (!SetRef.IsNull())
		{
			LoadedPresets = SetRef.LoadSynchronous();
		}

		if (!LoadedPresets)
		{
			// Expected on a clone without the asset packs. Say so once, then stay quiet.
			UE_LOG(LogExecutiveOps, Log,
				TEXT("No feedback preset set configured; game feel will be silent. "
					 "Run Scripts/import_fab_assets.ps1 then Scripts/m8_build_feedback_presets.py."));
		}
	}

	return LoadedPresets ? LoadedPresets->Find(Event) : nullptr;
}

APlayerController* UEOFeedbackSubsystem::GetLocalController() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

void UEOFeedbackSubsystem::PlayAtLocation(FName Event, const FVector& Location, float Scale)
{
	FEOFeedbackContext Context = FEOFeedbackContext::At(Location);
	Context.Scale = Scale;
	Play(Event, Context);
}

void UEOFeedbackSubsystem::PlayAtActor(FName Event, AActor* Actor, float Scale)
{
	FEOFeedbackContext Context = FEOFeedbackContext::AtActor(Actor);
	Context.Scale = Scale;
	Play(Event, Context);
}

void UEOFeedbackSubsystem::Play(FName Event, const FEOFeedbackContext& Context)
{
	const FEOFeedbackPreset* Preset = FindPreset(Event);
	if (!Preset)
	{
		return;
	}

	const float Scale = FMath::Max(Context.Scale, 0.f);
	if (Scale <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	ApplyAudio(*Preset, Context, Scale);
	ApplyEffect(*Preset, Context, Scale);
	ApplyCamera(*Preset, Context, Scale);
	ApplyScreen(*Preset, Context, Scale);
	ApplyHaptics(*Preset);
	ApplyDecal(*Preset, Context);
	ApplyHitStop(*Preset, Context);
}

void UEOFeedbackSubsystem::ApplyAudio(const FEOFeedbackPreset& Preset,
	const FEOFeedbackContext& Context, float Scale)
{
	USoundBase* Sound = Preset.Sound.LoadSynchronous();
	if (!Sound)
	{
		return;
	}

	const float Volume = Preset.VolumeMultiplier * Scale * UEOFeedbackSettings::Get().FeedbackVolume;
	const float Pitch = Preset.PitchJitter > 0.f
		? FMath::FRandRange(1.f - Preset.PitchJitter, 1.f + Preset.PitchJitter)
		: 1.f;

	if (Context.AttachTo)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, Context.AttachTo, NAME_None,
			FVector::ZeroVector, EAttachLocation::SnapToTarget, /*bStopWhenAttachedToDestroyed*/ true,
			Volume, Pitch);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Context.Location, Context.Rotation,
			Volume, Pitch);
	}
}

void UEOFeedbackSubsystem::ApplyEffect(const FEOFeedbackPreset& Preset,
	const FEOFeedbackContext& Context, float Scale)
{
	UNiagaraSystem* System = Preset.Effect.LoadSynchronous();
	if (!System)
	{
		return;
	}

	const FVector EffectScale = FVector(Preset.EffectScale * Scale);

	if (Preset.bAttachEffect && Context.AttachTo)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(System, Context.AttachTo, NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
			/*bAutoDestroy*/ true);
	}
	else
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, Context.Location,
			Context.Rotation, EffectScale, /*bAutoDestroy*/ true);
	}
}

void UEOFeedbackSubsystem::ApplyCamera(const FEOFeedbackPreset& Preset,
	const FEOFeedbackContext& Context, float Scale)
{
	if (!Preset.CameraShake)
	{
		return;
	}

	const float ShakeScale = Preset.CameraShakeScale * Scale
		* UEOFeedbackSettings::Get().GetCameraShakeScale();

	if (ShakeScale <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (APlayerController* PC = GetLocalController())
	{
		PC->ClientStartCameraShake(Preset.CameraShake, ShakeScale);
	}
}

void UEOFeedbackSubsystem::ApplyScreen(const FEOFeedbackPreset& Preset,
	const FEOFeedbackContext& Context, float Scale)
{
	const float ScreenScale = UEOFeedbackSettings::Get().GetScreenEffectScale();
	if (ScreenScale <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (!FMath::IsNearlyZero(Preset.FOVImpulse))
	{
		// Impulses replace rather than accumulate: two overlapping events should
		// not stack into a fisheye.
		const float Candidate = Preset.FOVImpulse * Scale * ScreenScale;
		if (FMath::Abs(Candidate) > FMath::Abs(FOVImpulse))
		{
			FOVImpulse = Candidate;
			FOVImpulseDecay = Preset.FOVImpulseDecay;
		}
	}

	if (Preset.VignetteIntensity > 0.f)
	{
		VignetteAlpha = FMath::Max(VignetteAlpha, Preset.VignetteIntensity * Scale * ScreenScale);
		VignetteDecay = Preset.VignetteDecay;
	}

	if (Preset.bDirectionalIndicator && Context.bHasSourceLocation)
	{
		const FVector ToSource = Context.SourceLocation - Context.Location;
		if (!ToSource.IsNearlyZero())
		{
			DamageDirection = ToSource.GetSafeNormal();
			DamageIndicatorAlpha = ScreenScale;
		}
	}
}

void UEOFeedbackSubsystem::ApplyHaptics(const FEOFeedbackPreset& Preset)
{
	if (!UEOFeedbackSettings::Get().bHapticsEnabled)
	{
		return;
	}

	UForceFeedbackEffect* Effect = Preset.ForceFeedback.LoadSynchronous();
	if (!Effect)
	{
		return;
	}

	if (APlayerController* PC = GetLocalController())
	{
		PC->ClientPlayForceFeedback(Effect);
	}
}

void UEOFeedbackSubsystem::ApplyDecal(const FEOFeedbackPreset& Preset,
	const FEOFeedbackContext& Context)
{
	UMaterialInterface* Material = Preset.Decal.LoadSynchronous();
	if (!Material)
	{
		return;
	}

	// Lifetime-limited by design. The brief rules out persistent world damage:
	// marks are there to confirm a shot landed, not to accumulate into a record.
	UGameplayStatics::SpawnDecalAtLocation(this, Material, Preset.DecalSize,
		Context.Location, Context.Rotation, Preset.DecalLifetime);
}

void UEOFeedbackSubsystem::ApplyHitStop(const FEOFeedbackPreset& Preset,
	const FEOFeedbackContext& Context)
{
	if (Preset.HitStopSeconds <= 0.f || !Context.Target)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* Target = Context.Target;
	Target->CustomTimeDilation = Preset.HitStopDilation;

	// Real seconds, not dilated ones, or a heavy hit-stop would extend itself.
	FTimerHandle Handle;
	TWeakObjectPtr<AActor> WeakTarget(Target);
	World->GetTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateUObject(this, &UEOFeedbackSubsystem::EndHitStop, WeakTarget),
		Preset.HitStopSeconds, false);
}

void UEOFeedbackSubsystem::EndHitStop(TWeakObjectPtr<AActor> Actor)
{
	if (AActor* Resolved = Actor.Get())
	{
		Resolved->CustomTimeDilation = 1.f;
	}
}

bool UEOFeedbackSubsystem::GetDamageDirection(FVector& OutWorldDirection, float& OutAlpha) const
{
	if (DamageIndicatorAlpha <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutWorldDirection = DamageDirection;
	OutAlpha = DamageIndicatorAlpha;
	return true;
}

void UEOFeedbackSubsystem::ClearScreenState()
{
	FOVImpulse = 0.f;
	VignetteAlpha = 0.f;
	DamageIndicatorAlpha = 0.f;
	DamageDirection = FVector::ZeroVector;
}

void UEOFeedbackSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime <= 0.f)
	{
		return;
	}

	// Exponential decay on all three, so nothing ends with a visible step.
	if (!FMath::IsNearlyZero(FOVImpulse))
	{
		FOVImpulse = FMath::FInterpTo(FOVImpulse, 0.f, DeltaTime,
			1.f / FMath::Max(FOVImpulseDecay, KINDA_SMALL_NUMBER));

		if (FMath::Abs(FOVImpulse) < 0.01f)
		{
			FOVImpulse = 0.f;
		}
	}

	if (VignetteAlpha > 0.f)
	{
		VignetteAlpha = FMath::FInterpTo(VignetteAlpha, 0.f, DeltaTime,
			1.f / FMath::Max(VignetteDecay, KINDA_SMALL_NUMBER));

		if (VignetteAlpha < 0.01f)
		{
			VignetteAlpha = 0.f;
		}
	}

	if (DamageIndicatorAlpha > 0.f)
	{
		DamageIndicatorAlpha = FMath::Max(0.f, DamageIndicatorAlpha - DeltaTime / DamageIndicatorDecay);
	}
}
