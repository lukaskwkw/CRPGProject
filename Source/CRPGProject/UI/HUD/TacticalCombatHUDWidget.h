#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"
#include "TacticalCombatHUDWidget.generated.h"

class UButton;
class UEventBusSubsystem;
class UPanelWidget;
class UTextBlock;
class ACRPGBaseCharacter;
class UTacticalInitiativeEntryWidget;
class UTacticalPartyEntryWidget;
class UTacticalTurnSubsystem;
class ACRPGProjectPlayerController;
class UTexture2D;

UENUM(BlueprintType)
enum class ETacticalHUDActionType : uint8
{
    None,
    MeleeAttack,
    RangedAttack,
    EndTurn,
    GoToActive
};

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FTacticalInitiativeEntryViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    TObjectPtr<ACRPGBaseCharacter> Unit = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    int32 Initiative = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    TObjectPtr<UTexture2D> PortraitTexture = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bIsAlive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bIsActive = false;
};

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FTacticalPartyEntryViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    TObjectPtr<ACRPGBaseCharacter> Unit = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    TObjectPtr<UTexture2D> PortraitTexture = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bIsAlive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bIsActive = false;
};

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FTacticalActionEntryViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    ETacticalHUDActionType ActionType = ETacticalHUDActionType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    FText Label;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bIsEnabled = false;
};

UCLASS()
class CRPGPROJECT_API UTacticalCombatHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Builds the initial HUD state once the widget is constructed. */
    virtual void NativeConstruct() override;
    /** Clears any live hover-preview when the cursor enters the tactical HUD root. */
    virtual void NativeOnMouseEnter(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    /** Releases transient HUD hover state when the cursor leaves the tactical HUD root. */
    virtual void NativeOnMouseLeave(const FPointerEvent &InMouseEvent) override;

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Toggles tactical turn mode from the HUD button. */
    void OnToggleTurnModePressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Ends the currently active unit turn when that unit is selected. */
    void OnEndTurnPressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Moves the tactical camera to the currently active unit. */
    void OnGoToActivePressed();

    UFUNCTION(BlueprintPure, Category = "Tactical|HUD")
    /** Text binding for the dedicated End Turn button. */
    FText GetEndTurnActionLabel() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|HUD")
    /** Text binding for the dedicated Go To Active button. */
    FText GetGoToActiveActionLabel() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|HUD")
    /** Returns whether the cursor is currently over a tactical HUD region that should block world hover preview. */
    bool IsPointerOverBlockingUI() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Requests a melee attack event for the active unit. */
    void OnMeleeAttackPressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Requests a ranged attack event for the active unit. */
    void OnRangedAttackPressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Rebuilds the widget state from tactical subsystem and controller data. */
    void RefreshFromSubsystem();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Commits the currently previewed tactical move. */
    void OnConfirmMovePressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Cancels the currently previewed tactical move. */
    void OnCancelMovePressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Toggles tactical movement input without leaving turn mode. */
    void OnToggleMovementEnabledPressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Explicitly enables or disables tactical movement input. */
    void SetMovementEnabled(bool bEnabled);

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bTurnModeEnabled = false;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    int32 RoundNumber = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD", meta = (Units = "cm"))
    float RemainingMovementRangeCm = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD", meta = (Units = "cm"))
    float PlannedMoveDistanceCm = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD", meta = (Units = "cm"))
    float RemainingMovementAfterPlannedMoveCm = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    int32 ActionPoints = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    int32 BonusActionPoints = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    ETacticalTurnState CurrentTurnState = ETacticalTurnState::Disabled;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    FString TurnModeLabel = TEXT("OFF");

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bHasPendingMovePreview = false;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bPendingMoveValid = false;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bMovementEnabled = true;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    TArray<FTacticalInitiativeEntryViewData> InitiativeEntries;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    TArray<FTacticalPartyEntryViewData> PartyEntries;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    TArray<FTacticalActionEntryViewData> ActionEntries;

    UFUNCTION(BlueprintImplementableEvent, Category = "Tactical|HUD")
    void OnTacticalCombatDataRefreshed();

private:
    /** Clears any hover-preview path while the cursor is interacting with HUD widgets. */
    void ClearPendingMovePreviewFromUI() const;
    /** Returns true when the cursor is over a real child entry/button, not just the fullscreen root widget. */
    bool IsAnyPanelChildHovered(const UPanelWidget *PanelWidget) const;

    /** Clears cached initiative, party, and action view-model data. */
    void ResetEncounterWidgetData();
    /** Rebuilds initiative, party, and action view-models from the tactical subsystem. */
    void RebuildEncounterWidgetData(const UTacticalTurnSubsystem &TacticalTurnSubsystem);
    /** Rebuilds the action list based on whether the current active unit can be focused or act. */
    void RebuildActionEntries(const UTacticalTurnSubsystem *TacticalTurnSubsystem, const class UTacticalUnitComponent *ActiveUnitComponent);
    /** Recreates all child widgets for initiative and party sections. */
    void RebuildEncounterWidgetChildren();
    /** Recreates initiative-entry widgets from the current view-model list. */
    void RebuildInitiativeBarChildren();
    /** Recreates party-entry widgets from the current player-party view-model list. */
    void RebuildPartyPanelChildren();
    /** Mirrors action availability into the optional UMG button bindings. */
    void SyncActionButtonStates();
    /** Publishes HUD action requests onto the EventBus for gameplay systems to consume. */
    void PublishActionRequestedEvent(const FString &EventName) const;

    /** Returns the owning CRPG player controller, if present. */
    ACRPGProjectPlayerController *GetOwningCRPGPlayerController() const;
    /** Returns the tactical turn subsystem from the current game instance. */
    UTacticalTurnSubsystem *GetTacticalTurnSubsystem() const;
    /** Returns the EventBus subsystem used to broadcast HUD action intents. */
    UEventBusSubsystem *GetEventBusSubsystem() const;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> MeleeAttackButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> RangedAttackButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> EndTurnButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> GoToActiveButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> InitiativeBarContainer = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> PlayerPartyContainer = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical|HUD", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UTacticalInitiativeEntryWidget> InitiativeEntryWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical|HUD", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UTacticalPartyEntryWidget> PartyEntryWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical|HUD", meta = (AllowPrivateAccess = "true"))
    FText EndTurnActionLabel = FText::FromString(TEXT("End Turn"));

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical|HUD", meta = (AllowPrivateAccess = "true"))
    FText GoToActiveActionLabel = FText::FromString(TEXT("Go To Active"));
};
