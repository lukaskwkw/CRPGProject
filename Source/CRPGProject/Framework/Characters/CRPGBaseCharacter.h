#pragma once

#include "Combat/Types/CombatTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CRPGBaseCharacter.generated.h"

struct FCombatAttackResult;

class UAbilitySystemComponent;
class UAnimMontage;
class UCRPGAttributeSet;
class UCapsuleComponent;
class UTextRenderComponent;
class UTacticalUnitComponent;

UENUM(BlueprintType)
enum class ECRPGOutlineCategory : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	HoveredEnemy = 1 UMETA(DisplayName = "Hovered Enemy"),
	PartyMember = 2 UMETA(DisplayName = "Party Member"),
	ActivePartyMember = 3 UMETA(DisplayName = "Active Party Member"),
	FriendlyNonParty = 4 UMETA(DisplayName = "Friendly Non-Party"),
	Neutral = 5 UMETA(DisplayName = "Neutral"),
	Enemy = 6 UMETA(DisplayName = "Enemy"),
	Interactable = 7 UMETA(DisplayName = "Interactable")
};

UCLASS()
class CRPGPROJECT_API ACRPGBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACRPGBaseCharacter();

	virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayMeleeAttackMontage();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayRangedAttackMontage();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayDodgeMontage();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayDodgeMontageForDirection(ECombatReactionDirection Direction);
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayHitReactMontage();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayHitReactMontageForDirection(ECombatReactionDirection Direction);
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	void EnterTacticalDeathState();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayDeathMontage();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayDeathMontageForDirection(ECombatReactionDirection Direction);
	void SetPendingCombatReactionDirection(ECombatReactionDirection Direction);
	bool ConsumePendingCombatReactionDirection(ECombatReactionDirection &OutDirection);
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	void EnterTacticalProneState();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayProneMontage();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	void RecoverFromPronePresentation();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	float PlayStandUpMontage();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
	void EnableCombatRagdoll();
	// Controller hover targeting toggles this so combat UI can point at a world-space enemy without needing BP glue.
	void SetCombatTargetHighlightEnabled(bool bEnabled);
	// Stores the authored/default outline category configured on the asset or changed explicitly by gameplay code.
	UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
	void SetOutlineCategory(ECRPGOutlineCategory NewCategory);
	UFUNCTION(BlueprintPure, Category = "Tactical|Outline")
	ECRPGOutlineCategory GetOutlineCategory() const { return OutlineCategory; }
	UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
	void ClearOutlineCategory();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
	void SetOutlineCategoryToPartyMember();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
	void SetOutlineCategoryToActivePartyMember();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
	void SetOutlineCategoryToFriendlyNonParty();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
	void SetOutlineCategoryToNeutral();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
	void SetOutlineCategoryToEnemy();
	UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
	void SetOutlineCategoryToInteractable();
	// Stores a transient runtime override used by controller overlays without destroying the authored category.
	void SetRuntimeOutlineCategory(ECRPGOutlineCategory NewCategory);
	UFUNCTION(BlueprintPure, Category = "Tactical|Outline")
	ECRPGOutlineCategory GetRuntimeOutlineCategory() const { return RuntimeOutlineCategory; }
	// Explicit exclusion gate for invisible/hidden actors that should never participate in shared outline overlays.
	void SetOutlineExcluded(bool bExcluded);
	UFUNCTION(BlueprintPure, Category = "Tactical|Outline")
	bool IsOutlineExcluded() const { return bOutlineExcluded; }
	// Emits a short-lived text label above the pawn for damage numbers, misses, and critical hits.
	void ShowCombatAttackResult(const FCombatAttackResult &AttackResult);
	// Mirrors UTacticalUnitComponent occupancy state into the hidden navigation blocker component.
	void UpdateTacticalOccupancyNavigationBlocker(const ACRPGBaseCharacter *ReferenceCharacter = nullptr);

	UFUNCTION(BlueprintPure, Category = "Tactical")
	UTacticalUnitComponent *GetTacticalUnitComponent() const;
	float GetCombatReactionFacingYawOffsetDegrees() const { return CombatReactionFacingYawOffsetDegrees; }

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float CombatFeedbackFloatDistanceCm = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CombatFeedbackFadeStartFraction = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> RangedAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ProneMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StandUpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float ProneCapsuleHalfHeightCm = 44.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Combat|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "-180.0", ClampMax = "180.0", Units = "deg"))
	float CombatReactionFacingYawOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Outline", meta = (AllowPrivateAccess = "true"))
	ECRPGOutlineCategory OutlineCategory = ECRPGOutlineCategory::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Outline", meta = (AllowPrivateAccess = "true"))
	bool bShowOutlineCategoryWhenIdle = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Outline", meta = (AllowPrivateAccess = "true"))
	bool bOutlineExcluded = false;

	UPROPERTY(Transient)
	ECRPGOutlineCategory RuntimeOutlineCategory = ECRPGOutlineCategory::None;

	FTimerHandle CombatFeedbackHideTimerHandle;
	FVector CombatFeedbackBaseRelativeLocation = FVector::ZeroVector;
	FColor CombatFeedbackActiveColor = FColor::White;
	float CombatFeedbackElapsedSeconds = 0.0f;
	float StandingCapsuleHalfHeightCm = 0.0f;
	ECombatReactionDirection PendingCombatReactionDirection = ECombatReactionDirection::Front;
	bool bCombatTargetHighlightEnabled = false;
	bool bCombatFeedbackAnimating = false;
	bool bHasPendingCombatReactionDirection = false;

	float PlayConfiguredMontage(UAnimMontage *Montage);
	float PlayConfiguredMontageRandomSection(UAnimMontage *Montage);
	float PlayConfiguredMontageSection(UAnimMontage *Montage, ECombatReactionDirection Direction);
	FName GetReactionDirectionSectionName(ECombatReactionDirection Direction) const;
	void RefreshOutlinePresentation();
	void ShowCombatFeedbackText(const FString &Text, const FColor &Color);
	void HideCombatFeedbackText();
	void UpdateCombatFeedbackPresentation(float DeltaSeconds);
};