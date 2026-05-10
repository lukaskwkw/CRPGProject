#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CRPGBaseCharacter.generated.h"

class UAbilitySystemComponent;
class UCRPGAttributeSet;
class UCapsuleComponent;
class UTacticalUnitComponent;

UCLASS()
class CRPGPROJECT_API ACRPGBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACRPGBaseCharacter();

	virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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
};