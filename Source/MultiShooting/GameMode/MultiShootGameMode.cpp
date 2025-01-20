// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootGameMode.h"
#include "Characters/MultiShootCharacter.h"
#include "PlayerController/MultiShootPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/MultiShootPlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}

AMultiShootGameMode::AMultiShootGameMode()
{
	//Match State 会停留在WaitingToStart, 直到我们手动调用StartMatch()
	//这期间不会自动生成GameMode指定的Pawn, 而是生成DefaultPawn
	bDelayedStart = true;
}

void AMultiShootGameMode::BeginPlay()
{
	Super::BeginPlay();

	//这时候会进入Map
	LevelStaringTime = GetWorld()->GetTimeSeconds();
}

void AMultiShootGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == MatchState::WaitingToStart)
	{
		//希望从BeginPlay(关卡加载完成后)开始热身倒计时
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStaringTime;
		if (CountdownTime <= 0.f)
		{
			//进入InProgress状态
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStaringTime;
		if (CountdownTime <= 0.f)
		{
			//进入Cooldown状态
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = WarmupTime + MatchTime + CooldownTime - GetWorld()->GetTimeSeconds() + LevelStaringTime;
		if (CountdownTime <= 0.f)
		{
			//GameMode提供的函数
			RestartGame();
		}
	}
}

void AMultiShootGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	//告诉所有PlayerController当前MatchState
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMultiShootPlayerController* MultiShootController = Cast<AMultiShootPlayerController>(*It);
		if (MultiShootController)
		{
			MultiShootController->OnGameMatchStateSet(MatchState);
		}
	}
}

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


