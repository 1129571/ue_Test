// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootGameState.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/MultiShootPlayerState.h"

void AMultiShootGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMultiShootGameState, TopScorePlayers);
}

void AMultiShootGameState::UpdateTopScore(AMultiShootPlayerState* InMultiShootPlayerState)
{
	if (TopScorePlayers.Num() == 0)
	{
		TopScorePlayers.Add(InMultiShootPlayerState);
		TopScore = InMultiShootPlayerState->GetScore();
	}
	else
	{
		if (TopScore == InMultiShootPlayerState->GetScore())
		{
			TopScorePlayers.AddUnique(InMultiShootPlayerState);
		}
		else if (TopScore < InMultiShootPlayerState->GetScore())
		{
			TopScorePlayers.Empty();
			TopScorePlayers.AddUnique(InMultiShootPlayerState);
			TopScore = InMultiShootPlayerState->GetScore();
		}
	}
}
