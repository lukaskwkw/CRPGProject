#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CRPGAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class CRPGPROJECT_API UCRPGAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UCRPGAttributeSet();

    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UCRPGAttributeSet, Health);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS(UCRPGAttributeSet, Mana);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData ActionPoints;
    ATTRIBUTE_ACCESSORS(UCRPGAttributeSet, ActionPoints);
};