#include "EOFunctionalTest.h"

#include "ExecutiveOpsDev.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "Core/EOPlayerController.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Mission/EOExtractionZone.h"
#include "Mission/EOMissionSubsystem.h"
#include "Mission/EOObjectiveTerminal.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AEOFunctionalTest::AEOFunctionalTest()
{
	PrimaryActorTick.bCanEverTick = true;

	// A sequence that stalls should fail rather than hang the run. Subclasses
	// with a long approach raise this; the default is generous for a phase
	// sequence and short for a hung one.
	TimeLimit = 120.f;
	TimesUpResult = EFunctionalTestResult::Failed;
}

bool AEOFunctionalTest::IsReady_Implementation()
{
	// Most sequences drive the player's pawn, so a controller without one is
	// not ready yet. The framework keeps asking until this says yes or the
	// preparation limit runs out.
	const AEOPlayerController* Controller = GetController();
	if (!Controller || Controller->GetPawn() == nullptr)
	{
		return false;
	}

	// The navmesh is generated at runtime. A sequence sampled while it is still
	// building sees a guard that cannot path, which is a fact about the first
	// second of the session rather than about the guard.
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	return Nav && !Nav->IsNavigationBuildInProgress();
}

void AEOFunctionalTest::StartTest()
{
	Super::StartTest();

	Phase = 0;
	PhaseDwell = 0.f;
	PhaseElapsed = 0.f;
	SinceStep = 0.f;
	bFinished = false;
	Checks = 0;
	Failures = 0;
}

void AEOFunctionalTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || bFinished)
	{
		return;
	}

	// Fixed cadence, so a dwell means the same thing at any frame rate.
	SinceStep += DeltaSeconds;
	while (SinceStep >= StepInterval && !bFinished)
	{
		SinceStep -= StepInterval;

		PhaseElapsed += StepInterval;
		if (PhaseElapsed < PhaseDwell)
		{
			continue;
		}

		Step();
	}
}

void AEOFunctionalTest::Advance(int32 NextPhase, float DwellSeconds)
{
	Phase = NextPhase;
	PhaseDwell = DwellSeconds;
	PhaseElapsed = 0.f;
}

void AEOFunctionalTest::Done()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	const bool bPassed = Failures == 0;
	const FString Summary = FString::Printf(TEXT("%d/%d checks passed"), Checks - Failures, Checks);

	// Same line the self-test wrote, so anything grepping for a verdict still
	// finds one - and so the two report identically while both exist.
	UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] === %s (%s) ==="),
		bPassed ? TEXT("PASSED") : TEXT("FAILED"), *Summary);

	FinishTest(bPassed ? EFunctionalTestResult::Succeeded : EFunctionalTestResult::Failed, Summary);
}

// The parameter is What rather than Description because AFunctionalTest has a
// Description member of its own, and shadowing it is an error in this build.
bool AEOFunctionalTest::Check(bool bCondition, const FString& What)
{
	++Checks;
	if (bCondition)
	{
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest]   PASS  %s"), *What);
	}
	else
	{
		++Failures;
		UE_LOG(LogExecutiveOpsDev, Error, TEXT("[SelfTest]   FAIL  %s"), *What);
	}

	// Forwarded so the framework's own report carries every assertion, not just
	// the summary - a failure is then visible in the Session Frontend by name.
	return AssertTrue(bCondition, What);
}

// ---- Finders -------------------------------------------------------------------

AEOPlayerController* AEOFunctionalTest::GetController() const
{
	return Cast<AEOPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

AEOOperativeCharacter* AEOFunctionalTest::GetOperative() const
{
	const AEOPlayerController* Controller = GetController();
	return Controller ? Cast<AEOOperativeCharacter>(Controller->GetPawn()) : nullptr;
}

AEOGuardCharacter* AEOFunctionalTest::GetGuard() const
{
	for (TActorIterator<AEOGuardCharacter> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AEOObjectiveTerminal* AEOFunctionalTest::GetObjective() const
{
	for (TActorIterator<AEOObjectiveTerminal> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AEOExtractionZone* AEOFunctionalTest::GetExtractionZone() const
{
	for (TActorIterator<AEOExtractionZone> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

APawn* AEOFunctionalTest::GetAircraftPawn() const
{
	const AEOPlayerController* Controller = GetController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return (Pawn && Pawn->Implements<UEOAircraftControlInterface>()) ? Pawn : nullptr;
}

APawn* AEOFunctionalTest::FindAircraftInLevel() const
{
	for (TActorIterator<AEOAircraftPawn> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

// ---- Shared setup ---------------------------------------------------------------

void AEOFunctionalTest::ResetEncounter() const
{
	if (AEOGuardCharacter* Guard = GetGuard())
	{
		Guard->ResetGuard();
	}
	if (AEOOperativeCharacter* Op = GetOperative())
	{
		if (UEOHealthComponent* Health = Op->GetHealth())
		{
			Health->Revive();
		}
	}
}

bool AEOFunctionalTest::EnterGroundMission() const
{
	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	if (!Mission)
	{
		return false;
	}

	// Stands in for the flight and the drop, which the flight sequence covers.
	Mission->ResetMission();
	return Mission->StartMission()
		&& Mission->BeginDeployment()
		&& Mission->CompleteDeployment();
}
