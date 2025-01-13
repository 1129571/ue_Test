// Fill out your copyright notice in the Description page of Project Settings.

// 不需要复制, 仅作为本地机器的视觉效果

#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/LatentActionManager.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	//防止弹壳模型对SpringArm阻挡
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	//启用物理模拟和重力
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);

	//蓝图 : Simulation generates hit events
	CasingMesh->SetNotifyRigidBodyCollision(true);

	//默认的冲量乘数
	ShellEjectionImpulse = 5.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound && bFirstHit)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	bFirstHit = false;

	GetWorld()->GetTimerManager().SetTimer(DestroyTimeHandle, this, &ThisClass::DestroyFun, 1.5f, false);
}

void ACasing::DestroyFun()
{
	Destroy();
}

void ACasing::Destroyed()
{
	Super::Destroyed();
	GetWorldTimerManager().ClearTimer(DestroyTimeHandle);
}