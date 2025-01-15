// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootPlayerController.h"
#include "HUD/MultiShootHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Characters/MultiShootCharacter.h"

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

		FString HealthString = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(InCurrentHealth), FMath::CeilToInt(InMaxHealth));
		MultiHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthString));
	}
}

void AMultiShootPlayerController::SetHUDScore(float InScore)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	bool bHUDValid = 
		MultiHUD && 
		MultiHUD->CharacterOverlay && 
		MultiHUD->CharacterOverlay->ScoreAmount;

	if (bHUDValid)
	{
		FString ScoreString = FString::Printf(TEXT("%d"), FMath::FloorToInt(InScore));
		MultiHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreString));
	}
}

void AMultiShootPlayerController::SetHUDDefeats(int32 InDefeatsAmount)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	bool bHUDValid =
		MultiHUD &&
		MultiHUD->CharacterOverlay &&
		MultiHUD->CharacterOverlay->DefeatsAmount;

	if (bHUDValid)
	{
		FString DefeatsString = FString::Printf(TEXT("%d"), InDefeatsAmount);
		MultiHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsString));
	}
}

void AMultiShootPlayerController::SetHUDWeaponAmmoAmount(int32 InAmmoAmount)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	bool bHUDValid =
		MultiHUD &&
		MultiHUD->CharacterOverlay &&
		MultiHUD->CharacterOverlay->WeaponAmmoAmount;

	if (bHUDValid)
	{
		FString AmmoString = FString::Printf(TEXT("%d"), InAmmoAmount);
		MultiHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoString));
	}
}

void AMultiShootPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);
	if (aPawn)
	{
		AMultiShootCharacter* MultiShootCharacter = Cast<AMultiShootCharacter>(aPawn);
		if (MultiShootCharacter)
		{
			// 效果应该是一样的, 上面更直观, 下面更合理
			// MultiShootCharacter->UpdateHUDHealth();
			SetHUDHealth(MultiShootCharacter->GetCurrentHealth(), MultiShootCharacter->GetMaxHealth());
		}
	}
}

void AMultiShootPlayerController::BeginPlay()
{
	Super::BeginPlay();

	MultiHUD = Cast<AMultiShootHUD>(GetHUD());
}
