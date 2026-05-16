#include "Combat/Subsystems/CombatExecutionSubsystem.h"

#include "Combat/Subsystems/CombatResolverSubsystem.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Math/RotationMatrix.h"
#include "Tactical/Components/TacticalUnitComponent.h"

namespace CombatExecutionEvents
{
    static const FString AttackStarted = TEXT("combat_attack_started");
    static const FString AttackCommitted = TEXT("combat_attack_committed");
    static const FString AttackCancelled = TEXT("combat_attack_cancelled");
    static const FString AttackMissReactionTriggered = TEXT("combat_attack_miss_reaction_triggered");
}

bool UCombatExecutionSubsystem::RequestAttackExecution(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender, ECombatActionType ActionType, bool bTriggeredAfterTraversal)
{
    if (!IsValid(Attacker) || !IsValid(Defender) || Attacker == Defender || ActionType == ECombatActionType::None)
    {
        return false;
    }

    if (!Attacker->CanAct() || Defender->IsDead())
    {
        return false;
    }

    if (!QueuePendingAttack(Attacker, Defender, ActionType, bTriggeredAfterTraversal))
    {
        return false;
    }

    UGameInstance *GameInstance = GetGameInstance();
    UCombatResolverSubsystem *CombatResolverSubsystem = GameInstance ? GameInstance->GetSubsystem<UCombatResolverSubsystem>() : nullptr;
    if (!CombatResolverSubsystem)
    {
        CancelPendingAttack(Attacker);
        return false;
    }

    const FCombatAttackResult ResolvedResult = CombatResolverSubsystem->ResolveAttackForExecution(Attacker, Defender, ActionType);
    if (!ResolvedResult.bWasResolved)
    {
        CancelPendingAttack(Attacker);
        return false;
    }

    if (!MarkPendingAttackResolved(Attacker, ResolvedResult))
    {
        CancelPendingAttack(Attacker);
        return false;
    }

    PublishEvent(
        CombatExecutionEvents::AttackStarted,
        FString::Printf(
            TEXT("attacker=%s;defender=%s;action=%d;after_traversal=%s;resolved_hit=%s;attack_roll=%d;natural_roll=%d;critical=%s"),
            *GetNameSafe(Attacker->GetOwner()),
            *GetNameSafe(Defender->GetOwner()),
            static_cast<int32>(ActionType),
            bTriggeredAfterTraversal ? TEXT("true") : TEXT("false"),
            ResolvedResult.bHit ? TEXT("true") : TEXT("false"),
            ResolvedResult.AttackRoll,
            ResolvedResult.NaturalRoll,
            ResolvedResult.bCriticalHit ? TEXT("true") : TEXT("false")));

    if (PlayAttackMontage(Attacker, ActionType) <= 0.0f)
    {
        return ExecutePendingAttackAtHitWindow(Attacker);
    }

    return true;
}

bool UCombatExecutionSubsystem::QueuePendingAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender, ECombatActionType ActionType, bool bTriggeredAfterTraversal)
{
    if (!IsValid(Attacker) || !IsValid(Defender) || Attacker == Defender || ActionType == ECombatActionType::None)
    {
        return false;
    }

    const int32 ExistingIndex = FindPendingAttackIndex(Attacker);
    FPendingCombatAttack PendingAttack;
    PendingAttack.Attacker = Attacker;
    PendingAttack.Defender = Defender;
    PendingAttack.ActionType = ActionType;
    PendingAttack.bTriggeredAfterTraversal = bTriggeredAfterTraversal;

    if (ExistingIndex != INDEX_NONE)
    {
        PendingAttacks[ExistingIndex] = PendingAttack;
        return true;
    }

    PendingAttacks.Add(PendingAttack);
    return true;
}

bool UCombatExecutionSubsystem::HasPendingAttackForAttacker(const UTacticalUnitComponent *Attacker) const
{
    return FindPendingAttackIndex(Attacker) != INDEX_NONE;
}

bool UCombatExecutionSubsystem::TryGetPendingAttackForAttacker(const UTacticalUnitComponent *Attacker, FPendingCombatAttack &OutPendingAttack) const
{
    const int32 PendingAttackIndex = FindPendingAttackIndex(Attacker);
    if (PendingAttackIndex == INDEX_NONE)
    {
        return false;
    }

    OutPendingAttack = PendingAttacks[PendingAttackIndex];
    return true;
}

bool UCombatExecutionSubsystem::MarkPendingAttackResolved(UTacticalUnitComponent *Attacker, const FCombatAttackResult &ResolvedResult)
{
    const int32 PendingAttackIndex = FindPendingAttackIndex(Attacker);
    if (PendingAttackIndex == INDEX_NONE)
    {
        return false;
    }

    PendingAttacks[PendingAttackIndex].ResolvedResult = ResolvedResult;
    PendingAttacks[PendingAttackIndex].bHasResolvedResult = true;
    return true;
}

bool UCombatExecutionSubsystem::ExecutePendingAttackAtHitWindow(UTacticalUnitComponent *Attacker)
{
    FPendingCombatAttack PendingAttack;
    if (!ConsumePendingAttack(Attacker, PendingAttack) || !PendingAttack.IsValid())
    {
        return false;
    }

    if (!IsValid(PendingAttack.Attacker) || !IsValid(PendingAttack.Defender) || !PendingAttack.Attacker->CanAct() || PendingAttack.Defender->IsDead())
    {
        PublishEvent(
            CombatExecutionEvents::AttackCancelled,
            FString::Printf(
                TEXT("attacker=%s;defender=%s;action=%d"),
                *GetNameSafe(PendingAttack.Attacker ? PendingAttack.Attacker->GetOwner() : nullptr),
                *GetNameSafe(PendingAttack.Defender ? PendingAttack.Defender->GetOwner() : nullptr),
                static_cast<int32>(PendingAttack.ActionType)));
        return false;
    }

    if (!PendingAttack.bHasResolvedResult || !PendingAttack.ResolvedResult.bWasResolved)
    {
        return false;
    }

    const ECombatReactionDirection ReactionDirection = CalculateReactionDirection(PendingAttack.Defender, PendingAttack.Attacker);
    if (ACRPGBaseCharacter *DefenderCharacter = Cast<ACRPGBaseCharacter>(PendingAttack.Defender->GetOwner()))
    {
        DefenderCharacter->SetPendingCombatReactionDirection(ReactionDirection);
    }

    UGameInstance *GameInstance = GetGameInstance();
    UCombatResolverSubsystem *CombatResolverSubsystem = GameInstance ? GameInstance->GetSubsystem<UCombatResolverSubsystem>() : nullptr;
    if (!CombatResolverSubsystem)
    {
        return false;
    }

    const FCombatAttackResult &ResolvedResult = PendingAttack.ResolvedResult;
    CombatResolverSubsystem->CommitResolvedAttack(PendingAttack.Attacker, PendingAttack.Defender, PendingAttack.ActionType, ResolvedResult);

    if (ACRPGBaseCharacter *DefenderCharacter = Cast<ACRPGBaseCharacter>(PendingAttack.Defender->GetOwner()))
    {
        if (ResolvedResult.bHit)
        {
            if (!PendingAttack.Defender->IsDead() && !PendingAttack.Defender->IsProne())
            {
                DefenderCharacter->PlayHitReactMontageForDirection(ReactionDirection);
            }
        }
        else
        {
            if (!PendingAttack.bMissReactionTriggered)
            {
                DefenderCharacter->PlayDodgeMontageForDirection(ReactionDirection);
            }
        }
    }

    PublishEvent(
        CombatExecutionEvents::AttackCommitted,
        FString::Printf(
            TEXT("attacker=%s;defender=%s;action=%d;hit=%s;critical=%s;damage=%d;dead=%s;prone=%s"),
            *GetNameSafe(PendingAttack.Attacker->GetOwner()),
            *GetNameSafe(PendingAttack.Defender->GetOwner()),
            static_cast<int32>(PendingAttack.ActionType),
            ResolvedResult.bHit ? TEXT("true") : TEXT("false"),
            ResolvedResult.bCriticalHit ? TEXT("true") : TEXT("false"),
            ResolvedResult.DamageRoll,
            PendingAttack.Defender->IsDead() ? TEXT("true") : TEXT("false"),
            PendingAttack.Defender->IsProne() ? TEXT("true") : TEXT("false")));
    return true;
}

bool UCombatExecutionSubsystem::TriggerPendingMissReactionWindow(UTacticalUnitComponent *Attacker)
{
    const int32 PendingAttackIndex = FindPendingAttackIndex(Attacker);
    if (PendingAttackIndex == INDEX_NONE)
    {
        return false;
    }

    FPendingCombatAttack &PendingAttack = PendingAttacks[PendingAttackIndex];
    if (!PendingAttack.IsValid() || !PendingAttack.bHasResolvedResult || !PendingAttack.ResolvedResult.bWasResolved || PendingAttack.ResolvedResult.bHit || PendingAttack.bMissReactionTriggered)
    {
        return false;
    }

    if (!IsValid(PendingAttack.Defender) || PendingAttack.Defender->IsDead())
    {
        return false;
    }

    const ECombatReactionDirection ReactionDirection = CalculateReactionDirection(PendingAttack.Defender, PendingAttack.Attacker);
    if (ACRPGBaseCharacter *DefenderCharacter = Cast<ACRPGBaseCharacter>(PendingAttack.Defender->GetOwner()))
    {
        DefenderCharacter->PlayDodgeMontageForDirection(ReactionDirection);
        PendingAttack.bMissReactionTriggered = true;

        PublishEvent(
            CombatExecutionEvents::AttackMissReactionTriggered,
            FString::Printf(
                TEXT("attacker=%s;defender=%s;action=%d;direction=%d"),
                *GetNameSafe(PendingAttack.Attacker->GetOwner()),
                *GetNameSafe(PendingAttack.Defender->GetOwner()),
                static_cast<int32>(PendingAttack.ActionType),
                static_cast<int32>(ReactionDirection)));
        return true;
    }

    return false;
}

bool UCombatExecutionSubsystem::ConsumePendingAttack(UTacticalUnitComponent *Attacker, FPendingCombatAttack &OutPendingAttack)
{
    const int32 PendingAttackIndex = FindPendingAttackIndex(Attacker);
    if (PendingAttackIndex == INDEX_NONE)
    {
        return false;
    }

    OutPendingAttack = PendingAttacks[PendingAttackIndex];
    PendingAttacks.RemoveAt(PendingAttackIndex);
    return true;
}

void UCombatExecutionSubsystem::CancelPendingAttack(UTacticalUnitComponent *Attacker)
{
    const int32 PendingAttackIndex = FindPendingAttackIndex(Attacker);
    if (PendingAttackIndex != INDEX_NONE)
    {
        PendingAttacks.RemoveAt(PendingAttackIndex);
    }
}

int32 UCombatExecutionSubsystem::FindPendingAttackIndex(const UTacticalUnitComponent *Attacker) const
{
    if (!Attacker)
    {
        return INDEX_NONE;
    }

    for (int32 AttackIndex = 0; AttackIndex < PendingAttacks.Num(); ++AttackIndex)
    {
        if (PendingAttacks[AttackIndex].Attacker == Attacker)
        {
            return AttackIndex;
        }
    }

    return INDEX_NONE;
}

float UCombatExecutionSubsystem::PlayAttackMontage(UTacticalUnitComponent *Attacker, ECombatActionType ActionType) const
{
    ACRPGBaseCharacter *AttackerCharacter = Attacker ? Cast<ACRPGBaseCharacter>(Attacker->GetOwner()) : nullptr;
    if (!AttackerCharacter)
    {
        return 0.0f;
    }

    switch (ActionType)
    {
    case ECombatActionType::MeleeAttack:
        return AttackerCharacter->PlayMeleeAttackMontage();
    case ECombatActionType::RangedAttack:
        return AttackerCharacter->PlayRangedAttackMontage();
    default:
        return 0.0f;
    }
}

ECombatReactionDirection UCombatExecutionSubsystem::CalculateReactionDirection(const UTacticalUnitComponent *Receiver, const UTacticalUnitComponent *Source) const
{
    const AActor *ReceiverActor = Receiver ? Receiver->GetOwner() : nullptr;
    const AActor *SourceActor = Source ? Source->GetOwner() : nullptr;
    if (!ReceiverActor || !SourceActor)
    {
        return ECombatReactionDirection::Front;
    }

    FVector ToSource = SourceActor->GetActorLocation() - ReceiverActor->GetActorLocation();
    ToSource.Z = 0.0f;
    if (!ToSource.Normalize())
    {
        return ECombatReactionDirection::Front;
    }

    float FacingYawDegrees = ReceiverActor->GetActorRotation().Yaw;
    if (const ACRPGBaseCharacter *ReceiverCharacter = Cast<ACRPGBaseCharacter>(ReceiverActor))
    {
        FacingYawDegrees += ReceiverCharacter->GetCombatReactionFacingYawOffsetDegrees();
    }

    const FRotator FacingRotation(0.0f, FacingYawDegrees, 0.0f);
    const FVector ReceiverForward = FRotationMatrix(FacingRotation).GetUnitAxis(EAxis::X).GetSafeNormal2D();
    const FVector ReceiverRight = FRotationMatrix(FacingRotation).GetUnitAxis(EAxis::Y).GetSafeNormal2D();

    const float ForwardDot = FVector::DotProduct(ReceiverForward, ToSource);
    const float RightDot = FVector::DotProduct(ReceiverRight, ToSource);
    const float AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));

    if (AngleDegrees > -45.0f && AngleDegrees <= 45.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("[CombatExecutionSubsystem] Reaction direction Front for receiver=%s source=%s angle=%.2f forwardDot=%.3f rightDot=%.3f facingYaw=%.2f"), *GetNameSafe(ReceiverActor), *GetNameSafe(SourceActor), AngleDegrees, ForwardDot, RightDot, FacingYawDegrees);
        return ECombatReactionDirection::Front;
    }

    if (AngleDegrees > 45.0f && AngleDegrees <= 135.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("[CombatExecutionSubsystem] Reaction direction Right for receiver=%s source=%s angle=%.2f forwardDot=%.3f rightDot=%.3f facingYaw=%.2f"), *GetNameSafe(ReceiverActor), *GetNameSafe(SourceActor), AngleDegrees, ForwardDot, RightDot, FacingYawDegrees);
        return ECombatReactionDirection::Right;
    }

    if (AngleDegrees <= -45.0f && AngleDegrees > -135.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("[CombatExecutionSubsystem] Reaction direction Left for receiver=%s source=%s angle=%.2f forwardDot=%.3f rightDot=%.3f facingYaw=%.2f"), *GetNameSafe(ReceiverActor), *GetNameSafe(SourceActor), AngleDegrees, ForwardDot, RightDot, FacingYawDegrees);
        return ECombatReactionDirection::Left;
    }

    UE_LOG(LogTemp, Log, TEXT("[CombatExecutionSubsystem] Reaction direction Back for receiver=%s source=%s angle=%.2f forwardDot=%.3f rightDot=%.3f facingYaw=%.2f"), *GetNameSafe(ReceiverActor), *GetNameSafe(SourceActor), AngleDegrees, ForwardDot, RightDot, FacingYawDegrees);
    return ECombatReactionDirection::Back;
}

void UCombatExecutionSubsystem::PublishEvent(const FString &EventName, const FString &Payload) const
{
    UGameInstance *GameInstance = GetGameInstance();
    UEventBusSubsystem *EventBusSubsystem = GameInstance ? GameInstance->GetSubsystem<UEventBusSubsystem>() : nullptr;
    if (EventBusSubsystem)
    {
        EventBusSubsystem->PublishEvent(EventName, Payload);
    }
}