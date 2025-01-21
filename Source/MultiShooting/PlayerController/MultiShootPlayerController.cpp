// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootPlayerController.h"
#include "HUD/MultiShootHUD.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/Announcement.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Characters/MultiShootCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameMode/MultiShootGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "MultiShootComponents/CombatComponent.h"
#include "GameState/MultiShootGameState.h"
#include "PlayerState/MultiShootPlayerState.h"

//处理客户端与服务器之间的连接以及玩家身份的初始化。
//我们希望尽早获取到CS时间差量
void AMultiShootPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AMultiShootPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//添加准备状态的Widget
	MultiHUD = Cast<AMultiShootHUD>(GetHUD());
	ServerCheckMatchState();
}

void AMultiShootPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);

	// 手动进入InProgress状态才会生成CharacterOverlay, 同时我们在OnPossess时尝试初始化HealthHUD, 
	// 但是无法确定谁先谁后, 有时候会失败
	if (bInitializedCharacterOverlay)
	{
		PollInit();
	}
}

void AMultiShootPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMultiShootPlayerController, GameMatchState);
}

void AMultiShootPlayerController::CheckTimeSync(float DeltaTime)
{
	//每隔TimeSyncFrequency进行一次时间同步
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds()); 
		TimeSyncRunningTime = 0.f;
	}
}

void AMultiShootPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AMultiShootPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	// 客户端发起ServerRPC 到 服务器调用ClientRPC 所花的时间
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	// 服务器当前时间(ClientRPC接收的瞬间), 这里认为ServerRPC和ClientRPC所花的时间是一致的
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	// 客户端和服务器的时间差量
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

//通过客户端当前时间 + CS时间差量可以获取到服务器时间
float AMultiShootPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AMultiShootPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (MultiHUD && MultiHUD->CharacterOverlay)
		{
			CharacterOverlay = MultiHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth,HUDMaxHealth);
				// Score和Defeats考虑是否有必要
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

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
	else
	{
		bInitializedCharacterOverlay = true;
		HUDHealth = InCurrentHealth;
		HUDMaxHealth = InMaxHealth;
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
	else
	{
		bInitializedCharacterOverlay = true;
		HUDScore = InScore;
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
	else
	{
		bInitializedCharacterOverlay = true;
		HUDDefeats = InDefeatsAmount;
	}
}

void AMultiShootPlayerController::SetHUDWeaponAmmoAmount(int32 InWeaponAmmoAmount)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	bool bHUDValid =
		MultiHUD &&
		MultiHUD->CharacterOverlay &&
		MultiHUD->CharacterOverlay->WeaponAmmoAmount;

	if (bHUDValid)
	{
		FString WeaponAmmoString = FString::Printf(TEXT("%d"), InWeaponAmmoAmount);
		MultiHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(WeaponAmmoString));
	}
}

void AMultiShootPlayerController::SetHUDCarriedAmmoAmount(int32 InCarriedAmmoAmount)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	bool bHUDValid =
		MultiHUD &&
		MultiHUD->CharacterOverlay &&
		MultiHUD->CharacterOverlay->CarriedAmmoAmount;

	if (bHUDValid)
	{
		FString CarriedAmmoString = FString::Printf(TEXT("%d"), InCarriedAmmoAmount);
		MultiHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoString));
	}
}

// 仅处理倒计时相关, 不处理其他Widget控件(倒计时需要在Tick调用, 避免资源浪费)
void AMultiShootPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (GameMatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime + LevelStartingTime - GetServerTime();
	else if (GameMatchState == MatchState::InProgress) TimeLeft = MatchTime + LevelStartingTime + WarmupTime - GetServerTime();
	else if (GameMatchState == MatchState::Cooldown) TimeLeft = MatchTime + LevelStartingTime + WarmupTime + CooldownTime - GetServerTime();
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	// 只有剩余时间(S)变化时才更新HUD
	if (SecondsLeft != CountdowmInt)
	{
		//更新热身状态的时间 && 更新比赛结束冷却的倒计时
		if (GameMatchState == MatchState::WaitingToStart || GameMatchState == MatchState::Cooldown) 
			SetHUDAnnouncementCountdown(TimeLeft);
		//更新比赛状态的倒计时
		else if (GameMatchState == MatchState::InProgress)  
			SetHUDMatchCountdown(TimeLeft);
	
	}

	CountdowmInt = SecondsLeft;
}

void AMultiShootPlayerController::SetHUDMatchCountdown(float InCountdownTime)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	bool bHUDValid =
		MultiHUD &&
		MultiHUD->CharacterOverlay &&
		MultiHUD->CharacterOverlay->GameTimeText;

	if (bHUDValid)
	{

		if (InCountdownTime < 0.f)
		{
			MultiHUD->CharacterOverlay->GameTimeText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(InCountdownTime / 60.f);
		int32 Seconds = InCountdownTime - Minutes * 60.f;

		FString MatchCountdownString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		MultiHUD->CharacterOverlay->GameTimeText->SetText(FText::FromString(MatchCountdownString));
	}
}

void AMultiShootPlayerController::SetHUDAnnouncementCountdown(float InCountdownTime)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	bool bHUDValid =
		MultiHUD &&
		MultiHUD->Announcement &&
		MultiHUD->Announcement->WarmupTimeText;

	if (bHUDValid)
	{
		if (InCountdownTime < 0.f)
		{
			MultiHUD->Announcement->WarmupTimeText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(InCountdownTime / 60.f);
		int32 Seconds = InCountdownTime - Minutes * 60.f;

		FString MatchCountdownString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		MultiHUD->Announcement->WarmupTimeText->SetText(FText::FromString(MatchCountdownString));
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

//Server上的PlayerController从GameMode获取一些信息, 传递给Client的PlayerController
void AMultiShootPlayerController::ServerCheckMatchState_Implementation()
{
	AMultiShootGameMode* GameMode = Cast<AMultiShootGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		GameMatchState = GameMode->GetMatchState();
		MatchTime = GameMode->MatchTime;
		LevelStartingTime = GameMode->LevelStaringTime;
		CooldownTime = GameMode->CooldownTime;
		ClientJoinMidgame(GameMatchState, LevelStartingTime, MatchTime, WarmupTime, CooldownTime);
	}
}

void AMultiShootPlayerController::ClientJoinMidgame_Implementation(FName InMatchState, float InLevelStartingTime, float InMatchTime, float InWarmupTime, float InCooldownTime)
{
	WarmupTime = InWarmupTime;
	LevelStartingTime = InLevelStartingTime;
	MatchTime = InMatchTime;
	GameMatchState = InMatchState;
	CooldownTime = InCooldownTime;
	OnGameMatchStateSet(GameMatchState);

	//热身阶段Widget
	if (MultiHUD)
	{
		MultiHUD->AddAnnouncement();
	}
}

void AMultiShootPlayerController::OnGameMatchStateSet(FName InMatchState)
{
	GameMatchState = InMatchState;

	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	if (MultiHUD == nullptr) return;

	// 只有在InProgress状态才会生成Widget并添加Viewport
	if (GameMatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (GameMatchState == MatchState::Cooldown)
	{
		HandleMatchHasCooldown();
	}
}

void AMultiShootPlayerController::OnRep_GameMatchSate()
{
	if (GameMatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (GameMatchState == MatchState::Cooldown)
	{
		HandleMatchHasCooldown();
	}
}
void AMultiShootPlayerController::HandleMatchHasStarted()
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	if (MultiHUD)
	{
		if (MultiHUD->CharacterOverlay == nullptr) MultiHUD->AddCharacterOverlay();
		if (MultiHUD->Announcement)
		{
			MultiHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

//处理Announcement控件的非倒计时部分, 这部分不需要在Tick调用
void AMultiShootPlayerController::HandleMatchHasCooldown()
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;
	if (MultiHUD)
	{
		if (MultiHUD->CharacterOverlay)
		{
			MultiHUD->CharacterOverlay->RemoveFromParent();
		}

		bool bAnnouncementValid =
			MultiHUD->Announcement &&
			MultiHUD->Announcement->AnnouncementText &&
			MultiHUD->Announcement->InfoText;

		if (bAnnouncementValid)
		{
			MultiHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementStr(TEXT("比赛结束, 新比赛即将开始:"));
			MultiHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementStr));

			AMultiShootGameState* MultiShootGameState = Cast<AMultiShootGameState>(UGameplayStatics::GetGameState(this));
			AMultiShootPlayerState* MultiShootPlayerState = GetPlayerState<AMultiShootPlayerState>();

			if (MultiShootGameState && MultiShootPlayerState)
			{
				FString InfoStr;
				TArray<AMultiShootPlayerState*> TopPlayerStatesArry = MultiShootGameState->TopScorePlayers;
				if (TopPlayerStatesArry.Num() == 0)
				{
					InfoStr = TEXT("本局游戏没有击杀任何玩家, 没有Owner");
				}
				else
				{
					InfoStr = TEXT("本局游戏击杀数最多的玩家为:\n");
					for (auto TopPlayerStates : TopPlayerStatesArry)
					{
						InfoStr.Append(FString::Printf(TEXT("%s\n"), *TopPlayerStates->GetPlayerName()));
					}
					if (TopPlayerStatesArry.Contains(MultiShootPlayerState))
					{
						FString EndStr = TopPlayerStatesArry.Num() == 1 ? TEXT("唯一胜利者") : TEXT("胜利者之一");
						InfoStr.Append(FString::Printf(TEXT("\n恭喜!! 你是比赛的%s!!"), *EndStr));
					}
				}

				MultiHUD->Announcement->InfoText->SetText(FText::FromString(InfoStr));
			}
		}
	}

	AMultiShootCharacter* MultiShootCharacter = Cast<AMultiShootCharacter>(GetPawn());
	if (MultiShootCharacter && MultiShootCharacter->GetCombatComponent())
	{
		//该状态禁用部分输入
		MultiShootCharacter->bDisableGameplay = true;
		//如果正在自动开火也应该停止
		MultiShootCharacter->GetCombatComponent()->WeaponFire(false);
	}
}

