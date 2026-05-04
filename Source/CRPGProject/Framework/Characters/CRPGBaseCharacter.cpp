#include "CRPGBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "CRPGAttributeSet.h"

ACRPGBaseCharacter::ACRPGBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UCRPGAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ACRPGBaseCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ACRPGBaseCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[CRPGBaseCharacter] GAS Initialized for %s"), *GetName());
}