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

	/*
	* Replication notifies, 定义在这里仅为了和Score保持一致
	*/
	UFUNCTION()
	virtual void OnRep_Defeats();
	void AddToDefeats(int32 InDefeats);

	virtual void OnRep_Score() override;
	void AddToScore(float InScore);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	//PlayerState没有提供获取Controller的函数, 我们需要通过Character获取, 以便于访问HUD
	UPROPERTY()
	class AMultiShootCharacter* Character;
	UPROPERTY()
	class AMultiShootPlayerController* Controller;

	//死亡次数
	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
};
