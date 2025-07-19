// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponManagerComponent.generated.h"

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single,
	Automatic
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFireMode FireMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AmmoPerMag = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentAmmo = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Magazines = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FireRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamagePerBullet = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TempAmmoPool = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USoundBase* FireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AWeaponActor> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MuzzleSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> BulletTracerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWeaponHitDistance = 10000.f;

	UPROPERTY(Transient)
	AWeaponActor* SpawnedWeapon = nullptr;
};

USTRUCT(BlueprintType)
struct FWeaponAmmoData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentMagAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentTotalAmmo;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMBATSYSTEM_API UWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponManagerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWeaponData CurrentWeapon;

	FTimerHandle AutoFireTimerHandle;

	void StartFire();

	void StopFire();

	void Fire();

	bool CanFire() const;

	void Reload();

	AActor* WeaponOwner;

	void EquipWeapon(const FWeaponData& NewWeapon);

	UFUNCTION(BlueprintCallable)
	void SwitchWeapon(const FWeaponData& NewWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsWeaponHolstered = false;

	void ToggleHolsterWeapon();

	FName GetHolsterSocketName(const FName& WeaponName) const;

	FWeaponData LastEquippedWeaponData;

	void SpawnAndHolsterWeapon(const FWeaponData& WeaponData);

	UPROPERTY()
	TMap<FName, FWeaponData> AllWeapons;

	bool IsWeaponHolstered() const;

	bool bIsFiring = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Reload")
	bool bIsReloading = false;

	FTimerHandle ReloadTimerHandle;
	float ReloadDuration = 1.5f;

	void TriggerReload();

	void ExecuteReload();

	bool IsCurrentWeaponAmmoEmpty() const;

	UPROPERTY(BlueprintReadOnly, Category = "Player Weapon Info")
	FName CurrentWeaponName;

	UPROPERTY(BlueprintReadOnly, Category = "Player Weapon Info")
	int CurrentWeaponMagAmmo;

	UPROPERTY(BlueprintReadOnly, Category = "Player Weapon Info")
	int CurrentWeaponTotalAmmo;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FName GetCurrentWeaponName() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int GetCurrentMagAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int GetCurrentTotalAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool GetCurrentReloadMode() const;

	UPROPERTY()
	TMap<FName, FWeaponAmmoData> WeaponAmmoMap;

	void SaveCurrentWeaponAmmo(FName WeaponName, int32 MagAmmo, int32 TotalAmmo);
};
