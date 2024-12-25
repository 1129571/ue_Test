// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverHeadWidget.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API UOverHeadWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DisplayText;

	void SetDisplayText(FString InText);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);

protected:
	//前往其他关卡时执行该函数
	// UE5.1之前叫OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
	virtual void NativeDestruct() override;
};
