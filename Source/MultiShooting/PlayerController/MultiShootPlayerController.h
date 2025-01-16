// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MultiShootPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API AMultiShootPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	void SetHUDHealth(float InCurrentHealth, float InMaxHealth);
	void SetHUDScore(float InScore);			//这里使用float是因为Score是PlayerState提供的
	void SetHUDDefeats(int32 InDefeatsAmount);
	void SetHUDWeaponAmmoAmount(int32 InWeaponAmmoAmount);			//武器内的弹药
	void SetHUDCarriedAmmoAmount(int32 InCarriedAmmoAmount);		//玩家携带的弹药

	virtual void OnPossess(APawn* aPawn) override;

protected:
	virtual void BeginPlay() override;

private:

	UPROPERTY()
	class AMultiShootHUD* MultiHUD;

};
