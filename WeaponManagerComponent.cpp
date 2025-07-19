// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponManagerComponent.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "EnemyBase.h"
#include "WeaponActor.h" 

// Sets default values for this component's properties
UWeaponManagerComponent::UWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	WeaponOwner = GetOwner();

	// Holster all available weapons at startup (except the default equipped one)
	for (const auto& Pair : AllWeapons)
	{
		if (Pair.Key != CurrentWeapon.WeaponName)
		{
			SpawnAndHolsterWeapon(Pair.Value);
		}
	}

	// Equip initial weapon (like AssaultRifle)
	EquipWeapon(CurrentWeapon);
}

void UWeaponManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CurrentWeaponName = CurrentWeapon.WeaponName;
	CurrentWeaponMagAmmo = CurrentWeapon.CurrentAmmo;
	CurrentWeaponTotalAmmo = (CurrentWeapon.Magazines * CurrentWeapon.AmmoPerMag) + CurrentWeapon.TempAmmoPool;

	if (CurrentWeaponName == "None")
	{
		CurrentWeaponMagAmmo = 0;
		CurrentWeaponTotalAmmo = 0;
	}

	GetCurrentReloadMode();
}

void UWeaponManagerComponent::StartFire()
{
	if (bIsFiring || !CanFire() || bIsReloading) return;

	bIsFiring = true;

	if (CurrentWeapon.FireMode == EFireMode::Single)
	{
		Fire();
		StopFire(); // For single fire, immediately stop after one shot
	}
	else if (CurrentWeapon.FireMode == EFireMode::Automatic)
	{
		Fire(); //Fire immediately
		float FireInterval = 1.0f / CurrentWeapon.FireRate;

		GetWorld()->GetTimerManager().SetTimer(
			AutoFireTimerHandle,
			this,
			&UWeaponManagerComponent::Fire,
			FireInterval,
			true,
			FireInterval //Delay for the *next* shot only
		);
	}
}


void UWeaponManagerComponent::StopFire()
{
	bIsFiring = false;
	GetWorld()->GetTimerManager().ClearTimer(AutoFireTimerHandle);
}

void UWeaponManagerComponent::Fire()
{
	if (!CanFire()) return;

	UE_LOG(LogTemp, Log, TEXT("Firing %s"), *CurrentWeapon.WeaponName.ToString());

	CurrentWeapon.CurrentAmmo--;

	if (CurrentWeapon.CurrentAmmo <= 0)
	{
		StopFire(); // Stop the auto-fire loop
		TriggerReload(); // Start reloading immediately
		return;
	}

	// Muzzle flash
	if (CurrentWeapon.SpawnedWeapon && CurrentWeapon.MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAttached(
			CurrentWeapon.MuzzleFlash,
			CurrentWeapon.SpawnedWeapon->WeaponMesh,
			CurrentWeapon.MuzzleSocketName
		);

		if (CurrentWeapon.FireSound)
		{
			UGameplayStatics::SpawnSoundAttached(
				CurrentWeapon.FireSound,
				CurrentWeapon.SpawnedWeapon->WeaponMesh,
				CurrentWeapon.MuzzleSocketName
			);
		}
	}

	// Ensure SpawnedWeapon is valid before accessing it
	if (!CurrentWeapon.SpawnedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapon not spawned, aborting fire."));
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;

	APawn* PawnOwner = Cast<APawn>(WeaponOwner);
	APlayerController* PC = nullptr;

	if (PawnOwner)
	{
		PC = Cast<APlayerController>(PawnOwner->GetController());
		if (PC)
		{
			PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
		}
	}

	if (PC)
	{
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
		FVector2D CrosshairScreenPosition(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);

		FVector CrosshairWorldLocation, CrosshairWorldDirection;
		if (PC->DeprojectScreenPositionToWorld(CrosshairScreenPosition.X, CrosshairScreenPosition.Y, CrosshairWorldLocation, CrosshairWorldDirection))
		{
			FVector Start = CameraLocation;
			FVector End = CameraLocation + (CrosshairWorldDirection * CurrentWeapon.MaxWeaponHitDistance);

			FCollisionQueryParams Params;
			Params.AddIgnoredActor(WeaponOwner);

			FHitResult Hit;
			bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

			// Spawn bullet tracer
			if (CurrentWeapon.BulletTracerClass && CurrentWeapon.SpawnedWeapon)
			{
				FVector MuzzleLocation = CurrentWeapon.SpawnedWeapon->WeaponMesh->GetSocketLocation(CurrentWeapon.MuzzleSocketName);
				FVector TargetPoint = bHit ? Hit.ImpactPoint : End;
				FVector Direction = (TargetPoint - MuzzleLocation).GetSafeNormal();
				FRotator TracerRotation = Direction.Rotation();

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AActor* Tracer = GetWorld()->SpawnActor<AActor>(CurrentWeapon.BulletTracerClass, MuzzleLocation, TracerRotation, SpawnParams);
			}

			if (bHit)
			{
				AActor* HitActor = Hit.GetActor();
				bool bIsEnemy = false;

				if (HitActor)
				{
					if (HitActor->ActorHasTag("EnemyTier1"))
					{
						if (AEnemyBase* Enemy = Cast<AEnemyBase>(HitActor))
						{
							Enemy->ReceiveDamage(CurrentWeapon.DamagePerBullet);
							bIsEnemy = true;
						}
					}

					if (HitActor->ActorHasTag("EnemyTier2"))
					{
						if (AEnemyBase* Enemy = Cast<AEnemyBase>(HitActor))
						{
							Enemy->ReceiveDamage(CurrentWeapon.DamagePerBullet);
							bIsEnemy = true;
						}
					}

					if (HitActor->ActorHasTag("EnemyTier3"))
					{
						if (AEnemyBase* Enemy = Cast<AEnemyBase>(HitActor))
						{
							Enemy->ReceiveDamage(CurrentWeapon.DamagePerBullet);
							bIsEnemy = true;
						}
					}
				}

				// Impact Effect
				if (!bIsEnemy && CurrentWeapon.ImpactEffect)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						CurrentWeapon.ImpactEffect,
						Hit.ImpactPoint,
						Hit.ImpactNormal.Rotation()
					);
				}
			}
		}
	}
}

bool UWeaponManagerComponent::CanFire() const
{
	return !bIsReloading && CurrentWeapon.CurrentAmmo > 0;
}

void UWeaponManagerComponent::TriggerReload()
{
	if (bIsReloading) return;

	bIsReloading = true;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UWeaponManagerComponent::ExecuteReload, ReloadDuration, false);
}

void UWeaponManagerComponent::ExecuteReload()
{
	bIsReloading = false;
	Reload();
}

bool UWeaponManagerComponent::IsCurrentWeaponAmmoEmpty() const
{
	return CurrentWeapon.CurrentAmmo <= 0;
}

FName UWeaponManagerComponent::GetCurrentWeaponName() const
{
	return CurrentWeaponName;
}

int UWeaponManagerComponent::GetCurrentMagAmmo() const
{
	return CurrentWeaponMagAmmo;
}

int UWeaponManagerComponent::GetCurrentTotalAmmo() const
{
	return CurrentWeaponTotalAmmo;
}

bool UWeaponManagerComponent::GetCurrentReloadMode() const
{
	return bIsReloading;
}

void UWeaponManagerComponent::SaveCurrentWeaponAmmo(FName WeaponName, int32 MagAmmo, int32 TotalAmmo)
{
	WeaponAmmoMap.FindOrAdd(WeaponName) = { MagAmmo, TotalAmmo };
}

void UWeaponManagerComponent::Reload()
{
	if (CurrentWeapon.CurrentAmmo == CurrentWeapon.AmmoPerMag)
		return;

	// Cannot reload if no mags and no temp ammo
	if (CurrentWeapon.Magazines <= 0 && CurrentWeapon.TempAmmoPool <= 0)
		return;

	int32 NeededAmmo = CurrentWeapon.AmmoPerMag - CurrentWeapon.CurrentAmmo;
	int32 AmmoToSave = CurrentWeapon.CurrentAmmo;

	// Always refill to full if we have enough
	if (CurrentWeapon.Magazines > 0)
	{
		CurrentWeapon.Magazines--;
		CurrentWeapon.CurrentAmmo = CurrentWeapon.AmmoPerMag;

		// Save unused bullets
		if (AmmoToSave > 0)
		{
			int32 TotalAmmo = AmmoToSave + (CurrentWeapon.Magazines * CurrentWeapon.AmmoPerMag);

			// How many mags can we reform?
			int32 ReformedMags = TotalAmmo / CurrentWeapon.AmmoPerMag;
			int32 RemainingAmmo = TotalAmmo % CurrentWeapon.AmmoPerMag;

			CurrentWeapon.Magazines = ReformedMags;

			// If remaining bullets can't make a mag, store as temp
			if (RemainingAmmo > 0)
			{
				CurrentWeapon.TempAmmoPool += RemainingAmmo;
			}
		}
	}
	else if (CurrentWeapon.TempAmmoPool > 0)
	{
		// Last resort: use temp ammo to refill
		int32 LoadAmount = FMath::Min(CurrentWeapon.AmmoPerMag, CurrentWeapon.TempAmmoPool);
		CurrentWeapon.CurrentAmmo = LoadAmount;
		CurrentWeapon.TempAmmoPool -= LoadAmount;
	}
}

void UWeaponManagerComponent::EquipWeapon(const FWeaponData& NewWeapon)
{
	if (bIsWeaponHolstered)
	{
		return;
	}

	StopFire();

	if (CurrentWeapon.WeaponName != "None")
	{
		int32 TotalAmmo = (CurrentWeapon.Magazines * CurrentWeapon.AmmoPerMag) + CurrentWeapon.TempAmmoPool;
		SaveCurrentWeaponAmmo(CurrentWeapon.WeaponName, CurrentWeapon.CurrentAmmo, TotalAmmo);
	}

	if (CurrentWeapon.SpawnedWeapon)
	{
		FName HolsterSocket = GetHolsterSocketName(CurrentWeapon.WeaponName);
		if (USkeletalMeshComponent* OwnerMesh = WeaponOwner->FindComponentByClass<USkeletalMeshComponent>())
		{
			CurrentWeapon.SpawnedWeapon->AttachToComponent(
				OwnerMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				HolsterSocket
			);
			//CurrentWeapon.SpawnedWeapon->SetActorHiddenInGame(true);
			CurrentWeapon.SpawnedWeapon->SetActorEnableCollision(false);
		}
	}

	CurrentWeapon = NewWeapon;

	AWeaponActor* Spawned = nullptr;
	if (AllWeapons.Contains(NewWeapon.WeaponName))
	{
		Spawned = AllWeapons[NewWeapon.WeaponName].SpawnedWeapon;
	}
	else
	{
		if (!NewWeapon.WeaponClass) return;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = WeaponOwner;

		Spawned = GetWorld()->SpawnActor<AWeaponActor>(NewWeapon.WeaponClass, SpawnParams);
		if (Spawned)
		{
			FWeaponData StoredData = NewWeapon;
			StoredData.SpawnedWeapon = Spawned;
			AllWeapons.Add(NewWeapon.WeaponName, StoredData);
		}
	}

	if (Spawned)
	{
		CurrentWeapon.SpawnedWeapon = Spawned;

		if (FWeaponAmmoData* SavedAmmo = WeaponAmmoMap.Find(CurrentWeapon.WeaponName))
		{
			CurrentWeapon.CurrentAmmo = SavedAmmo->CurrentMagAmmo;

			// Calculate mags and temp pool from total
			int32 Total = SavedAmmo->CurrentTotalAmmo;
			CurrentWeapon.Magazines = Total / CurrentWeapon.AmmoPerMag;
			CurrentWeapon.TempAmmoPool = Total % CurrentWeapon.AmmoPerMag;
		}

		if (USkeletalMeshComponent* OwnerMesh = WeaponOwner->FindComponentByClass<USkeletalMeshComponent>())
		{
			FName EquipSocket = (CurrentWeapon.WeaponName == "Pistol") ? FName("SecondaryWeaponHolder") : FName("PrimaryWeaponHolder");

			Spawned->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, EquipSocket);
		}

		//Spawned->SetActorHiddenInGame(false);
		Spawned->SetActorEnableCollision(true);
		LastEquippedWeaponData = CurrentWeapon;
	}
}


void UWeaponManagerComponent::SwitchWeapon(const FWeaponData& NewWeapon)
{
	EquipWeapon(NewWeapon);
}

void UWeaponManagerComponent::ToggleHolsterWeapon()
{
	if (!WeaponOwner) return;

	if (bIsWeaponHolstered)
	{
		// Unholster and re-equip the last weapon
		if (LastEquippedWeaponData.SpawnedWeapon)
		{
			// Simply attach to equip socket (WeaponHolder)
			if (USkeletalMeshComponent* OwnerMesh = WeaponOwner->FindComponentByClass<USkeletalMeshComponent>())
			{
				FName EquipSocket = (LastEquippedWeaponData.WeaponName == "Pistol") ? FName("SecondaryWeaponHolder") : FName("PrimaryWeaponHolder");

				LastEquippedWeaponData.SpawnedWeapon->AttachToComponent(
					OwnerMesh,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale,
					EquipSocket
				);
				LastEquippedWeaponData.SpawnedWeapon->SetActorEnableCollision(true);
			}

			CurrentWeapon = LastEquippedWeaponData;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No weapon to unholster."));
		}
	}
	else
	{
		if (!CurrentWeapon.SpawnedWeapon) return;

		StopFire();

		// Save current before holstering
		LastEquippedWeaponData = CurrentWeapon;

		if (USkeletalMeshComponent* OwnerMesh = WeaponOwner->FindComponentByClass<USkeletalMeshComponent>())
		{
			FName HolsterSocket = GetHolsterSocketName(CurrentWeapon.WeaponName);
			CurrentWeapon.SpawnedWeapon->AttachToComponent(
				OwnerMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				HolsterSocket
			);
			CurrentWeapon.SpawnedWeapon->SetActorEnableCollision(false);
		}

		// Clear current weapon reference while holstered
		CurrentWeapon = FWeaponData();
	}

	bIsWeaponHolstered = !bIsWeaponHolstered;
}

FName UWeaponManagerComponent::GetHolsterSocketName(const FName& WeaponName) const
{
	if (WeaponName == "Assault Rifle")
	{
		return FName("AR_HolsterSocket");
	}
	else if (WeaponName == "Shotgun")
	{
		return FName("Shotgun_HolsterSocket");
	}
	else if (WeaponName == "Pistol")
	{
		return FName("Pistol_HolsterSocket");
	}
	return NAME_None;
}

void UWeaponManagerComponent::SpawnAndHolsterWeapon(const FWeaponData& WeaponData)
{
	if (!WeaponOwner || !WeaponData.WeaponClass) return;

	if (AllWeapons.Contains(WeaponData.WeaponName)) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = WeaponOwner;
	AWeaponActor* SpawnedWeapon = GetWorld()->SpawnActor<AWeaponActor>(WeaponData.WeaponClass, SpawnParams);
	if (!SpawnedWeapon) return;

	if (USkeletalMeshComponent* OwnerMesh = WeaponOwner->FindComponentByClass<USkeletalMeshComponent>())
	{
		FName HolsterSocket = GetHolsterSocketName(WeaponData.WeaponName);
		SpawnedWeapon->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HolsterSocket);
	}

	//SpawnedWeapon->SetActorHiddenInGame(true);
	SpawnedWeapon->SetActorEnableCollision(false);

	FWeaponData Copy = WeaponData;
	Copy.SpawnedWeapon = SpawnedWeapon;
	AllWeapons.Add(WeaponData.WeaponName, Copy);
}

bool UWeaponManagerComponent::IsWeaponHolstered() const
{
	return bIsWeaponHolstered;
}





