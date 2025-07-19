#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WeaponManagerComponent.h"
#include "PlayerCharacterController.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AActor;
class ALevelManager;

UCLASS()
class COMBATSYSTEM_API APlayerCharacterController : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacterController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void HandleJump();

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void StartRunning();

	void StopRunning();

	void CheckForWallRun();

	void StartWallRun(bool bIsRightWall, const FHitResult& WallHit);

	void StopWallRun();

	bool CanWallRunOnSurface(FHitResult& WallHit);

	void ResetWallRunCooldown();

	void WallJump();

	float TargetArmLengthWalking = 150.0f;
	float TargetArmLengthRunning = 100.0f;
	float SpringArmInterpSpeed = 10.0f;

	FVector2D MovementVector; // to store last move direction

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* PlayerInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RunAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchToPrimary1Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchToPrimary2Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchToSecondaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleWeaponHolsterAction;

	UPROPERTY(EditDefaultsOnly, Category = Movement)
	float WalkSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = Movement)
	float RunSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
	float WallRunDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
	float WallRunSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
	float WallDetectionDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
	float WallRunGravityScale = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wall Running")
	bool bIsWallRunning = false;

	FTimerHandle WallRunTimerHandle;
	bool bWallRunCooldown = false;
	FTimerHandle WallRunCooldownHandle;
	float WallRunCooldownDuration = 0.5f;

	FVector LastWallNormal = FVector::ZeroVector;
	float WallRunMinAngleDifference = 30.f; // Degrees
	float WallRunMinDistance = 1000.f; // Distance to re-trigger on same wall
	FVector LastWallRunLocation = FVector::ZeroVector;

	// Store the last wall hit actor
	AActor* LastWallActor = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wall Running")
	bool bIsLeftWallRun = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wall Running")
	bool bIsRightWallRun = false;

	//variables related to Weapon Manager and Weapons....

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UWeaponManagerComponent* WeaponManager;

	void StartFiring();

	void StopFiring();

	void Reloading();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FWeaponData AssaultRifleData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FWeaponData ShotgunData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FWeaponData PistolData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	FName CurrentWeaponSlot;

	void StartSwitchToPrimary1();

	void StartSwitchToPrimary2();

	void StartSwitchToSecondary();

	void StartHolsterWeapon();

	void StartAiming();

	void StopAiming();

	void StopSwitchToPrimary1();

	void StopSwitchToPrimary2();

	void StopSwitchToSecondary();

	void StopHolsterWeapon();

	//variables for animations

	bool bIsFiring = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	bool bIsWeaponEquipped = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	bool bIsPrimaryWeapon = true;

	bool bStoreIsPrimaryWeaponBeforeHolster = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	bool bIsWeaponHolstering = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	bool bIsWeaponBeingPutAway = false;

	FTimerHandle WeaponSwitchTimerHandle;
	float WeaponHolsterDelayTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* HolsterAnimSequence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* WeaponPuttingAwayAnimSequence;

	bool bWasCharacterPreviouslyRunningBeforeAiming = false;

	bool bCanFire = true;

	bool bIsRunActionPressed = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Health System")
	float CurrentShieldPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Health System")
	float MaxShieldPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Health System")
	float CurrentHealthPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Health System")
	float MaxHealthPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Health System")
	float HealthRegenSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Health System")
	float ShieldRegenDelay = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Health System")
	float HealthRegenDelay = 2.0f;

	// Tracks time since last damage
	float LastDamageTime = 0.0f;

	// Called when enemy takes damage
	UFUNCTION(BlueprintCallable, Category = "Player Health System")
	virtual void ReceiveDamage(float Amount);

	// Shield regeneration logic
	void RegenerateShieldAndHealth(float DeltaTime);

	UPROPERTY()
	ALevelManager* LevelManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Enemy Tracker")
	int DeadEnemies = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Enemy Tracker")
	int TotalEnemies = 0;

	void EnemyTracker();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Death")
	bool bIsPlayerDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Death")
	bool bIsPlayerDeadExecuted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Death")
	UAnimSequence* PlayerDeathAnimSequence;

	FTimerHandle PlayerDeathTimerHandle;

	void CheckPlayerHealth();

	void ConfirmPlayerDeath();


public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

};