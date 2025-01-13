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
};
