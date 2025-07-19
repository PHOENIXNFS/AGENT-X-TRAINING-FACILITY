#include "PlayerCharacterController.h"
#include "Engine/LocalPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/Actor.h"
#include "Animation/AnimSequenceBase.h"
#include "Camera/PlayerCameraManager.h"
#include "LevelManager.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APlayerCharacterController::APlayerCharacterController()
{
	// Initialize properties
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 150.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = false;  // DO NOT rotate boom with controller
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned
void APlayerCharacterController::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(PlayerInputMappingContext, 0);
		}
	}

	if (!WeaponManager)
	{
		WeaponManager = NewObject<UWeaponManagerComponent>(this, UWeaponManagerComponent::StaticClass());
		if (WeaponManager)
		{
			WeaponManager->RegisterComponent();
			WeaponManager->WeaponOwner = this;

			// Initialize all weapons
			WeaponManager->SpawnAndHolsterWeapon(AssaultRifleData);
			WeaponManager->SpawnAndHolsterWeapon(ShotgunData);
			WeaponManager->SpawnAndHolsterWeapon(PistolData);

			// Equip default weapon
			WeaponManager->EquipWeapon(AssaultRifleData);
			CurrentWeaponSlot = AssaultRifleData.WeaponName;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create WeaponManager component"));
		}
	}

	TArray<AActor*> FoundManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALevelManager::StaticClass(), FoundManagers);

	if (FoundManagers.Num() > 0)
	{
		LevelManager = Cast<ALevelManager>(FoundManagers[0]);
	}
}

// Called every frame
void APlayerCharacterController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsWallRunning && GetCharacterMovement()->IsFalling())
	{
		CheckForWallRun();
	}

	// Force walk speed in the air
	if (!GetCharacterMovement()->IsMovingOnGround() && GetCharacterMovement()->MaxWalkSpeed != WalkSpeed)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}

	// Lerp spring arm based on movement and speed
	bool bIsMovingForward = MovementVector.Y > 0.1f;
	float DesiredLength = bIsMovingForward && GetCharacterMovement()->MaxWalkSpeed > WalkSpeed
		? TargetArmLengthRunning
		: TargetArmLengthWalking;

	float CurrentLength = CameraBoom->TargetArmLength;
	CameraBoom->TargetArmLength = FMath::FInterpTo(CurrentLength, DesiredLength, DeltaTime, SpringArmInterpSpeed);

	RegenerateShieldAndHealth(DeltaTime);

	EnemyTracker();

	CheckPlayerHealth();

}

void APlayerCharacterController::HandleJump()
{
	if (bIsPlayerDeadExecuted) return;

	if (bIsWallRunning)
	{
		WallJump();
	}
	else
	{
		Jump();
	}
}

// Called to bind functionality to input
void APlayerCharacterController::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacterController::HandleJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Started, this, &APlayerCharacterController::StartRunning);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Completed, this, &APlayerCharacterController::StopRunning);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacterController::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacterController::Look);

		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerCharacterController::StartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &APlayerCharacterController::StopFiring);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &APlayerCharacterController::StartAiming);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &APlayerCharacterController::StopAiming);

		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &APlayerCharacterController::Reloading);

		EnhancedInputComponent->BindAction(SwitchToPrimary1Action, ETriggerEvent::Started, this, &APlayerCharacterController::StartSwitchToPrimary1);
		EnhancedInputComponent->BindAction(SwitchToPrimary2Action, ETriggerEvent::Started, this, &APlayerCharacterController::StartSwitchToPrimary2);
		EnhancedInputComponent->BindAction(SwitchToSecondaryAction, ETriggerEvent::Started, this, &APlayerCharacterController::StartSwitchToSecondary);

		EnhancedInputComponent->BindAction(ToggleWeaponHolsterAction, ETriggerEvent::Started, this, &APlayerCharacterController::StartHolsterWeapon);

	}
}

// Movement
void APlayerCharacterController::Move(const FInputActionValue& Value)
{
	if (bIsPlayerDeadExecuted) return;

	MovementVector = Value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, MovementVector.Y);
		AddMovementInput(Right, MovementVector.X);
	}
}

// Look
void APlayerCharacterController::Look(const FInputActionValue& Value)
{
	if (bIsPlayerDeadExecuted) return;
	
	const FVector2D LookAxis = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Add yaw (left/right) input normally without restriction
		AddControllerYawInput(LookAxis.X);

		// Apply pitch input normally (without clamping)
		AddControllerPitchInput(LookAxis.Y);
	}
}



// Running
void APlayerCharacterController::StartRunning()
{
	if (bIsPlayerDeadExecuted) return;
	
	if (bIsAiming) return;

	bIsRunActionPressed = true;
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void APlayerCharacterController::StopRunning()
{
	if (bIsPlayerDeadExecuted) return;
	
	bIsRunActionPressed = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacterController::CheckForWallRun()
{
	//if (bIsPlayerDeadExecuted) return;
	
	if (bIsWallRunning || bWallRunCooldown) return;

	FVector StartLocation = GetActorLocation();
	FVector RightWallDetectionDistance = StartLocation + GetActorRightVector() * WallDetectionDistance;
	FVector LeftWallDetectionDistance = StartLocation - GetActorRightVector() * WallDetectionDistance;

	FHitResult RightWallHit, LeftWallHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bIsRightWall = GetWorld()->LineTraceSingleByChannel(RightWallHit, StartLocation, RightWallDetectionDistance, ECC_Visibility, Params);
	bool bIsLeftWall = GetWorld()->LineTraceSingleByChannel(LeftWallHit, StartLocation, LeftWallDetectionDistance, ECC_Visibility, Params);

	if (bIsRightWall && CanWallRunOnSurface(RightWallHit))
	{
		StartWallRun(true, RightWallHit);
		bIsRightWallRun = true;
	}
	else if (bIsLeftWall && CanWallRunOnSurface(LeftWallHit))
	{
		StartWallRun(false, LeftWallHit);
		bIsLeftWallRun = true;
	}
}

void APlayerCharacterController::StartWallRun(bool bIsRightWall, const FHitResult& WallHit)
{
	//if (bIsPlayerDeadExecuted) return;
	
	bIsWallRunning = true;
	bWallRunCooldown = true;

	LastWallNormal = WallHit.ImpactNormal;
	LastWallRunLocation = WallHit.ImpactPoint;
	LastWallActor = WallHit.GetActor();  // Store the last wall actor

	GetCharacterMovement()->bOrientRotationToMovement = false;

	GetCharacterMovement()->GravityScale = WallRunGravityScale;
	GetCharacterMovement()->Velocity = FVector::ZeroVector;

	const FVector WallRunDirection = GetActorForwardVector() + FVector::UpVector * 0.2f;
	LaunchCharacter(WallRunDirection * WallRunSpeed, true, true);

	GetWorldTimerManager().SetTimer(WallRunTimerHandle, this, &APlayerCharacterController::StopWallRun, WallRunDuration, false);
}

void APlayerCharacterController::StopWallRun()
{
	//if (bIsPlayerDeadExecuted) return;
	
	bIsWallRunning = false;

	if (bIsLeftWallRun)
	{
		bIsLeftWallRun = false;
	}

	if (bIsRightWallRun)
	{
		bIsRightWallRun = false;
	}

	GetCharacterMovement()->GravityScale = 1.0f;

	bWallRunCooldown = true;

	GetWorldTimerManager().SetTimer(WallRunCooldownHandle, this, &APlayerCharacterController::ResetWallRunCooldown, WallRunCooldownDuration, false);
}

bool APlayerCharacterController::CanWallRunOnSurface(FHitResult& WallHit)
{	
	// Check if the hit actor is valid and has the tag "Wall"
	AActor* HitActor = WallHit.GetActor();
	if (!HitActor || !HitActor->ActorHasTag("Wall"))
	{
		return false;
	}

	float Angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(LastWallNormal, WallHit.ImpactNormal)));
	bool bDifferentWall = Angle > WallRunMinAngleDifference;

	// Check distance moved since last wall run
	float Distance = FVector::Dist(WallHit.ImpactPoint, LastWallRunLocation);
	bool bMovedAway = Distance > WallRunMinDistance;

	// Check if we are not wall running on the same wall unless we are far enough away and on the ground
	bool bOnGround = GetCharacterMovement()->IsMovingOnGround();
	bool bSameWall = (LastWallActor == HitActor && !bMovedAway && bOnGround);

	return bDifferentWall || bMovedAway || !bSameWall;
}

void APlayerCharacterController::ResetWallRunCooldown()
{
	//if (bIsPlayerDeadExecuted) return;
	
	bWallRunCooldown = false;
}

void APlayerCharacterController::WallJump()
{
	//if (bIsPlayerDeadExecuted) return;
	
	StopWallRun();

	FVector WallJumpDirection = LastWallNormal + FVector::UpVector * 1.0f;
	WallJumpDirection.Normalize();

	LaunchCharacter(WallJumpDirection * 800.f, true, true);
}

void APlayerCharacterController::StartFiring()
{
	if (bIsPlayerDeadExecuted) return;

	if (!bCanFire || GetCharacterMovement()->IsFalling() || bIsWeaponHolstering || !bIsWeaponEquipped || !bIsAiming)
	{
		return;
	}

	if (!WeaponManager) return;

	bIsFiring = true;
	bCanFire = false; // Block further StartFire calls

	WeaponManager->StartFire();
}


void APlayerCharacterController::StopFiring()
{
	if (bIsPlayerDeadExecuted) return;

	if (WeaponManager)
	{
		bIsFiring = false;
		WeaponManager->StopFire();
		bCanFire = true;
	}
}

void APlayerCharacterController::Reloading()
{
	if (bIsPlayerDeadExecuted) return;

	if (WeaponManager)
	{
		WeaponManager->TriggerReload();
	}
}

void APlayerCharacterController::StartSwitchToPrimary1()
{
	if (bIsPlayerDeadExecuted) return;

	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsAiming) return;

	if (CurrentWeaponSlot == AssaultRifleData.WeaponName || WeaponManager->IsWeaponHolstered()) return;

	StopFiring();
	bCanFire = false;
	bIsWeaponHolstering = true;
	float HolsterAnimDuration = (HolsterAnimSequence->GetPlayLength()) / 2;
	GetWorldTimerManager().SetTimer(WeaponSwitchTimerHandle, this, &APlayerCharacterController::StopSwitchToPrimary1, HolsterAnimDuration, false);
}

void APlayerCharacterController::StartSwitchToPrimary2()
{
	if (bIsPlayerDeadExecuted) return;

	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsAiming) return;
	
	if (CurrentWeaponSlot == ShotgunData.WeaponName || WeaponManager->IsWeaponHolstered()) return;

	StopFiring();
	bCanFire = false;
	bIsWeaponHolstering = true;
	float HolsterAnimDuration = (HolsterAnimSequence->GetPlayLength()) / 2;

	GetWorldTimerManager().SetTimer(WeaponSwitchTimerHandle, this, &APlayerCharacterController::StopSwitchToPrimary2, HolsterAnimDuration, false);
}

void APlayerCharacterController::StartSwitchToSecondary()
{
	if (bIsPlayerDeadExecuted) return;

	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsAiming) return;
	
	if (CurrentWeaponSlot == PistolData.WeaponName || WeaponManager->IsWeaponHolstered()) return;

	StopFiring();
	bCanFire = false;
	bIsWeaponHolstering = true;
	float HolsterAnimDuration = (HolsterAnimSequence->GetPlayLength()) / 2;

	GetWorldTimerManager().SetTimer(WeaponSwitchTimerHandle, this, &APlayerCharacterController::StopSwitchToSecondary, HolsterAnimDuration, false);

}

void APlayerCharacterController::StartHolsterWeapon()
{
	if (bIsPlayerDeadExecuted) return;

	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsAiming) return;
	
	StopFiring();
	bCanFire = false;
	bIsWeaponHolstering = true;
	bIsWeaponBeingPutAway = !bIsWeaponBeingPutAway;
	float HolsterAnimDuration = (HolsterAnimSequence->GetPlayLength()) / 2;
	float WeaponPuttingAwayAnimDuration = WeaponPuttingAwayAnimSequence->GetPlayLength();

	if (bIsWeaponHolstering && bIsWeaponBeingPutAway)
	{
		GetWorldTimerManager().SetTimer(WeaponSwitchTimerHandle, this, &APlayerCharacterController::StopHolsterWeapon, WeaponPuttingAwayAnimDuration, false);
	}

	else if (bIsWeaponHolstering && !bIsWeaponBeingPutAway)
	{
		GetWorldTimerManager().SetTimer(WeaponSwitchTimerHandle, this, &APlayerCharacterController::StopHolsterWeapon, HolsterAnimDuration, false);
	}

}

void APlayerCharacterController::StartAiming()
{
	if (bIsPlayerDeadExecuted) return;

	if (GetCharacterMovement()->IsFalling()) return;
	
	bIsAiming = true;

	if (GetCharacterMovement()->MaxWalkSpeed = RunSpeed)
	{
		bWasCharacterPreviouslyRunningBeforeAiming = true;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void APlayerCharacterController::StopAiming()
{
	if (bIsPlayerDeadExecuted) return;

	bIsAiming = false;

	if (bIsFiring)
	{
		StopFiring();
	}

	if (bWasCharacterPreviouslyRunningBeforeAiming == true)
	{
		if (bIsRunActionPressed) // Run key is currently held
		{
			GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
		}
		else
		{
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		}

		bWasCharacterPreviouslyRunningBeforeAiming = false;
	}
	else
	{
		// Explicitly reset to walk speed to avoid unintended sprinting
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void APlayerCharacterController::StopSwitchToPrimary1()
{
	if (bIsPlayerDeadExecuted) return;

	bIsWeaponHolstering = false;
	WeaponManager->EquipWeapon(AssaultRifleData);
	CurrentWeaponSlot = AssaultRifleData.WeaponName;
	bIsPrimaryWeapon = true;
	bStoreIsPrimaryWeaponBeforeHolster = bIsPrimaryWeapon;
	bCanFire = true;

}

void APlayerCharacterController::StopSwitchToPrimary2()
{
	if (bIsPlayerDeadExecuted) return;

	bIsWeaponHolstering = false;
	WeaponManager->EquipWeapon(ShotgunData);
	CurrentWeaponSlot = ShotgunData.WeaponName;
	bIsPrimaryWeapon = true;
	bStoreIsPrimaryWeaponBeforeHolster = bIsPrimaryWeapon;
	bCanFire = true;

}

void APlayerCharacterController::StopSwitchToSecondary()
{
	if (bIsPlayerDeadExecuted) return;

	bIsWeaponHolstering = false;
	WeaponManager->EquipWeapon(PistolData);
	CurrentWeaponSlot = PistolData.WeaponName;
	bIsPrimaryWeapon = false;
	bStoreIsPrimaryWeaponBeforeHolster = bIsPrimaryWeapon;
	bCanFire = true;

}

void APlayerCharacterController::StopHolsterWeapon()
{
	if (bIsPlayerDeadExecuted) return;

	bIsWeaponHolstering = false;
	if (WeaponManager)
	{
		WeaponManager->ToggleHolsterWeapon();

		if (bIsWeaponEquipped)
		{
			bIsPrimaryWeapon = bStoreIsPrimaryWeaponBeforeHolster;
		}
	}
	bIsWeaponEquipped = !bIsWeaponEquipped;
	bCanFire = true;

}

void APlayerCharacterController::ReceiveDamage(float Amount)
{
	if (bIsPlayerDeadExecuted) return;

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

	UE_LOG(LogTemp, Warning, TEXT("Current Shield Pool: %f"), CurrentShieldPool);
	UE_LOG(LogTemp, Warning, TEXT("Current Health Pool: %f"), CurrentHealthPool);

	LastDamageTime = GetWorld()->GetTimeSeconds();
}

void APlayerCharacterController::RegenerateShieldAndHealth(float DeltaTime)
{
	if (bIsPlayerDeadExecuted) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	// Regenerate health first if not full and delay passed
	if (CurrentHealthPool < MaxHealthPool &&
		HealthRegenSpeed > 0.0f &&
		(CurrentTime - LastDamageTime) >= HealthRegenDelay)
	{
		CurrentHealthPool += HealthRegenSpeed * DeltaTime;
		CurrentHealthPool = FMath::Clamp(CurrentHealthPool, 0.0f, MaxHealthPool);
	}

	// Only regenerate shield if health is full, shield not full, and shield delay passed
	if (CurrentHealthPool >= MaxHealthPool &&
		CurrentShieldPool < MaxShieldPool &&
		HealthRegenSpeed > 0.0f &&
		(CurrentTime - LastDamageTime) >= ShieldRegenDelay)
	{
		CurrentShieldPool += HealthRegenSpeed * DeltaTime;
		CurrentShieldPool = FMath::Clamp(CurrentShieldPool, 0.0f, MaxShieldPool);
	}
}

void APlayerCharacterController::EnemyTracker()
{
	if (bIsPlayerDead) return;

	if (LevelManager)
	{
		DeadEnemies = LevelManager->GetDeadEnemyCount();
		TotalEnemies = LevelManager->GetTotalEnemies();
	}
}

void APlayerCharacterController::CheckPlayerHealth()
{
	if (CurrentHealthPool <= 0.0f && CurrentShieldPool <= 0.0f)
	{
		if (!bIsPlayerDeadExecuted)
		{
			bIsPlayerDeadExecuted = true;
			float PlayerDeathAnimationDuration = PlayerDeathAnimSequence->GetPlayLength();

			GetWorldTimerManager().SetTimer(PlayerDeathTimerHandle, this, &APlayerCharacterController::ConfirmPlayerDeath, PlayerDeathAnimationDuration, false);
		}
	}
}

void APlayerCharacterController::ConfirmPlayerDeath()
{
	bIsPlayerDead = true;
}

