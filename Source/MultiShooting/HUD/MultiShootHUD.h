// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MultiShootHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D* CrosshairCenter;
	UTexture2D* CrosshairLeft;
	UTexture2D* CrosshairRight;
	UTexture2D* CrosshairTop;
	UTexture2D* CrosshairBottom;

	float CrosshairSpreadScale;			// 准星浮动基础缩放值, 多种因素影响

	FLinearColor CrosshairColor;		// 本例中, 实现了CrosshairInterface接口的对象我们显示红色, 否则为白色
};

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API AMultiShootHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	//逐帧调用绘制到屏幕
	virtual void DrawHUD() override;

private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* InTexture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax;		// 暴露到蓝图, 便于缩放结构体中的准星最大浮动值

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& InHUDPackage) { HUDPackage = InHUDPackage; }
};
