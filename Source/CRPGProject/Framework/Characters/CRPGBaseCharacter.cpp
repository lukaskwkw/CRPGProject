#include "CRPGBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "CRPGAttributeSet.h"
#include "Engine/GameInstance.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

ACRPGBaseCharacter::ACRPGBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UCRPGAttributeSet>(TEXT("AttributeSet"));
    TacticalUnitComponent = CreateDefaultSubobject<UTacticalUnitComponent>(TEXT("TacticalUnitComponent"));
}

UAbilitySystemComponent* ACRPGBaseCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ACRPGBaseCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UTacticalTurnSubsystem* TacticalTurnSubsystem = GameInstance->GetSubsystem<UTacticalTurnSubsystem>())
        {
            TacticalTurnSubsystem->RegisterUnit(this);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[CRPGBaseCharacter] GAS Initialized for %s"), *GetName());
}

void ACRPGBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UTacticalTurnSubsystem* TacticalTurnSubsystem = GameInstance->GetSubsystem<UTacticalTurnSubsystem>())
        {
            TacticalTurnSubsystem->UnregisterUnit(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

UTacticalUnitComponent* ACRPGBaseCharacter::GetTacticalUnitComponent() const
{
    return TacticalUnitComponent;
}