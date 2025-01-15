// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootPlayerState.h"
#include "Characters/MultiShootCharacter.h"
#include "PlayerController/MultiShootPlayerController.h"
#include "Net/UnrealNetwork.h"


void AMultiShootPlayerState::AddToScore(float InScore)
{
	SetScore(GetScore() + InScore);
	Character = Character == nullptr ? Cast<AMultiShootCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
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
			Controller->SetHUDScore(GetScore());
		}
	}
}

void AMultiShootPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMultiShootPlayerState, Defeats);
}

void AMultiShootPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<AMultiShootCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void AMultiShootPlayerState::AddToDefeats(int32 InDefeats)
{
	Defeats += InDefeats;
	Character = Character == nullptr ? Cast<AMultiShootCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}


