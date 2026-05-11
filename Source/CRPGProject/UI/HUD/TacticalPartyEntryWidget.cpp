#include "UI/HUD/TacticalPartyEntryWidget.h"

#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "Input/Events.h"

void UTacticalPartyEntryWidget::SetViewData(const FTacticalPartyEntryViewData &InViewData)
{
    // Native code owns the authoritative data payload, while Blueprint stays responsible only for presentation.
    ViewData = InViewData;
    OnViewDataChanged();
}

void UTacticalPartyEntryWidget::NativeOnMouseEnter(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (ACRPGProjectPlayerController *PlayerController = GetOwningPlayer<ACRPGProjectPlayerController>())
    {
        PlayerController->NotifyTacticalUIHoverBegin();
    }
}

void UTacticalPartyEntryWidget::NativeOnMouseLeave(const FPointerEvent &InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (ACRPGProjectPlayerController *PlayerController = GetOwningPlayer<ACRPGProjectPlayerController>())
    {
        PlayerController->NotifyTacticalUIHoverEnd();
    }
}

FReply UTacticalPartyEntryWidget::NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent)
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

    // Party entries are stricter than initiative entries: they only ever request ally selection/possession,
    // never enemy focus, because this panel represents the player-controlled roster rather than turn order.
    return PlayerController->SelectPlayerPartyUnit(ViewData.Unit) ? FReply::Handled() : Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}