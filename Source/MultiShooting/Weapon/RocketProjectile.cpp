// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "MultiShootComponents/RocketMovementComponent.h"

ARocketProjectile::ARocketProjectile()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//子弹的旋转跟随速度方向, 如应用重力时速度回向下呈抛物线, 弹头方向会每帧更新和速度保持一致
	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);

	RocketMovementComponent->InitialSpeed = 1500.f;
	RocketMovementComponent->MaxSpeed = 1500.f;
}

void ARocketProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		//该Projectile具有模型, 因此除了在服务器应用伤害外, 还有处理模型的隐藏等(客户端和服务器)
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}

	SpawnTrailSystem();

	if (SocketLoopSound && SocketLoopAttenuation)
	{
		SocketLoopComponent = UGameplayStatics::SpawnSoundAttached(
			SocketLoopSound,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			SocketLoopAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		// 我们如果击中了自己, Rocket 并不会停止运动, 而是继续移动直至碰撞爆炸(在RocketMovementComponent中实现)
		return;
	}

	ExplodeDamage(MinDamage, InnerRadius, OuterRadius, DamageFalloff);

	//父类是在Destroyed时播放特效和音效, 我们希望RocketProjectile可以在OnHit后延迟销毁, 所以需要再碰撞时就播放
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticle,
			GetActorTransform()
		);
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	//隐藏模型, 禁用碰撞
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	//停止继续产生Niagara粒子
	if (SocketNiagaraComponent && SocketNiagaraComponent->GetSystemInstance())
	{
		SocketNiagaraComponent->GetSystemInstance()->Deactivate();
	}
	//飞行中音效
	if (SocketLoopComponent && SocketLoopComponent->IsPlaying())
	{
		SocketLoopComponent->Stop();
	}

	StartDestroyTimer();
}
