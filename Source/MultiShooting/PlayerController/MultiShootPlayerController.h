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
	virtual void OnPossess(APawn* aPawn) override;

protected:
	virtual void BeginPlay() override;

private:

	class AMultiShootHUD* MultiHUD;

};
