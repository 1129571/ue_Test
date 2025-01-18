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
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHUDHealth(float InCurrentHealth, float InMaxHealth);
	void SetHUDScore(float InScore);			//这里使用float是因为Score是PlayerState提供的
	void SetHUDDefeats(int32 InDefeatsAmount);
	void SetHUDWeaponAmmoAmount(int32 InWeaponAmmoAmount);			//武器内的弹药
	void SetHUDCarriedAmmoAmount(int32 InCarriedAmmoAmount);		//玩家携带的弹药
	void SetHUDMatchCountdown(float InCountdownTime);				//游戏倒计时(秒)

	virtual void OnPossess(APawn* aPawn) override;

	virtual float GetServerTime();					//获取和Server同步后的时间
	virtual void ReceivedPlayer() override;			//希望尽可能早地将时间同步

	virtual void OnGameMatchStateSet(FName InMatchState);

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();

	/**
	* 同步客户端和服务器的时间(RPC是需要时间的)
	* 这里指的是player加入游戏开始 的时间(s)
	*/

	//客户端调用, 请求当前服务器时间, 给服务器传入 客户端RPC请求发起的时间
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	//服务器调用, 给客户端传递 客户端RPC请求发起的时间 和 服务器的当前时间(被ServerRPC的瞬间)
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	//客户端和服务器的时间差量
	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category = "Time")		//隔多久同步一次
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;				//距离上次同步的时间

	void CheckTimeSync(float DeltaTime);

	void PollInit();
private:

	UPROPERTY()
	class AMultiShootHUD* MultiHUD;

	//模拟 : 游戏时长为120s
	float MatchTime = 120.f;
	//游戏剩余时间
	uint32 CountdowmInt = 0;

	//来自GameMode的MatchState, 用于在bDelayedStart = true 时响应一些行为, 如HUD的隐藏和显示等
	UPROPERTY(ReplicatedUsing = OnRep_GameMatchSate)
	FName GameMatchState;

	UFUNCTION()
	void OnRep_GameMatchSate();

	UPROPERTY()
	class	UCharacterOverlay* CharacterOverlay;

	// 如果SetHUD时CharacterOverlay不存在, 就把它相关参数保留
	bool bInitializedCharacterOverlay = false;
	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
};
