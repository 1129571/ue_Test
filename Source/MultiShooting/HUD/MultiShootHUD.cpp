// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootHUD.h"

void AMultiShootHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewPortSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewPortSize);
		const FVector2D ViewPortCenter = ViewPortSize / 2.f;

		float CrosshairOffSet = CrosshairSpreadMax * HUDPackage.CrosshairSpreadScale;

		if (HUDPackage.CrosshairCenter)
		{
			FVector2D Spread(0.f, 0.f);
			DrawCrosshair(HUDPackage.CrosshairCenter, ViewPortCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairLeft)
		{
			FVector2D Spread(-CrosshairOffSet, 0.f);
			DrawCrosshair(HUDPackage.CrosshairLeft, ViewPortCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairRight)
		{
			FVector2D Spread(CrosshairOffSet, 0.f);
			DrawCrosshair(HUDPackage.CrosshairRight, ViewPortCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairTop)
		{
			FVector2D Spread(0.f, -CrosshairOffSet);
			DrawCrosshair(HUDPackage.CrosshairTop, ViewPortCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairBottom)
		{
			FVector2D Spread(0.f, CrosshairOffSet);
			DrawCrosshair(HUDPackage.CrosshairBottom, ViewPortCenter, Spread, HUDPackage.CrosshairColor);
		}

	}
}

void AMultiShootHUD::DrawCrosshair(UTexture2D* InTexture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureW = InTexture->GetSizeX();
	const float TextureH = InTexture->GetSizeY();

	// Texture 默认左上角为(0,0), 我们希望使用中心作为绘制的点
	const FVector2D DrawPosition2D(
		ViewportCenter.X - TextureW / 2.f + Spread.X,		//添加准星浮动值
		ViewportCenter.Y - TextureH / 2.f + Spread.Y
	);

	DrawTexture(
		InTexture,
		DrawPosition2D.X,
		DrawPosition2D.Y,
		TextureW,
		TextureH,
		0.f,
		0.f,
		1.f,
		1.f,
		CrosshairColor
	);
}
