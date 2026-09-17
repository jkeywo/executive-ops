#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Combat/EOWeaponComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

/**
 * The weapon's own decisions, tested without playing the game.
 *
 * These are checks that could not previously exist. The pistol lived inside a
 * 1,324-line character, so the only way to observe a shot was to fly the
 * approach, drop in, walk up to the guard and take one - which is why the gun
 * check was intermittent across three commits, and why two of the three
 * explanations for it were wrong.
 *
 * The scope is deliberate. What the weapon decides - am I ready, where does the
 * round actually go - is asserted here. What the world then does with the round
 * - collision, damage, reactions - is not: that needs real geometry and a real
 * BeginPlay, which a synthetic world does not give you, and it belongs in an
 * AFunctionalTest in one of the maps. Docs/adr/0007 splits the two on exactly
 * this line.
 */
namespace EOWeaponTestHelpers
{
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

		UEOWeaponComponent* SpawnWeapon() const
		{
			AActor* Shooter = World->SpawnActor<AActor>(AActor::StaticClass());
			UEOWeaponComponent* Weapon = NewObject<UEOWeaponComponent>(Shooter);
			Weapon->RegisterComponent();
			return Weapon;
		}

		UWorld* World = nullptr;
		FWorldContext* Context = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOWeaponRespectsCooldownTest,
	"ExecutiveOps.Weapon.A second shot inside the interval is refused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOWeaponRespectsCooldownTest::RunTest(const FString& Parameters)
{
	EOWeaponTestHelpers::FScopedTestWorld Scope;
	UEOWeaponComponent* Weapon = Scope.SpawnWeapon();

	TestTrue(TEXT("a new weapon is ready"), Weapon->IsReady());
	TestTrue(TEXT("the first shot fires"),
		Weapon->TryFire(FVector::ZeroVector, FVector(500.f, 0.f, 0.f)).bFired);

	TestFalse(TEXT("the weapon is no longer ready"), Weapon->IsReady());
	TestEqual(TEXT("and it is cooling down for the full interval"),
		Weapon->GetCooldownRemaining(), Weapon->Interval);

	TestFalse(TEXT("so a second shot is refused outright"),
		Weapon->TryFire(FVector::ZeroVector, FVector(500.f, 0.f, 0.f)).bFired);

	// Resetting is how the reset command and the encounter checks put the weapon
	// back to a known state between runs.
	Weapon->ResetWeapon();
	TestTrue(TEXT("after a reset it is ready again"), Weapon->IsReady());
	TestTrue(TEXT("and fires"),
		Weapon->TryFire(FVector::ZeroVector, FVector(500.f, 0.f, 0.f)).bFired);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEOWeaponSpreadTest,
	"ExecutiveOps.Weapon.Spread stays inside its cone, and aiming removes it",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEOWeaponSpreadTest::RunTest(const FString& Parameters)
{
	EOWeaponTestHelpers::FScopedTestWorld Scope;
	UEOWeaponComponent* Weapon = Scope.SpawnWeapon();

	const FVector Muzzle = FVector::ZeroVector;
	const FVector AimPoint = FVector(1000.f, 0.f, 0.f);
	const FVector Intended = (AimPoint - Muzzle).GetSafeNormal();

	// This is the property the flaky gun check actually hinged on, and it was
	// only ever observable by taking real shots at a real guard: the cone is
	// centred on the shot line, so half of it falls outside a target the round
	// is aimed at the rim of. Asserting the bound directly is the point of
	// giving the weapon an interface of its own.
	Weapon->HipSpread = 4.5f;
	for (int32 Shot = 0; Shot < 64; ++Shot)
	{
		Weapon->ResetWeapon();
		const FEOShotResult Result = Weapon->TryFire(Muzzle, AimPoint);

		const float OffAxis = FMath::RadiansToDegrees(FMath::Acos(
			FMath::Clamp(FVector::DotProduct(Result.Direction, Intended), -1.f, 1.f)));

		if (!TestTrue(FString::Printf(
				TEXT("hip shot %d stayed inside the 4.5 degree cone (was %.2f)"), Shot, OffAxis),
			OffAxis <= 4.5f + KINDA_SMALL_NUMBER))
		{
			return false;
		}
	}

	// Aiming is the only reason to aim: it replaces the cone with a far tighter
	// one, so the round goes where the crosshair is.
	Weapon->AimSpread = 0.f;
	Weapon->ResetWeapon();
	const FEOShotResult Aimed = Weapon->TryFire(Muzzle, AimPoint, /*bAccurate=*/true);

	TestTrue(TEXT("an aimed shot with no spread goes exactly where it was aimed"),
		Aimed.Direction.Equals(Intended, KINDA_SMALL_NUMBER));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
