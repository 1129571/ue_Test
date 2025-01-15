// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootGameMode.h"
#include "Characters/MultiShootCharacter.h"
#include "PlayerController/MultiShootPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/MultiShootPlayerState.h"

void AMultiShootGameMode::PlayerEliminated(class AMultiShootCharacter* ElimmedCharacter, class AMultiShootPlayerController* VictimController, AMultiShootPlayerController* AttacherController)
{
	//当有玩家死亡时, 其伤害来源的玩家获得加分(自杀除外)
	AMultiShootPlayerState* AttackerPlayerState = AttacherController ? Cast<AMultiShootPlayerState>(AttacherController->PlayerState) : nullptr;
	AMultiShootPlayerState* VictimPlayerState = VictimController ? Cast<AMultiShootPlayerState>(VictimController->PlayerState) : nullptr;
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	//处理被击杀的Character
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void AMultiShootGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		//销毁之前先调用Reset将Pawn和Controller安全分离
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> OutPlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), OutPlayerStarts);
		int32 Selection = FMath::RandRange(0, OutPlayerStarts.Num() - 1);
		//随机在PlayerStart生成
		RestartPlayerAtPlayerStart(ElimmedController, OutPlayerStarts[Selection]);
	}
}
