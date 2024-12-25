// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerSessionSubsystem.generated.h"

//
// 自定义委托, 用于让外部(Menu)绑定.
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bResult);
// 这里使用静态委托,因为我们也没必要让蓝图知道,并且FOnlineSessionSearchResult是一个纯C++类, 无法在蓝图使用
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bResult);
// EOnJoinSessionCompleteResult也与蓝图不兼容, 所以不能使用动态委托
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type JoinResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bResult);



/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	//构造函数, 绑定委托
	UMultiplayerSessionSubsystem();

	//
	// 处理Session的方法, 供其他类调用
	// 这些方法都是异步的, 因此我们只负责调用
	//
	void CreatSession(int32 InNumPublicConnections, FString InSessionMatchType);
	void FindSession(int32 InMaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& InSessionResult);
	void DestroySession();
	void StartSession();

	//
	// 创建Session完成后给外部的委托
	//
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionComplete MultiplayerOnFindSessionComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

protected:
	//
	// 委托的回调函数, 这些函数的入参参考其对应的委托定义
	//
	void OnCreateSessionCompleteCallback(FName InSessionName, bool InbWasSuccessful);
	void OnFindSessionsCompleteCallback(bool InbWasSuccessful);
	void OnJoinSessionCompleteCallback(FName InSessionName, EOnJoinSessionCompleteResult::Type InJoinResult);
	void OnDestroySessionCompleteCallback(FName InSessionName, bool InbWasSuccessful);
	void OnStartSessionCompleteCallback(FName InSessionName, bool InbWasSuccessful);
private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	FName SessionName = NAME_GameSession;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	//
	// 用于加入到OnlineSessionInterface的委托列表中, 这些委托已经被提供
	// 我们需要提供委托的回调函数并绑定它们
	//
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;

	//
	// 将委托添加到托管的服务委托列表时有一个FDelegate返回值,
	// 我们可以通过它在合适的时候将委托从OnlineSessionInterface的委托列表中移除
	//
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	//临时记录创建Session所需的数据(创建时已经存在Session需要先删除,因此需要将创建参数记录)
	bool bCreateSessionOnDestroy = false;
	int32 LastNumPublicConnections;
	FString LastMatchType;
};
