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
	if (MultiHUD)
	{
		MultiHUD->AddAnnouncement();
	}
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

void AMultiShootPlayerController::SetHUDMatchCountdown(float InCountdownTime)
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	bool bHUDValid =
		MultiHUD &&
		MultiHUD->CharacterOverlay &&
		MultiHUD->CharacterOverlay->GameTimeText;

	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(InCountdownTime / 60.f);
		int32 Seconds = InCountdownTime - Minutes * 60.f;

		FString MatchCountdownString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		MultiHUD->CharacterOverlay->GameTimeText->SetText(FText::FromString(MatchCountdownString));
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

void AMultiShootPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());

	// 只有剩余时间(S)变化时才更新HUD
	if (SecondsLeft != CountdowmInt)
	{
		SetHUDMatchCountdown(MatchTime - GetServerTime());
	}

	CountdowmInt = SecondsLeft;
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
}

void AMultiShootPlayerController::HandleMatchHasStarted()
{
	MultiHUD->AddCharacterOverlay();
	if (MultiHUD->Announcement)
	{
		MultiHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AMultiShootPlayerController::OnRep_GameMatchSate()
{
	MultiHUD = MultiHUD == nullptr ? Cast<AMultiShootHUD>(GetHUD()) : MultiHUD;

	if (MultiHUD == nullptr) return;

	if (GameMatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
}
