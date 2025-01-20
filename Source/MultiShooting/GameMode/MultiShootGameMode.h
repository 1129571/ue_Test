// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiShootGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API AMultiShootGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AMultiShootGameMode();

	virtual void Tick(float DeltaSeconds) override;

	float LevelStaringTime = 0.f;		//地图打开的时间, 热身时间要从地图打开开始

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 15.f;			//热身时间

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 180.f;			//比赛时间

	/**
	 * 玩家淘汰时将其从游戏移除
	 * @param ElimmedCharacter 淘汰被移除的Character
	 * @param VictimController 受害者控制器
	 * @param AttacherController 加害者控制器
	 */
	virtual void PlayerEliminated(
		class AMultiShootCharacter* ElimmedCharacter, 
		class AMultiShootPlayerController* VictimController, 
		AMultiShootPlayerController* AttacherController
	);

	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

protected:
	virtual void BeginPlay() override;

	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;		//Tick 记录当前的热身时间
};
