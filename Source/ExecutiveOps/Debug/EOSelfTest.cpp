#include "Debug/EOSelfTest.h"

#include "Core/EOPlayerController.h"
#include "GameFramework/Character.h"
#include "ExecutiveOps.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Mission/EOMissionSubsystem.h"

namespace
{
	int32 GFailures = 0;

	void Check(bool bCondition, const FString& Description)
	{
		if (bCondition)
		{
			UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest]   PASS  %s"), *Description);
		}
		else
		{
			++GFailures;
			UE_LOG(LogExecutiveOps, Error, TEXT("[SelfTest]   FAIL  %s"), *Description);
		}
	}

	FString StateName(const UEOMissionSubsystem* Mission)
	{
		return Mission ? Mission->GetMissionStateName() : TEXT("<no subsystem>");
	}
}

bool UEOSelfTest::IsRequested()
{
	return FParse::Param(FCommandLine::Get(), TEXT("EOSelfTest"));
}

bool UEOSelfTest::ShouldExitAfterRun()
{
	return FParse::Param(FCommandLine::Get(), TEXT("EOSelfTestExit"));
}

bool UEOSelfTest::Run(AEOPlayerController* Controller)
{
	GFailures = 0;
	UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] === M0 self-test ==="));

	if (!Controller)
	{
		UE_LOG(LogExecutiveOps, Error, TEXT("[SelfTest] no player controller"));
		return false;
	}

	UEOMissionSubsystem* Mission = Controller->GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	Check(Mission != nullptr, TEXT("mission subsystem exists"));

	// 1. Boot state.
	Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
		TEXT("boots possessing the aircraft"));
	Check(Mission && Mission->GetMissionState() == EEOMissionState::Inactive,
		FString::Printf(TEXT("boots with mission Inactive (got %s)"), *StateName(Mission)));

	APawn* BootPawn = Controller->GetPawn();
	Check(BootPawn != nullptr && BootPawn->Implements<UEOAircraftControlInterface>(),
		TEXT("possessed pawn implements the aircraft interface"));

	// 2. Deployment is refused until the aircraft is stabilised.
	if (BootPawn && BootPawn->Implements<UEOAircraftControlInterface>())
	{
		IEOAircraftControlInterface::Execute_SetHoverEnabled(BootPawn, false);
		Check(!IEOAircraftControlInterface::Execute_IsReadyForDeployment(BootPawn),
			TEXT("not ready to deploy while not hovering"));
		Check(!Controller->RequestDeployment(),
			TEXT("deployment refused while not hovering"));

		IEOAircraftControlInterface::Execute_SetHoverEnabled(BootPawn, true);
		Check(IEOAircraftControlInterface::Execute_IsReadyForDeployment(BootPawn),
			TEXT("ready to deploy while hovering and slow"));
	}

	// 3. Deploy: aircraft -> operative.
	Check(Controller->RequestDeployment(), TEXT("deployment accepted while hovering"));
	Check(Controller->GetControlMode() == EEOControlMode::Operative,
		TEXT("deployment possesses the operative"));
	Check(Mission && Mission->GetMissionState() == EEOMissionState::OnGround,
		FString::Printf(TEXT("deployment leaves mission OnGround (got %s)"), *StateName(Mission)));

	APawn* GroundPawn = Controller->GetPawn();
	Check(GroundPawn != nullptr && GroundPawn->IsA(ACharacter::StaticClass()),
		TEXT("operative pawn is a Character"));
	Check(GroundPawn != BootPawn, TEXT("operative is a different pawn to the aircraft"));

	// 4. Extract: operative -> aircraft.
	Check(Controller->RequestExtraction(), TEXT("extraction accepted"));
	Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
		TEXT("extraction returns control to the aircraft"));

	// 5. Reset returns to the boot state.
	Controller->EOReset();
	Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
		TEXT("reset leaves the player in the aircraft"));
	Check(Mission && Mission->GetMissionState() == EEOMissionState::Inactive,
		FString::Printf(TEXT("reset returns mission to Inactive (got %s)"), *StateName(Mission)));

	// 6. The state machine rejects illegal transitions.
	if (Mission)
	{
		Check(!Mission->CompleteMission(), TEXT("cannot complete a mission that never started"));
		Check(!Mission->BeginExtraction(), TEXT("cannot extract before the objective is done"));
		Check(Mission->GetMissionState() == EEOMissionState::Inactive,
			TEXT("rejected transitions leave state untouched"));
	}

	const bool bPassed = (GFailures == 0);
	UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] === %s (%d failure(s)) ==="),
		bPassed ? TEXT("PASSED") : TEXT("FAILED"), GFailures);
	return bPassed;
}
