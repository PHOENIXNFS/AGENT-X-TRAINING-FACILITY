#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelManager.generated.h"

class AEnemyBase;

UCLASS()
class COMBATSYSTEM_API ALevelManager : public AActor
{
	GENERATED_BODY()

public:
	ALevelManager();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<AEnemyBase*> TrackedEnemies;

	UPROPERTY()
	int32 TotalEnemies = 0;

	UPROPERTY()
	int32 DeadEnemyCount = 0;

	UFUNCTION()
	void OnEnemyDied(AEnemyBase* DeadEnemy);

	void CheckAllEnemiesDead();

public:

	UFUNCTION(BlueprintCallable, Category = "Enemy Tracking")
	int32 GetDeadEnemyCount() const { return DeadEnemyCount; }

	UFUNCTION(BlueprintCallable, Category = "Enemy Tracking")
	int32 GetTotalEnemies() const { return TotalEnemies; }
};
