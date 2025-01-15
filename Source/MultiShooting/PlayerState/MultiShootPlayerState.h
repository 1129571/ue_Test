// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MultiShootPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API AMultiShootPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void OnRep_Score() override;
	void AddToScore(float InScore);

private:
	//PlayerState没有提供获取Controller的函数, 我们需要通过Character获取, 以便于访问HUD
	class AMultiShootCharacter* Character;
	class AMultiShootPlayerController* Controller;
};
