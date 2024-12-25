// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (Button_Host)
	{
		Button_Host->OnClicked.AddDynamic(this, &UMenu::Button_HostClicked);
	}
	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this, &UMenu::Button_JoinClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::MenuSetup(int32 InNumPublicConnections, FString InMatchType, FString InLobbyLeve)
{
	NumPublicConnections = InNumPublicConnections;
	MatchTypeValue = InMatchType;
	PathToLobbyLevel = FString::Printf(TEXT("%s?listen"), *InLobbyLeve);

	//添加到视口
	AddToViewport();
	//可视
	SetVisibility(ESlateVisibility::Visible);
	//可被聚焦
	bIsFocusable = true;

	UWorld* World = GetWorld();
	//设置输入模式和鼠标显示
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController(); 
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);

			PlayerController->SetShowMouseCursor(true);
		}
	}

	//获取MultiplayerSessionSubsystem单例
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>();
	}

	//绑定回调
	if (MultiplayerSessionSubsystem)
	{
		MultiplayerSessionSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreatSessionCallback);
		MultiplayerSessionSubsystem->MultiplayerOnFindSessionComplete.AddUObject(this, &ThisClass::OnFindSessionCallback);
		MultiplayerSessionSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSessionCallback);
		MultiplayerSessionSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySessionCallback);
		MultiplayerSessionSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSessionCallback);

	}
}

void UMenu::Button_HostClicked()
{
	//防止按钮被多次点击, 在回调中, 如果失败了再恢复点击
	Button_Host->SetIsEnabled(false);
	if (MultiplayerSessionSubsystem)
	{	//创建Session
		MultiplayerSessionSubsystem->CreatSession(NumPublicConnections, MatchTypeValue);
	}
}

void UMenu::Button_JoinClicked()
{
	//该按钮包含了寻找和加入的功能, 因此两者失败了都需要恢复按钮点击
	Button_Join->SetIsEnabled(false);
	if (MultiplayerSessionSubsystem)
	{
		//因为使用了Steam开发者APPID, 可能有很多人在测试,所以尝试把这个值设置大一点,确保能找到我们的Session
		MultiplayerSessionSubsystem->FindSession(10000);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	//设置输入模式和鼠标显示
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);

			PlayerController->SetShowMouseCursor(false);
		}
	}

}

void UMenu::OnCreatSessionCallback(bool bResult)
{
	if (bResult)
	{
		//打开新关卡
		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel(PathToLobbyLevel);
		}
	}
	else {
		Button_Host->SetIsEnabled(true);
	}
	FString DebugMessage = bResult ? TEXT("Create Session Successfully!") : TEXT("Create Session Failed!");
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, DebugMessage);
	}
}

void UMenu::OnFindSessionCallback(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bResult)
{
	if (MultiplayerSessionSubsystem == nullptr)
	{
		return;
	}

	//找到之后尝试加入第一个Session(符合我们的自定义键值对: "MatchType" - MatchTypeValue)
	for (auto SerachResult : SearchResults)
	{
		FString SessionValue;
		SerachResult.Session.SessionSettings.Get(FName("MatchType"), SessionValue);

		//找到了和创建Session具有相同自定义键值对的Session
		if (SessionValue.Equals(MatchTypeValue))
		{
			//尝试加入该Session
			MultiplayerSessionSubsystem->JoinSession(SerachResult);
			//在本例中后续遍历就没有必要
			return;
		}
	}
	//如果寻找失败 或者 找到0个结果, 启用Join按钮便于再次寻找
	if (!bResult || SearchResults.Num() == 0)
	{
		Button_Join->SetIsEnabled(true);
	}
}

void UMenu::OnJoinSessionCallback(EOnJoinSessionCompleteResult::Type JoinResult)
{
	//尝试进行ClientTravel
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString OutHostAddress;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, OutHostAddress);
			
			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(OutHostAddress, ETravelType::TRAVEL_Absolute);
			}
		}
	}
	if (JoinResult != EOnJoinSessionCompleteResult::Success)
	{
		Button_Join->SetIsEnabled(true);
	}
}

void UMenu::OnDestroySessionCallback(bool bResult)
{

}

void UMenu::OnStartSessionCallback(bool bResult)
{

}
