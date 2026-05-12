#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CRPGBaseCharacter.generated.h"

struct FCombatAttackResult;

class UAbilitySystemComponent;
class UCRPGAttributeSet;
class UCapsuleComponent;
class UTextRenderComponent;
class UTacticalUnitComponent;

UCLASS()
class CRPGPROJECT_API ACRPGBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACRPGBaseCharacter();

	virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	void EnterTacticalDeathState();
	// Controller hover targeting toggles this so combat UI can point at a world-space enemy without needing BP glue.
	void SetCombatTargetHighlightEnabled(bool bEnabled);
	// Emits a short-lived text label above the pawn for damage numbers, misses, and critical hits.
	void ShowCombatAttackResult(const FCombatAttackResult &AttackResult);
	// Mirrors UTacticalUnitComponent occupancy state into the hidden navigation blocker component.
	void UpdateTacticalOccupancyNavigationBlocker(const ACRPGBaseCharacter *ReferenceCharacter = nullptr);

	UFUNCTION(BlueprintPure, Category = "Tactical")
	UTacticalUnitComponent *GetTacticalUnitComponent() const;

	//// Called every frame
	// virtual void Tick(float DeltaTime) override;

	//// Called to bind functionality to input
	// virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UAbilitySystemComponent *AbilitySystemComponent;

	UPROPERTY()
	UCRPGAttributeSet *AttributeSet;

	// Tactical runtime data such as movement budget, initiative state and logical occupancy.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical")
	UTacticalUnitComponent *TacticalUnitComponent;

	// Invisible navigation-only footprint used to mark occupied space on the dynamic navmesh.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> TacticalOccupancyNavigationBlocker;

	// Lightweight code-only fallback for combat feedback until a dedicated widget/VFX presentation layer replaces it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> CombatFeedbackText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float CombatFeedbackDurationSeconds = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float CombatFeedbackHeightOffsetCm = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "255"))
	int32 CombatTargetHighlightStencilValue = 1;

	FTimerHandle CombatFeedbackHideTimerHandle;

	void ShowCombatFeedbackText(const FString &Text, const FColor &Color);
	void HideCombatFeedbackText();
};