#include "LevelManager.h"
#include "EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameData.h"

ALevelManager::ALevelManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALevelManager::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyBase::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		AEnemyBase* Enemy = Cast<AEnemyBase>(Actor);
		if (Enemy)
		{
			TrackedEnemies.Add(Enemy);

			if (!Enemy->OnEnemyDeath.IsAlreadyBound(this, &ALevelManager::OnEnemyDied))
			{
				Enemy->OnEnemyDeath.AddDynamic(this, &ALevelManager::OnEnemyDied);
			}
		}
	}

	TotalEnemies = TrackedEnemies.Num();
	UE_LOG(LogTemp, Warning, TEXT("Number of Enemies: %d"), TotalEnemies);

	CheckAllEnemiesDead(); // In case some enemies start dead
}

void ALevelManager::OnEnemyDied(AEnemyBase* DeadEnemy)
{
	DeadEnemyCount++;
	UE_LOG(LogTemp, Warning, TEXT("Enemy died. Total Dead: %d / %d"), DeadEnemyCount, TotalEnemies);
	CheckAllEnemiesDead();
}

void ALevelManager::CheckAllEnemiesDead()
{
	if (DeadEnemyCount >= TotalEnemies)
	{
		UE_LOG(LogTemp, Warning, TEXT("All enemies eliminated! Level complete."));
		// Trigger level complete UI, next level, etc.

		FString CurrentLevelName = GetWorld()->GetMapName();
		CurrentLevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix); // removes "UEDPIE_0_" or similar

		if (CurrentLevelName.Equals("GameLevel_1"))
		{
			// Create or load existing save game
			USaveGameData* SaveGameInstance = Cast<USaveGameData>(
				UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSaveSlot"), 0));

			if (!SaveGameInstance)
			{
				SaveGameInstance = Cast<USaveGameData>(
					UGameplayStatics::CreateSaveGameObject(USaveGameData::StaticClass()));
			}

			SaveGameInstance->bIsLevel2Unlocked = true;

			UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("PlayerSaveSlot"), 0);

			UE_LOG(LogTemp, Warning, TEXT("Level 2 unlocked and saved."));
		}

		UGameplayStatics::OpenLevel(GetWorld(), FName("MainMenuMap"));
	}
}
