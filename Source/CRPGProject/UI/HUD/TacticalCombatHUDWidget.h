#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"
#include "TacticalCombatHUDWidget.generated.h"

class UTacticalTurnSubsystem;
class ACRPGProjectPlayerController;

UCLASS()
class CRPGPROJECT_API UTacticalCombatHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    void OnToggleTurnModePressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    void OnEndTurnPressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    void RefreshFromSubsystem();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    void OnConfirmMovePressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    void OnCancelMovePressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    void OnToggleMovementEnabledPressed();

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
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

    UFUNCTION(BlueprintImplementableEvent, Category = "Tactical|HUD")
    void OnTacticalCombatDataRefreshed();

private:
   ACRPGProjectPlayerController* GetOwningCRPGPlayerController() const;
    UTacticalTurnSubsystem* GetTacticalTurnSubsystem() const;
};
