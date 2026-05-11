#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TacticalCombatHUDWidget.h"
#include "Input/Reply.h"
#include "TacticalPartyEntryWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class CRPGPROJECT_API UTacticalPartyEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Intercepts clicks so the party panel can request tactical unit selection. */
    virtual FReply NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    /** Mirrors hover state into the controller so any world preview is cleared while scrubbing the party list. */
    virtual void NativeOnMouseEnter(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    /** Releases transient UI-hover state after leaving the party entry. */
    virtual void NativeOnMouseLeave(const FPointerEvent &InMouseEvent) override;

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Updates the party-entry view model and notifies Blueprint to redraw. */
    void SetViewData(const FTacticalPartyEntryViewData &InViewData);

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    FTacticalPartyEntryViewData ViewData;

    UFUNCTION(BlueprintImplementableEvent, Category = "Tactical|HUD")
    /** Called after SetViewData so Blueprint visuals can refresh from the new data. */
    void OnViewDataChanged();
};