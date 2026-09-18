#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Interfaces/EOMissionTypes.h"
#include "Mission/EOMissionSubsystem.h"

/**
 * The first checks in this project that do not require playing the game.
 *
 * Everything else lived in UEOSelfTest, which was one 39-phase timed sequence: to
 * assert anything about mission transitions you have to fly the approach first,
 * nothing can be run on its own, and an early failure takes the rest with it.
 * The mission state machine needs none of that - it is a validated transition
 * table - so it is tested through its own interface here.
 *
 * Run from the editor's Session Frontend, or:
 *   UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=Automation
 *       -ExecCmds="Automation RunTests ExecutiveOps.Mission" -unattended -nullrhi
 *
 * See Docs/adr/0007.
 */
namespace EOMissionTestHelpers
{
	/**
	 * A world that exists only for the subsystem under test.
	 *
	 * UEOMissionSubsystem is a UWorldSubsystem, so it needs one - but it needs
	 * nothing else from it. No map, no actors, no pawns, no level.
	 */
	struct FScopedTestWorld
	{
		FScopedTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/false);
			Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
			Context->SetCurrentWorld(World);

			FURL URL;
			World->InitializeActorsForPlay(URL);
			World->BeginPlay();
		}

		~FScopedTestWorld()
		{
			World->BeginTearingDown();
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}

		UEOMissionSubsystem* Mission() const { return World->GetSubsystem<UEOMissionSubsystem>(); }

		UWorld* World = nullptr;
		FWorldContext* Context = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOMissionLegalRouteTest,
	"ExecutiveOps.Mission.A full run walks the loop end to end",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOMissionLegalRouteTest::RunTest(const FString& Parameters)
{
	EOMissionTestHelpers::FScopedTestWorld Scope;
	UEOMissionSubsystem* Mission = Scope.Mission();

	if (!TestNotNull(TEXT("the world has a mission subsystem"), Mission))
	{
		return false;
	}

	TestEqual(TEXT("a fresh mission is inactive"),
		Mission->GetMissionState(), EEOMissionState::Inactive);

	TestTrue(TEXT("a mission can start"), Mission->StartMission());
	TestEqual(TEXT("starting puts it in flight"),
		Mission->GetMissionState(), EEOMissionState::InFlight);

	TestTrue(TEXT("deployment can begin from flight"), Mission->BeginDeployment());
	TestTrue(TEXT("deployment can complete"), Mission->CompleteDeployment());
	TestEqual(TEXT("completing a deployment puts the player on the ground"),
		Mission->GetMissionState(), EEOMissionState::OnGround);

	TestTrue(TEXT("the objective can be completed"), Mission->CompleteObjective());
	TestTrue(TEXT("extraction can begin once the objective is done"),
		Mission->BeginExtraction());
	TestTrue(TEXT("the mission can close out"), Mission->CompleteMission());
	TestEqual(TEXT("the loop ends complete"),
		Mission->GetMissionState(), EEOMissionState::Complete);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOMissionIllegalTransitionTest,
	"ExecutiveOps.Mission.An illegal transition is refused and changes nothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOMissionIllegalTransitionTest::RunTest(const FString& Parameters)
{
	EOMissionTestHelpers::FScopedTestWorld Scope;
	UEOMissionSubsystem* Mission = Scope.Mission();

	if (!TestNotNull(TEXT("the world has a mission subsystem"), Mission))
	{
		return false;
	}

	// The subsystem's contract is that an illegal request is logged and dropped
	// rather than silently corrupting state. Both halves matter: it has to say
	// no, and the state has to be untouched afterwards.
	AddExpectedError(TEXT("rejected: expected state"),
		EAutomationExpectedErrorFlags::Contains, 0);

	TestFalse(TEXT("a mission cannot complete before it starts"), Mission->CompleteMission());
	TestEqual(TEXT("and the refusal left the state alone"),
		Mission->GetMissionState(), EEOMissionState::Inactive);

	TestFalse(TEXT("extraction cannot begin before there is a mission"),
		Mission->BeginExtraction());
	TestEqual(TEXT("and that refusal left the state alone too"),
		Mission->GetMissionState(), EEOMissionState::Inactive);

	TestFalse(TEXT("a deployment cannot complete out of nowhere"),
		Mission->CompleteDeployment());
	TestEqual(TEXT("still inactive"),
		Mission->GetMissionState(), EEOMissionState::Inactive);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOMissionResetTest,
	"ExecutiveOps.Mission.Reset drops back to inactive from anywhere",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOMissionResetTest::RunTest(const FString& Parameters)
{
	EOMissionTestHelpers::FScopedTestWorld Scope;
	UEOMissionSubsystem* Mission = Scope.Mission();

	if (!TestNotNull(TEXT("the world has a mission subsystem"), Mission))
	{
		return false;
	}

	Mission->StartMission();
	Mission->BeginDeployment();
	Mission->CompleteDeployment();

	Mission->ResetMission();
	TestEqual(TEXT("reset returns to inactive"),
		Mission->GetMissionState(), EEOMissionState::Inactive);

	// The point of reset is that the next run is unaffected by the last one,
	// which is what the ground suite proves by running the mission twice.
	TestTrue(TEXT("and a mission can start again afterwards"), Mission->StartMission());

	return true;
}

#endif // WITH_AUTOMATION_TESTS
