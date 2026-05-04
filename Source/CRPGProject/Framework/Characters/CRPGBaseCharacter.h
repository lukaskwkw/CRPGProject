#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CRPGBaseCharacter.generated.h"

class UAbilitySystemComponent;
class UCRPGAttributeSet;

UCLASS()
class CRPGPROJECT_API ACRPGBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACRPGBaseCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	//// Called every frame
	//virtual void Tick(float DeltaTime) override;

	//// Called to bind functionality to input
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY()
	UCRPGAttributeSet* AttributeSet;
};