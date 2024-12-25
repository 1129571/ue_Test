// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	//GameMode值存在于服务器
	//当进入Lobby关卡的人数达到一定数量就ServerTravel新关卡
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

private:
	int32 MinPlayerNumbers = 2;
	const FString& TargetLevelPath = TEXT("");
};
