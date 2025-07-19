// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeathSignature, AEnemyBase*, DeadEnemy);

UCLASS()
class COMBATSYSTEM_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Health System")
	float CurrentShieldPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Health System")
	float MaxShieldPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Health System")
	float CurrentHealthPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Health System")
	float MaxHealthPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Health System")
	float HealthRegenSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Combat")
	float Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Combat")
	USoundBase* FireSound;

	// Time in seconds after last hit before shield starts regenerating
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Health System")
	float ShieldRegenDelay = 5.0f;

	// Tracks time since last damage
	float LastDamageTime = 0.0f;

	// Called when enemy takes damage
	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual void ReceiveDamage(float Amount);

	// Shield regeneration logic
	void RegenerateShield(float DeltaTime);

	// Weapon fire logic
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void FireAtPlayer();

	// Cached player reference (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	APawn* PlayerPawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Enemy Weapon")
	TSubclassOf<AActor> WeaponBlueprint;

	UPROPERTY()
	AActor* SpawnedWeapon;

	UPROPERTY()
	USkeletalMeshComponent* SpawnedWeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Weapon")
	bool bIsEnemyAimingWeapon = false;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy Weapon")
	UParticleSystem* MuzzleFlashFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Weapon")
	float FireRate = 2.0f; // 2 shots per second

	FTimerHandle FireRateTimerHandle;

	UFUNCTION()
	void SetEnemyAiming();

	UPROPERTY(EditDefaultsOnly, Category = "Enemy Weapon")
	TSubclassOf<AActor> TracerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Health System")
	bool bIsEnemyDead = false;

	bool bEnemyDeathSequenceExecuted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* EnemyDyingSequence;

	FTimerHandle EnemyDeathTimerHandle;

	void CheckEnemyDeath();

	void DestroyEnemy();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyDeathSignature OnEnemyDeath;

};
