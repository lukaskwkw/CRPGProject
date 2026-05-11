#include "UI/HUD/TacticalInitiativeEntryWidget.h"

#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "Input/Events.h"

void UTacticalInitiativeEntryWidget::NativeOnMouseEnter(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (ACRPGProjectPlayerController *PlayerController = GetOwningPlayer<ACRPGProjectPlayerController>())
    {
        // Entry widgets still clear any already-visible preview on hover, even though preview gating itself
        // now relies on direct hovered-widget checks from the HUD instead of a fullscreen root hover state.
        PlayerController->NotifyTacticalUIHoverBegin();
    }
}

void UTacticalInitiativeEntryWidget::NativeOnMouseLeave(const FPointerEvent &InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (ACRPGProjectPlayerController *PlayerController = GetOwningPlayer<ACRPGProjectPlayerController>())
    {
        PlayerController->NotifyTacticalUIHoverEnd();
    }
}

FReply UTacticalInitiativeEntryWidget::NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    ACRPGProjectPlayerController *PlayerController = GetOwningPlayer<ACRPGProjectPlayerController>();
    if (!PlayerController || !ViewData.Unit)
    {
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    // Initiative clicks intentionally reuse the controller's shared selection path so allies can be possessed,
    // while non-player units fall back to focus-only behavior unless the narrow debug override is enabled.
    return PlayerController->SelectOrFocusTacticalUnit(ViewData.Unit) ? FReply::Handled() : Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UTacticalInitiativeEntryWidget::SetViewData(const FTacticalInitiativeEntryViewData &InViewData)
{
    ViewData = InViewData;
    OnViewDataChanged();
}