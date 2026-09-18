#include "UI/EOHudPanelComponent.h"

#include "UI/EOHudSettings.h"
#include "UI/EOHudViewModel.h"
#include "UI/EOHudWidget.h"
#include "UI/EOPlayerHUD.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UEOHudPanelComponent::UEOHudPanelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// The same panel the procedural mesh drew: a gentle 84 degree sweep at a
	// full-screen resolution, so a layout authored for the viewport carries
	// across without being squeezed. Cylinder mode is the engine doing what the
	// old component built by hand.
	SetWidgetSpace(EWidgetSpace::World);
	SetGeometryMode(EWidgetGeometryMode::Cylinder);
	SetCylinderArcAngle(84.f);
	SetDrawSize(FVector2D(1920.f, 1080.f));
	SetBlendMode(EWidgetBlendMode::Transparent);
	SetTwoSided(false);
	SetDrawAtDesiredSize(false);

	// Nothing on the panel is clickable, and a panel that blocks traces is a
	// panel the aircraft's own collision sweeps can hit.
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
}

bool UEOHudPanelComponent::IsLive() const
{
	return GetWidgetClass() != nullptr && GetUserWidgetObject() != nullptr;
}

void UEOHudPanelComponent::BeginPlay()
{
	// The class comes from settings rather than a per-instance property, so the
	// viewport and the panel are assigned in the same place and stay in step.
	if (UClass* PanelClass = UEOHudSettings::Get().PanelWidget.LoadSynchronous())
	{
		SetWidgetClass(PanelClass);
	}

	Super::BeginPlay();

	TryBind();
}

void UEOHudPanelComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AEOPlayerHUD* Hud = BoundHud.Get())
	{
		Hud->OnViewModelBuilt.Remove(Handle);
	}
	BoundHud.Reset();
	Handle.Reset();

	Super::EndPlay(EndPlayReason);
}

void UEOHudPanelComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!BoundHud.IsValid())
	{
		TryBind();
	}
}

void UEOHudPanelComponent::TryBind()
{
	// Through the owning pawn's controller, so an aircraft nobody is flying
	// binds to nothing. The HUD is not guaranteed to exist at the pawn's
	// BeginPlay, which is why this is retried from the tick until it takes.
	const APawn* Owner = Cast<APawn>(GetOwner());
	const APlayerController* Controller = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
	AEOPlayerHUD* Hud = Controller ? Cast<AEOPlayerHUD>(Controller->GetHUD()) : nullptr;
	if (!Hud)
	{
		return;
	}

	BoundHud = Hud;
	Handle = Hud->OnViewModelBuilt.AddUObject(this, &UEOHudPanelComponent::HandleViewModel);
}

void UEOHudPanelComponent::HandleViewModel(const FEOHudViewModel& ViewModel)
{
	if (!IsVisible())
	{
		return;
	}

	// Named to avoid shadowing UWidgetComponent's own Widget member.
	if (UEOHudWidget* PanelWidget = Cast<UEOHudWidget>(GetUserWidgetObject()))
	{
		PanelWidget->Apply(ViewModel);
	}
}
