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

		if (HUDPackage.CrosshairCenter)
		{
			DrawCrosshair(HUDPackage.CrosshairCenter, ViewPortCenter);
		}
		if (HUDPackage.CrosshairLeft)
		{
			DrawCrosshair(HUDPackage.CrosshairLeft, ViewPortCenter);
		}
		if (HUDPackage.CrosshairRight)
		{
			DrawCrosshair(HUDPackage.CrosshairRight, ViewPortCenter);
		}
		if (HUDPackage.CrosshairTop)
		{
			DrawCrosshair(HUDPackage.CrosshairTop, ViewPortCenter);
		}
		if (HUDPackage.CrosshairBottom)
		{
			DrawCrosshair(HUDPackage.CrosshairBottom, ViewPortCenter);
		}

	}
}

void AMultiShootHUD::DrawCrosshair(UTexture2D* InTexture, FVector2D ViewportCenter)
{
	const float TextureW = InTexture->GetSizeX();
	const float TextureH = InTexture->GetSizeY();

	//Texture默认左上角为(0,0), 我们希望使用中心作为绘制的点
	const FVector2D DrawPosition2D(
		ViewportCenter.X - TextureW / 2.f,
		ViewportCenter.Y - TextureH / 2.f
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
		FLinearColor::Red
	);
}
