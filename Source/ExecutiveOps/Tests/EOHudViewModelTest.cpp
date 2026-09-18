#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Combat/EOGuardCharacter.h"
#include "Interfaces/EOMissionTypes.h"
#include "UI/EOHudViewModel.h"

/**
 * The interface's reductions, without a viewport, a world or an actor.
 *
 * These were unreachable while they lived inside AEOPlayerHUD's draw functions:
 * the only way to see what the standing slot said about a guard was to run the
 * game, put a guard in a state, and look. The reductions now take plain inputs,
 * so what the player reads for a given situation is a thing that can be
 * asserted directly - and a change to it is a change a test notices.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOHudDirectiveTest,
	"ExecutiveOps.HUD.The directive names the mission state",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOHudDirectiveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("no mission control at all"),
		FEOHudViewModel::DirectiveFor(false, EEOMissionState::Inactive, TEXT("")).ToString(),
		FString(TEXT("NO MISSION CONTROL")));

	TestEqual(TEXT("nothing selected"),
		FEOHudViewModel::DirectiveFor(true, EEOMissionState::Inactive, TEXT("")).ToString(),
		FString(TEXT("NO MISSION SELECTED")));

	// The site name is folded into the approach directive, upper-cased, so the
	// slot reads as one line rather than a label and a value.
	TestEqual(TEXT("the approach names the site"),
		FEOHudViewModel::DirectiveFor(true, EEOMissionState::InFlight, TEXT("Harbour Relay")).ToString(),
		FString(TEXT("APPROACH HARBOUR RELAY")));

	TestEqual(TEXT("on the ground the objective is the directive"),
		FEOHudViewModel::DirectiveFor(true, EEOMissionState::OnGround, TEXT("x")).ToString(),
		FString(TEXT("SECURE THE OBJECTIVE")));

	TestEqual(TEXT("a dead operative reads as such"),
		FEOHudViewModel::DirectiveFor(true, EEOMissionState::Failed, TEXT("x")).ToString(),
		FString(TEXT("OPERATIVE DOWN")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOHudStandingTest,
	"ExecutiveOps.HUD.Standing reports the worst guard, and moves on first notice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOHudStandingTest::RunTest(const FString& Parameters)
{
	// The tier ladder.
	TestEqual(TEXT("an oblivious patrol is tier 0"),
		FEOHudViewModel::StandingTierFor(EEOGuardState::Patrolling, 0.f), 0);
	TestEqual(TEXT("a patrol that has started to notice is already suspicious"),
		FEOHudViewModel::StandingTierFor(EEOGuardState::Patrolling, 0.05f), 1);
	TestEqual(TEXT("suspicious is tier 1"),
		FEOHudViewModel::StandingTierFor(EEOGuardState::Suspicious, 0.5f), 1);
	TestEqual(TEXT("searching is tier 2"),
		FEOHudViewModel::StandingTierFor(EEOGuardState::Searching, 0.f), 2);
	TestEqual(TEXT("alerted is tier 3"),
		FEOHudViewModel::StandingTierFor(EEOGuardState::Alerted, 1.f), 3);

	// What the tiers say, and how urgently.
	TestEqual(TEXT("no guards at all is no contact"),
		FEOHudViewModel::StandingTextFor(0, false).ToString(), FString(TEXT("NO CONTACT")));
	TestEqual(TEXT("and reads dim"),
		FEOHudViewModel::StandingToneFor(0, false), EEOHudTone::Dim);
	TestEqual(TEXT("an unaware guard is unaware"),
		FEOHudViewModel::StandingTextFor(0, true).ToString(), FString(TEXT("UNAWARE")));
	TestEqual(TEXT("an alerted guard reads as alarm"),
		FEOHudViewModel::StandingToneFor(3, true), EEOHudTone::Alarm);
	TestEqual(TEXT("searching reads as caution"),
		FEOHudViewModel::StandingToneFor(2, true), EEOHudTone::Caution);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOHudStanceTest,
	"ExecutiveOps.HUD.Stance is decided by what overrides what",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOHudStanceTest::RunTest(const FString& Parameters)
{
	// Down beats everything, whatever else is true.
	TestEqual(TEXT("dead is down, even mid-sprint"),
		FEOHudViewModel::StanceFor(true, false, false, TEXT(""), false, true, false, false).ToString(),
		FString(TEXT("DOWN")));

	// A verb in progress beats a posture.
	TestEqual(TEXT("a takedown in progress is a takedown"),
		FEOHudViewModel::StanceFor(false, true, false, TEXT(""), false, false, true, false).ToString(),
		FString(TEXT("TAKEDOWN")));
	TestEqual(TEXT("a traversal names itself"),
		FEOHudViewModel::StanceFor(false, false, false, TEXT("Vault"), false, true, false, false).ToString(),
		FString(TEXT("VAULT")));

	// Postures, in order.
	TestEqual(TEXT("sliding beats sprinting"),
		FEOHudViewModel::StanceFor(false, false, false, TEXT(""), true, true, false, false).ToString(),
		FString(TEXT("SLIDING")));
	TestEqual(TEXT("crouched beats aiming"),
		FEOHudViewModel::StanceFor(false, false, false, TEXT(""), false, false, true, true).ToString(),
		FString(TEXT("CROUCHED")));
	TestEqual(TEXT("nothing at all is standing"),
		FEOHudViewModel::StanceFor(false, false, false, TEXT(""), false, false, false, false).ToString(),
		FString(TEXT("STANDING")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOHudExtractionStatusTest,
	"ExecutiveOps.HUD.The commit slot offers the right thing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOHudExtractionStatusTest::RunTest(const FString& Parameters)
{
	// Inbound overrides the rest: once the craft is coming, the pad no longer
	// matters.
	TestEqual(TEXT("inbound wins over everything"),
		FEOHudViewModel::ExtractionStatusFor(true, false, false, false), EEOHudExtractionStatus::Inbound);
	TestEqual(TEXT("no zone in the level is no route"),
		FEOHudViewModel::ExtractionStatusFor(false, false, true, true), EEOHudExtractionStatus::NoRoute);
	TestEqual(TEXT("a zone that is not yet open is sealed"),
		FEOHudViewModel::ExtractionStatusFor(false, true, false, true), EEOHudExtractionStatus::Sealed);
	TestEqual(TEXT("open and standing on it offers extract"),
		FEOHudViewModel::ExtractionStatusFor(false, true, true, true), EEOHudExtractionStatus::Extract);
	TestEqual(TEXT("open but not there yet is pad open"),
		FEOHudViewModel::ExtractionStatusFor(false, true, true, false), EEOHudExtractionStatus::PadOpen);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
