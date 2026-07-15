#include "HunterCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayCueInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "BBCharacterMovementComponent.h"
#include "GliderComponent.h"
#include "BBMantleComponent.h"
#include "Breachborne/UI/GliderFuelWidget.h"
#include "Net/UnrealNetwork.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBAbilityVisualSet.h"
#include "Breachborne/Abilities/BBHunterDefinition.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Breachborne.h"

namespace
{
	void SpawnHudsonLocalCueFallback(AHunterCharacter* Hunter, const FBBLocalGameplayCue& Cue)
	{
		if (!Hunter || Hunter->GetNetMode() == NM_DedicatedServer || !Cue.CueTag.IsValid())
		{
			return;
		}

		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (Cue.CueTag == BBGameplayTags::GameplayCue_Hunter_Hudson_LMB_Fire)
		{
			if (ABBPrimitiveBeamActor* Beam = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
				ABBPrimitiveBeamActor::StaticClass(), Cue.Location, FRotator::ZeroRotator, Params))
			{
				Beam->SetReplicates(false);
				const FVector Direction = Cue.Normal.IsNearlyZero() ? Hunter->GetActorForwardVector() : FVector(Cue.Normal).GetSafeNormal();
				Beam->InitBeam(Cue.Location, Cue.Location + Direction * 1300.0f, 2.5f, 0.09f,
					FLinearColor(1.0f, 0.54f, 0.16f, 1.0f));
			}
		}
		else if (Cue.CueTag == BBGameplayTags::GameplayCue_Hunter_Hudson_LMB_Impact
			|| Cue.CueTag == BBGameplayTags::GameplayCue_Hunter_Hudson_LMB_Heal)
		{
			if (ABBPrimitiveBurstActor* Burst = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
				ABBPrimitiveBurstActor::StaticClass(), Cue.Location, FRotator::ZeroRotator, Params))
			{
				Burst->SetReplicates(false);
				const bool bHeal = Cue.CueTag == BBGameplayTags::GameplayCue_Hunter_Hudson_LMB_Heal;
				Burst->InitBurst(Cue.Location, bHeal ? 34.0f : 20.0f, bHeal ? 0.2f : 0.14f,
					bHeal ? FLinearColor(0.31f, 0.64f, 0.78f, 1.0f) : FLinearColor(1.0f, 0.82f, 0.4f, 1.0f));
			}
		}
	}
}

AHunterCharacter::AHunterCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBBCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// --- Glider Component ---
	GliderComponent = CreateDefaultSubobject<UGliderComponent>(TEXT("GliderComponent"));

	// --- Mantle Component ---
	MantleComponent = CreateDefaultSubobject<UBBMantleComponent>(TEXT("MantleComponent"));

	// --- Placeholder Visual (cylinder for testing — replace with real mesh later) ---
	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(RootComponent);
	PlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	PlaceholderMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(CylinderMesh.Object);
	}

	// --- Facing Indicator (cone pointing forward — shows aim direction) ---
	FacingIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingIndicator"));
	FacingIndicator->SetupAttachment(RootComponent);
	// Position at front of character, slightly above center
	FacingIndicator->SetRelativeLocation(FVector(40.0f, 0.0f, 20.0f));
	// Rotate so the cone tip points along local +X (forward)
	FacingIndicator->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	FacingIndicator->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.5f));
	FacingIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(
		TEXT("/Engine/BasicShapes/Cone"));
	if (ConeMesh.Succeeded())
	{
		FacingIndicator->SetStaticMesh(ConeMesh.Object);
	}

	// --- Glider Indicator (flat plane hovering above character — hidden until gliding) ---
	GliderIndicatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GliderIndicatorMesh"));
	GliderIndicatorMesh->SetupAttachment(RootComponent);
	GliderIndicatorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	GliderIndicatorMesh->SetRelativeScale3D(FVector(1.2f, 0.8f, 1.0f));
	GliderIndicatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GliderIndicatorMesh->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane"));
	if (PlaneMesh.Succeeded())
	{
		GliderIndicatorMesh->SetStaticMesh(PlaneMesh.Object);
	}

	GliderFuelWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("GliderFuelWidget"));
	GliderFuelWidgetComp->SetupAttachment(RootComponent);
	GliderFuelWidgetComp->SetRelativeLocation(FVector(-35.0f, -70.0f, 150.0f));
	GliderFuelWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	GliderFuelWidgetComp->SetDrawSize(FVector2D(34.0f, 34.0f));
	GliderFuelWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GliderFuelWidgetComp->SetVisibility(true);
	GliderFuelWidgetComp->SetWidgetClass(UGliderFuelWidget::StaticClass());

	// --- Camera Setup (top-down isometric) ---
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 2000.0f;
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// --- Character Movement (top-down) ---
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->bOrientRotationToMovement = false;    // Do NOT face movement direction
	CMC->bUseControllerDesiredRotation = true;  // Face cursor via controller yaw
	CMC->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // Fast snap to cursor
	CMC->MaxWalkSpeed = 600.0f;
	CMC->BrakingDecelerationWalking = 2000.0f;

	// Character faces controller yaw (set by PlayerController from cursor)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Networking
	bReplicates = true;
	SetReplicatingMovement(true);
}

void AHunterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHunterCharacter, ReplicatedAimLocation);
	DOREPLIFETIME(AHunterCharacter, ReplicatedHunterMesh);
	DOREPLIFETIME(AHunterCharacter, ReplicatedHunterAnimInstanceClass);
	DOREPLIFETIME(AHunterCharacter, ReplicatedHunterMeshRelativeTransform);
	DOREPLIFETIME(AHunterCharacter, ReplicatedHunterVisualSet);
}

UAbilitySystemComponent* AHunterCharacter::GetAbilitySystemComponent() const
{
	const ABreachbornePlayerState* BBPS = GetPlayerState<ABreachbornePlayerState>();
	if (BBPS)
	{
		return BBPS->GetAbilitySystemComponent();
	}
	return nullptr;
}

UBBCharacterMovementComponent* AHunterCharacter::GetBBMovementComponent() const
{
	return Cast<UBBCharacterMovementComponent>(GetCharacterMovement());
}

void AHunterCharacter::SetAerialStrikeViewActive(bool bActive, float TargetArmLength)
{
	if (!CameraBoom)
	{
		return;
	}

	TargetCameraBoomArmLength = bActive ? FMath::Max(DefaultCameraBoomArmLength, TargetArmLength) : DefaultCameraBoomArmLength;
}

void AHunterCharacter::ApplyHunterDefinitionVisuals(const UBBHunterDefinition* HunterDefinition)
{
	if (!HunterDefinition || !GetMesh())
	{
		return;
	}

	if (HasAuthority())
	{
		ReplicatedHunterVisualSet = HunterDefinition->VisualSet;
		ReplicatedHunterMesh = HunterDefinition->VisualSet && HunterDefinition->VisualSet->SkeletalMesh
			? HunterDefinition->VisualSet->SkeletalMesh
			: HunterDefinition->SkeletalMesh;
		ReplicatedHunterAnimInstanceClass = HunterDefinition->VisualSet && HunterDefinition->VisualSet->AnimInstanceClass
			? HunterDefinition->VisualSet->AnimInstanceClass
			: HunterDefinition->AnimInstanceClass;
		ReplicatedHunterMeshRelativeTransform = HunterDefinition->VisualSet
			? HunterDefinition->VisualSet->MeshRelativeTransform
			: HunterDefinition->MeshRelativeTransform;
	}

	ApplyReplicatedHunterVisuals();
	ForceNetUpdate();
}

void AHunterCharacter::OnRep_HunterVisuals()
{
	ApplyReplicatedHunterVisuals();
}

void AHunterCharacter::ApplyReplicatedHunterVisuals()
{
	if (!GetMesh())
	{
		return;
	}

	if (ReplicatedHunterMesh)
	{
		GetMesh()->SetSkeletalMesh(ReplicatedHunterMesh);
		GetMesh()->SetRelativeTransform(ReplicatedHunterMeshRelativeTransform);
		GetMesh()->SetVisibility(true, true);

		if (PlaceholderMesh)
		{
			PlaceholderMesh->SetVisibility(false, true);
		}
	}
	else if (ReplicatedHunterVisualSet && ReplicatedHunterVisualSet->DebugFallbackMesh && PlaceholderMesh)
	{
		GetMesh()->SetVisibility(false, true);

		PlaceholderMesh->SetStaticMesh(ReplicatedHunterVisualSet->DebugFallbackMesh);
		PlaceholderMesh->SetRelativeTransform(ReplicatedHunterMeshRelativeTransform);
		PlaceholderMesh->SetVisibility(true, true);
	}

	if (ReplicatedHunterMesh && ReplicatedHunterAnimInstanceClass)
	{
		GetMesh()->SetAnimInstanceClass(ReplicatedHunterAnimInstanceClass);
	}
}

float AHunterCharacter::PlayAbilityMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float PlayRateOverride)
{
	const float Duration = PlayAbilityMontageLocal(AbilityOrInputTag, Phase, PlayRateOverride);
	if (HasAuthority())
	{
		Multicast_PlayAbilityMontage(AbilityOrInputTag, Phase, PlayRateOverride);
	}
	return Duration;
}

void AHunterCharacter::StopAbilityMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float BlendOutTime)
{
	StopAbilityMontageLocal(AbilityOrInputTag, Phase, BlendOutTime);
	if (HasAuthority())
	{
		Multicast_StopAbilityMontage(AbilityOrInputTag, Phase, BlendOutTime);
	}
}

void AHunterCharacter::Multicast_PlayAbilityMontage_Implementation(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float PlayRateOverride)
{
	if (HasAuthority())
	{
		return;
	}

	PlayAbilityMontageLocal(AbilityOrInputTag, Phase, PlayRateOverride);
}

void AHunterCharacter::Multicast_StopAbilityMontage_Implementation(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float BlendOutTime)
{
	if (HasAuthority())
	{
		return;
	}

	StopAbilityMontageLocal(AbilityOrInputTag, Phase, BlendOutTime);
}

float AHunterCharacter::PlayAbilityMontageLocal(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float PlayRateOverride)
{
	if (!AbilityOrInputTag.IsValid() || !ReplicatedHunterVisualSet || !GetMesh())
	{
		return 0.0f;
	}

	float ConfiguredPlayRate = 1.0f;
	bool bConfiguredLooping = false;
	UAnimMontage* Montage = ReplicatedHunterVisualSet->FindMontage(AbilityOrInputTag, Phase, ConfiguredPlayRate, bConfiguredLooping);
	if (!Montage)
	{
		return 0.0f;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const bool bAvoidRestart = bConfiguredLooping || Phase == EBBAbilityAnimationPhase::Loop || AnimInstance->Montage_IsPlaying(Montage);
		if (bAvoidRestart && AnimInstance->Montage_IsPlaying(Montage))
		{
			return Montage->GetPlayLength();
		}
	}

	const float FinalPlayRate = PlayRateOverride > 0.0f ? PlayRateOverride : ConfiguredPlayRate;
	return PlayAnimMontage(Montage, FMath::Max(0.01f, FinalPlayRate));
}

void AHunterCharacter::StopAbilityMontageLocal(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float BlendOutTime)
{
	if (!AbilityOrInputTag.IsValid() || !ReplicatedHunterVisualSet || !GetMesh())
	{
		return;
	}

	float ConfiguredPlayRate = 1.0f;
	bool bConfiguredLooping = false;
	UAnimMontage* Montage = ReplicatedHunterVisualSet->FindMontage(AbilityOrInputTag, Phase, ConfiguredPlayRate, bConfiguredLooping);
	if (!Montage)
	{
		return;
	}
	const bool bIsLoopPhase = bConfiguredLooping || Phase == EBBAbilityAnimationPhase::Loop;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (bIsLoopPhase && !AnimInstance->Montage_IsPlaying(Montage))
		{
			return;
		}
		AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), Montage);
	}
}

void AHunterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CameraBoom)
	{
		return;
	}

	if (!FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetCameraBoomArmLength, 1.0f))
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetCameraBoomArmLength, DeltaSeconds, CameraBoomZoomInterpSpeed);
	}
	else
	{
		CameraBoom->TargetArmLength = TargetCameraBoomArmLength;
	}
}

void AHunterCharacter::ServerSetAimLocation_Implementation(FVector NewAimLocation)
{
	ReplicatedAimLocation = NewAimLocation;
}

void AHunterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (CameraBoom)
	{
		DefaultCameraBoomArmLength = CameraBoom->TargetArmLength;
		TargetCameraBoomArmLength = DefaultCameraBoomArmLength;
	}

	if (GliderFuelWidgetComp)
	{
		GliderFuelWidgetComp->InitWidget();
		if (UGliderFuelWidget* FuelWidget = Cast<UGliderFuelWidget>(GliderFuelWidgetComp->GetUserWidgetObject()))
		{
			FuelWidget->SetTargetHunter(this);
		}
	}

	// Backup binding path — on clients, OnRep_PlayerState may not have fired yet.
	// If this also fails (PlayerState still null), BindASCFromPlayerState starts a retry timer.
	BindASCFromPlayerState();
}

void AHunterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server-side: bind ASC from the new controller's PlayerState
	BindASCFromPlayerState();
}

void AHunterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client-side: bind ASC when PlayerState replicates
	BindASCFromPlayerState();
}

void AHunterCharacter::BindASCFromPlayerState()
{
	ABreachbornePlayerState* BBPS = GetPlayerState<ABreachbornePlayerState>();
	if (!BBPS)
	{
		// On clients, PlayerState may not have replicated yet.
		// Start a retry timer so we bind as soon as it arrives.
		if (!HasAuthority() && GetWorld())
		{
			if (!GetWorld()->GetTimerManager().IsTimerActive(ASCBindRetryHandle))
			{
				UE_LOG(LogBreachborne, Log, TEXT("HunterCharacter::BindASC [%s]: PlayerState is NULL on client — starting retry timer"), *GetName());
				GetWorld()->GetTimerManager().SetTimer(
					ASCBindRetryHandle,
					this,
					&AHunterCharacter::BindASCFromPlayerState,
					0.1f,
					true
				);
			}
		}
		return;
	}

	// PlayerState found — clear any retry timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ASCBindRetryHandle);
	}

	UBBAbilitySystemComponent* ASC = BBPS->GetBBAbilitySystemComponent();
	if (ASC)
	{
		ASC->InitAbilityActorInfo(BBPS, this);
		UE_LOG(LogBreachborne, Log, TEXT("HunterCharacter: ASC bound — Owner=%s Avatar=%s (%s)"),
			*BBPS->GetPlayerName(), *GetName(), HasAuthority() ? TEXT("Server") : TEXT("Client"));
	}
	else
	{
		UE_LOG(LogBreachborne, Warning, TEXT("HunterCharacter::BindASC [%s]: PlayerState found but ASC is NULL"), *GetName());
	}
}

void AHunterCharacter::Multicast_InvokeLocalGameplayCueBatch_Implementation(
	const TArray<FBBLocalGameplayCue>& Cues, bool bSkipFirstCueForRemoteOwner)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || Cues.IsEmpty())
	{
		return;
	}

	const int32 FirstCueIndex = bSkipFirstCueForRemoteOwner && IsLocallyControlled() && !HasAuthority() ? 1 : 0;
	for (int32 Index = FirstCueIndex; Index < Cues.Num(); ++Index)
	{
		const FBBLocalGameplayCue& Cue = Cues[Index];
		if (!Cue.CueTag.IsValid())
		{
			continue;
		}

		FGameplayCueParameters Params;
		Params.Instigator = this;
		Params.EffectCauser = this;
		Params.SourceObject = this;
		Params.Location = Cue.Location;
		Params.Normal = Cue.Normal;
		ASC->InvokeGameplayCueEvent(Cue.CueTag, EGameplayCueEvent::Executed, Params);
		SpawnHudsonLocalCueFallback(this, Cue);
	}
}

void AHunterCharacter::PlayLocalPrimitiveCueFallback(const FBBLocalGameplayCue& Cue)
{
	SpawnHudsonLocalCueFallback(this, Cue);
}

void AHunterCharacter::BeginHookPull(AActor* Target, float PullSpeed, float ImpactDistance, float MaxPullDuration, float PullTickInterval, UClass* DamageEffectClass, float Damage, const FGameplayEffectContextHandle& Context)
{
	if (!HasAuthority() || !Target || Target == this)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(HookPullTimerHandle);

	HookPullTargetActor = Target;
	HookPullDamageEffectClass = DamageEffectClass;
	HookPullEffectContext = Context;
	HookPullElapsedSeconds = 0.0f;
	HookPullSpeed = FMath::Max(100.0f, PullSpeed);
	HookPullImpactDistance = FMath::Max(50.0f, ImpactDistance);
	HookPullMaxDuration = FMath::Max(0.1f, MaxPullDuration);
	HookPullTickInterval = FMath::Max(0.01f, PullTickInterval);
	HookPullDamage = FMath::Max(0.0f, Damage);

	const float CurrentDistance = FVector::Dist2D(Target->GetActorLocation(), GetActorLocation());
	if (CurrentDistance <= HookPullImpactDistance)
	{
		EndHookPull(true);
		return;
	}

	PlayAbilityMontage(BBGameplayTags::Ability_Hunter_Kingpin_RMB, EBBAbilityAnimationPhase::Loop);
	GetWorldTimerManager().SetTimer(HookPullTimerHandle, this, &AHunterCharacter::HookPullTick, HookPullTickInterval, true, 0.0f);
	UE_LOG(LogBreachborne, Log, TEXT("KingpinHook: %s started pulling %s"),
		*GetName(), *Target->GetName());
}

void AHunterCharacter::HookPullTick()
{
	if (!HasAuthority())
	{
		EndHookPull(false);
		return;
	}

	AActor* Target = HookPullTargetActor.Get();
	if (!Target)
	{
		EndHookPull(false);
		return;
	}

	HookPullElapsedSeconds += HookPullTickInterval;
	if (HookPullElapsedSeconds >= HookPullMaxDuration)
	{
		EndHookPull(false);
		return;
	}

	const FVector TargetLocation = Target->GetActorLocation();
	const FVector ToKingpin = GetActorLocation() - TargetLocation;
	const float Distance2D = ToKingpin.Size2D();
	if (Distance2D <= HookPullImpactDistance)
	{
		EndHookPull(true);
		return;
	}

	FVector PullDirection = ToKingpin.GetSafeNormal2D();
	if (PullDirection.IsNearlyZero())
	{
		PullDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector PullVelocity = PullDirection * HookPullSpeed;
	if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		if (UCharacterMovementComponent* TargetMoveComp = TargetCharacter->GetCharacterMovement())
		{
			TargetMoveComp->Velocity = PullVelocity;
			return;
		}
	}

	FHitResult SweepHit;
	Target->SetActorLocation(TargetLocation + PullVelocity * HookPullTickInterval, true, &SweepHit);
}

void AHunterCharacter::EndHookPull(bool bApplyImpactDamage)
{
	GetWorldTimerManager().ClearTimer(HookPullTimerHandle);

	StopAbilityMontage(BBGameplayTags::Ability_Hunter_Kingpin_RMB, EBBAbilityAnimationPhase::Loop);
	if (bApplyImpactDamage)
	{
		PlayAbilityMontage(BBGameplayTags::Ability_Hunter_Kingpin_RMB, EBBAbilityAnimationPhase::Impact);
		ApplyHookPullImpactDamage();
	}

	HookPullTargetActor.Reset();
	HookPullDamageEffectClass = nullptr;
	HookPullEffectContext = FGameplayEffectContextHandle();
	HookPullElapsedSeconds = 0.0f;
	HookPullDamage = 0.0f;
}

void AHunterCharacter::ApplyHookPullImpactDamage()
{
	AActor* Target = HookPullTargetActor.Get();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = Target ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) : nullptr;
	if (!SourceASC || !TargetASC || !HookPullDamageEffectClass || HookPullDamage <= 0.0f)
	{
		return;
	}

	FGameplayEffectContextHandle Context = HookPullEffectContext.IsValid()
		? HookPullEffectContext
		: SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const TSubclassOf<UGameplayEffect> DamageClass(HookPullDamageEffectClass.Get());
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageClass, 1.0f, Context);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, HookPullDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void AHunterCharacter::BeginGrapplePull(AActor* TargetActor, const FVector& TargetLocation, float InitialSpeed, float MaxSpeed, float StopDistance, float MaxPullDuration, float PullTickInterval)
{
	if (!HasAuthority())
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(GrapplePullTimerHandle);

	GrappleTargetActor = TargetActor;
	GrappleTargetLocation = TargetActor ? TargetActor->GetActorLocation() : TargetLocation;
	GrappleInitialDistance = FVector::Dist(GetActorLocation(), GrappleTargetLocation);
	GrappleElapsedSeconds = 0.0f;
	GrappleInitialSpeed = FMath::Max(100.0f, InitialSpeed);
	GrappleMaxSpeed = FMath::Max(GrappleInitialSpeed, MaxSpeed);
	GrappleStopDistance = FMath::Max(50.0f, StopDistance);
	GrappleMaxDuration = FMath::Max(0.2f, MaxPullDuration);
	GrappleTickInterval = FMath::Max(0.01f, PullTickInterval);

	if (GrappleInitialDistance <= GrappleStopDistance)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(GrapplePullTimerHandle, this, &AHunterCharacter::GrapplePullTick, GrappleTickInterval, true, 0.0f);
	UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: %s started GrapplingHook pull to %s"),
		*GetName(), TargetActor ? *TargetActor->GetName() : *GrappleTargetLocation.ToCompactString());
}

void AHunterCharacter::GrapplePullTick()
{
	if (!HasAuthority())
	{
		EndGrapplePull();
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		EndGrapplePull();
		return;
	}

	GrappleElapsedSeconds += GrappleTickInterval;
	if (GrappleElapsedSeconds >= GrappleMaxDuration)
	{
		EndGrapplePull();
		return;
	}

	if (GrappleTargetActor.IsValid())
	{
		GrappleTargetLocation = GrappleTargetActor->GetActorLocation();
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector ToTarget = GrappleTargetLocation - CurrentLocation;
	const float Distance = ToTarget.Size();
	if (Distance <= GrappleStopDistance)
	{
		EndGrapplePull();
		return;
	}

	const float DistanceAlpha = GrappleInitialDistance > KINDA_SMALL_NUMBER
		? 1.0f - FMath::Clamp(Distance / GrappleInitialDistance, 0.0f, 1.0f)
		: 1.0f;
	const float SpeedAlpha = FMath::InterpEaseIn(0.0f, 1.0f, DistanceAlpha, 1.75f);
	const float Speed = FMath::Lerp(GrappleInitialSpeed, GrappleMaxSpeed, SpeedAlpha);

	MoveComp->Velocity = ToTarget.GetSafeNormal() * Speed;
}

void AHunterCharacter::EndGrapplePull()
{
	GetWorldTimerManager().ClearTimer(GrapplePullTimerHandle);
	GrappleTargetActor.Reset();
	GrappleInitialDistance = 0.0f;
	GrappleElapsedSeconds = 0.0f;
}

void AHunterCharacter::ResetTransientCombatState()
{
	if (!HasAuthority())
	{
		return;
	}

	EndHookPull(false);
	EndGrapplePull();

	if (GliderComponent)
	{
		GliderComponent->CancelForMantle();
	}
	if (MantleComponent)
	{
		MantleComponent->NotifyMantleFinished();
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance())
		{
			AnimInstance->StopAllMontages(0.1f);
		}
	}
}
