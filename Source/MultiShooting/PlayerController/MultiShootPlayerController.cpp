// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootPlayerController.h"
#include "HUD/MultiShootHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void AMultiShootPlayerController::SetHUDHealth(float InCurrentHealth, float InMaxHealth)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	//建议对所有(需要操作的)指针都进行非空检查
	bool bHUDValid = MultiHUD && 
		MultiHUD->CharacterOverlay && 
		MultiHUD->CharacterOverlay->HealthBar && 
		MultiHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = InCurrentHealth / InMaxHealth;
		MultiHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);

		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(InCurrentHealth), FMath::CeilToInt(InMaxHealth));
		MultiHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void AMultiShootPlayerController::BeginPlay()
{
	Super::BeginPlay();

	MultiHUD = Cast<AMultiShootHUD>(GetHUD());
}
