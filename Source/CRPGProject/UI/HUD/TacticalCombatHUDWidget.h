#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"
#include "TacticalCombatHUDWidget.generated.h"

class UTacticalTurnSubsystem;

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

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    bool bTurnModeEnabled = false;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    int32 RoundNumber = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD", meta = (Units = "cm"))
    float RemainingMovementRangeCm = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    int32 ActionPoints = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    int32 BonusActionPoints = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    ETacticalTurnState CurrentTurnState = ETacticalTurnState::Disabled;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    FString TurnModeLabel = TEXT("OFF");

    UFUNCTION(BlueprintImplementableEvent, Category = "Tactical|HUD")
    void OnTacticalCombatDataRefreshed();

private:
    UTacticalTurnSubsystem* GetTacticalTurnSubsystem() const;
};
