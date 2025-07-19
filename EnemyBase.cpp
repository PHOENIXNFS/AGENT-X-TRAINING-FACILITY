#include "EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/Actor.h"
#include "PlayerCharacterController.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (WeaponBlueprint)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		// Spawn the weapon actor
		SpawnedWeapon = GetWorld()->SpawnActor<AActor>(WeaponBlueprint, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (SpawnedWeapon)
		{
			// Attach weapon to character mesh socket
			SpawnedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("EnemyWeaponHolder"));

			// Cache weapon's mesh
			SpawnedWeaponMesh = SpawnedWeapon->FindComponentByClass<USkeletalMeshComponent>();
		}
	}
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RegenerateShield(DeltaTime);

	SetEnemyAiming();

	CheckEnemyDeath();
}

void AEnemyBase::ReceiveDamage(float Amount)
{
	if (CurrentShieldPool > 0.0f)
	{
		float ShieldDamage = FMath::Min(CurrentShieldPool, Amount);
		CurrentShieldPool -= ShieldDamage;
		Amount -= ShieldDamage;
	}

	if (Amount > 0.0f)
	{
		CurrentHealthPool = FMath::Clamp(CurrentHealthPool - Amount, 0.0f, MaxHealthPool);
	}

	LastDamageTime = GetWorld()->GetTimeSeconds();
}


void AEnemyBase::RegenerateShield(float DeltaTime)
{
	if (bIsEnemyDead) return;
	
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentShieldPool >= 0.0f && CurrentShieldPool < MaxShieldPool && HealthRegenSpeed > 0.0f)
	{
		if (CurrentTime - LastDamageTime >= ShieldRegenDelay)
		{
			CurrentShieldPool += HealthRegenSpeed * DeltaTime;
			CurrentShieldPool = FMath::Clamp(CurrentShieldPool, 0.0f, MaxShieldPool);
		}
	}
}


void AEnemyBase::FireAtPlayer()
{
	if (!PlayerPawn || !SpawnedWeaponMesh || !bIsEnemyAimingWeapon || bIsEnemyDead)
		return;

	FVector MuzzleLocation = SpawnedWeaponMesh->GetSocketLocation(TEXT("MuzzleFlash_AR"));
	FVector PlayerLocation = PlayerPawn->GetActorLocation();

	float Distance = FVector::Dist(MuzzleLocation, PlayerLocation);
	if (Distance > 5500.f)
		return;

	FVector Direction = (PlayerLocation - MuzzleLocation).GetSafeNormal();

	// --- Muzzle Flash FX ---
	if (MuzzleFlashFX)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleFlashFX,
			SpawnedWeaponMesh,
			TEXT("MuzzleFlash_AR"),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, MuzzleLocation);
	}

	// --- Line Trace to Determine Impact ---
	FHitResult Hit;
	FVector TraceEnd = MuzzleLocation + Direction * 10000.f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		MuzzleLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// --- Spawn Tracer FX ---
	if (TracerClass)
	{
		FRotator TracerRotation = (TraceEnd - MuzzleLocation).Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* Tracer = GetWorld()->SpawnActor<AActor>(
			TracerClass,
			MuzzleLocation,
			TracerRotation,
			SpawnParams
		);
	}

	if (bHit)
	{
		TraceEnd = Hit.ImpactPoint;
		AActor* HitActor = Hit.GetActor();

		if (HitActor && HitActor->ActorHasTag(FName("Player")))
		{
			// Cast directly to your character class (which inherits ACharacter)
			APlayerCharacterController* HitCharacter = Cast<APlayerCharacterController>(HitActor);
			if (HitCharacter)
			{
				UE_LOG(LogTemp, Warning, TEXT("Player Is Hit"));
				HitCharacter->ReceiveDamage(Damage);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Cast to APlayerCharacterController failed!"));
			}
		}
	}

}


void AEnemyBase::SetEnemyAiming()
{
	if (bIsEnemyDead) return;
	
	bool bIsAiming = bIsEnemyAimingWeapon;

	if (bIsAiming)
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(FireRateTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(
				FireRateTimerHandle,
				this,
				&AEnemyBase::FireAtPlayer,
				1.0f / FireRate,
				true
			);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(FireRateTimerHandle);
	}
}

void AEnemyBase::CheckEnemyDeath()
{
	if (CurrentShieldPool <= 0.0f && CurrentHealthPool <= 0.0f)
	{
		if (!bEnemyDeathSequenceExecuted)
		{
			bIsEnemyDead = true;
			bEnemyDeathSequenceExecuted = true;
			float EnemyDeathAnimationDuration = EnemyDyingSequence->GetPlayLength();

			GetWorldTimerManager().SetTimer(EnemyDeathTimerHandle, this, &AEnemyBase::DestroyEnemy, EnemyDeathAnimationDuration, false);
		}
	}
}

void AEnemyBase::DestroyEnemy()
{	
	OnEnemyDeath.Broadcast(this);
	
	SetActorTickEnabled(false);

	// Disable collision on the main actor and mesh
	SetActorEnableCollision(false);
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetSimulatePhysics(false); // if physics is on, disable it
	}

	if (SpawnedWeapon)
	{
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
		SpawnedWeaponMesh = nullptr;
	}

	if (FireRateTimerHandle.IsValid())
		GetWorldTimerManager().ClearTimer(FireRateTimerHandle);

	// Optionally hide the actor or play dissolve FX
	SetActorHiddenInGame(true);

	GetWorldTimerManager().ClearAllTimersForObject(this);


	// Destroy the actor after short delay (if animation is already handled before this call)
	Destroy();
}
