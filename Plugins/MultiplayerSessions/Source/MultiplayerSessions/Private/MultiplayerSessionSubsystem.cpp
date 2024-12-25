// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionSubsystem::UMultiplayerSessionSubsystem():
	//绑定委托, 也可以在构造函数内部绑定
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionCompleteCallback)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsCompleteCallback)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionCompleteCallback)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionCompleteCallback)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionCompleteCallback))
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
}

void UMultiplayerSessionSubsystem::CreatSession(int32 InNumPublicConnections, FString InSessionMatchType)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	auto ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession != nullptr)
	{
		//销毁Session这个操作需要通过网络发送到托管平台执行
		//因此先销毁, 并在销毁的回调进行创建Session
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = InNumPublicConnections;
		LastMatchType = InSessionMatchType;
		DestroySession();
		return;
	}

	// 添加到委托列表并保存委托句柄, 便于在不需要它的时候将其从在线服务的委托列表移除
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	
	// Session参数设置
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	// 如果连接了在线服务(如: Steam)就使用网络Session, 否则就使用局域网Session
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSettings->NumPublicConnections = InNumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;		//从所在区域搜索会话
	LastSessionSettings->bShouldAdvertise = true;			//允许平台使用广告发布Session, 其他用户可以选择加入
	LastSessionSettings->bUsesPresence = true;				//是否显示用户状态信息
	LastSessionSettings->bUseLobbiesIfAvailable = true;			//如果平台支持大厅API(通常用于多人在线游戏中),是否优先使用
	// 自定义的Session键值对, Value常表示Session类型. 如游戏的匹配类型
	// 服务在进行匹配、查找玩家或展示游戏大厅时，不仅通过在线服务（例如，游戏服务器、匹配服务等）来进行广告宣传，
	// 还通过网络Ping数据（即玩家的连接质量、延迟等信息）来优化和展示游戏大厅或游戏信息。
	LastSessionSettings->Set(FName("MatchType"), InSessionMatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;

	//尝试创建Session
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), SessionName, *LastSessionSettings))
	{
		//如果创建失败, 从服务委托列表移除CreateSessionCompleteDelegate
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		//直接失败, 调用委托
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionSubsystem::FindSession(int32 InMaxSearchResults)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	//添加到在线服务委托列表
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	//设置搜索参数(和CreateSession部分参数要对应), 同时也作为出参, 保存搜索结果
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = InMaxSearchResults;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	//QuerySettings用于筛选会话, 下方代码可以筛选掉不在线的会话
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	//尝试FindSession
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		// 如果Find失败, 就执行自定义委托(调用上层回调) 并 从服务委托列表移除FindSessionComplete委托
		MultiplayerOnFindSessionComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);

		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}
}

void UMultiplayerSessionSubsystem::JoinSession(const FOnlineSessionSearchResult& InSessionResult)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	//尝试加入Session
	ULocalPlayer* Localplayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*Localplayer->GetPreferredUniqueNetId(), SessionName, InSessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	
	if (!SessionInterface->DestroySession(SessionName))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionSubsystem::StartSession()
{
}

void UMultiplayerSessionSubsystem::OnCreateSessionCompleteCallback(FName InSessionName, bool InbWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}
	MultiplayerOnCreateSessionComplete.Broadcast(InbWasSuccessful);
}

void UMultiplayerSessionSubsystem::OnFindSessionsCompleteCallback(bool InbWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	//没有找到Session
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiplayerOnFindSessionComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	MultiplayerOnFindSessionComplete.Broadcast(LastSessionSearch->SearchResults, InbWasSuccessful);
}

void UMultiplayerSessionSubsystem::OnJoinSessionCompleteCallback(FName InSessionName, EOnJoinSessionCompleteResult::Type InJoinResult)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}
	MultiplayerOnJoinSessionComplete.Broadcast(InJoinResult);
}

void UMultiplayerSessionSubsystem::OnDestroySessionCompleteCallback(FName InSessionName, bool InbWasSuccessful)
{
	//销毁Session执行完成
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	//在尝试创建新Session时成功将旧Session销毁
	if (InbWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreatSession(LastNumPublicConnections, LastMatchType);
	}
	MultiplayerOnDestroySessionComplete.Broadcast(InbWasSuccessful);
}

void UMultiplayerSessionSubsystem::OnStartSessionCompleteCallback(FName InSessionName, bool InbWasSuccessful)
{
}
