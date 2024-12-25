// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Interfaces/OnlineSessionInterface.h"

#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Widget 的一些初始化及相关操作
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 InNumPublicConnections = 4, FString InMatchType = FString(TEXT("FreeForAll")), FString InLobbyLevel = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));

protected:
	// 初始化时将按钮和函数进行绑定
	virtual bool Initialize() override;

	//前往其他关卡时执行该函数
	virtual void NativeDestruct() override;

	// 创建Session完成的委托回调(动态委托, 因此回调需要是UFUNCTION)
	UFUNCTION()
	void OnCreatSessionCallback(bool bResult);
	void OnFindSessionCallback(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bResult);
	void OnJoinSessionCallback(EOnJoinSessionCompleteResult::Type JoinResult);
	UFUNCTION()
	void OnDestroySessionCallback(bool bResult);
	UFUNCTION()
	void OnStartSessionCallback(bool bResult);

private:
	// 和UMG中的Button关联, 注意按钮名称要完全一致
	UPROPERTY(meta = (BindWidget))			//蓝图的按钮将链接到这个c++变量
	class UButton* Button_Host;
	UPROPERTY(meta = (BindWidget))
	class UButton* Button_Join;

	// 用于将Widget和子系统进行关联
	class UMultiplayerSessionSubsystem* MultiplayerSessionSubsystem;

	// 此处仅仅是定义了方法, 并没有与按钮进行绑定
	UFUNCTION()
	void Button_HostClicked();
	UFUNCTION()
	void Button_JoinClicked();

	// MenuSetup中设置了输入模式仅UI, 在此处进行还原
	void MenuTearDown();

	//创建Session时的参数, 通过函数从蓝图传递进来
	int32 NumPublicConnections;
	FString MatchTypeValue;

	FString PathToLobbyLevel = TEXT("");
};