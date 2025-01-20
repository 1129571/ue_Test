// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MultiShootGameState.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API AMultiShootGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	TArray<class AMultiShootPlayerState*> TopScorePlayers;			//最高分PlayerState , 可能多个并列

	void UpdateTopScore(AMultiShootPlayerState* InMultiShootPlayerState);

private:
	float TopScore = 0.f;
};
