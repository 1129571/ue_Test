// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootPlayerState.h"
#include "Characters/MultiShootCharacter.h"
#include "PlayerController/MultiShootPlayerController.h"


void AMultiShootPlayerState::AddToScore(float InScore)
{
	Score += InScore;
	Character = Character == nullptr ? Cast<AMultiShootCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(Score);
		}
	}
}

void AMultiShootPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<AMultiShootCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(Score);
		}
	}
}

