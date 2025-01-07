// Fill out your copyright notice in the Description page of Project Settings.

//Todo: 弹壳音效和粒子

#include "Casing.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
}

