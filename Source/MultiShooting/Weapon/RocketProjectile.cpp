// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"

ARocketProjectile::ARocketProjectile()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARocketProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		//该Projectile具有模型, 因此处理在服务器应用伤害外, 还有处理模型的隐藏等(客户端和服务器)
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}

	if (SocketTrailSystem)
	{
		SocketNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			SocketTrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
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

void ARocketProjectile::DestroyTimerFinished()
{
	Destroy();
}

void ARocketProjectile::Destroyed()
{
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//拥有爆炸伤害的发射物
	APawn* FirePawn = GetInstigator();		//生成发射物Actor时设置的
	if (FirePawn && HasAuthority())
	{
		AController* FireController = FirePawn->GetController();
		if (FireController)
		{
			//应用径向伤害 : InnerRadius内受到全额伤害BaseDamage, OuterRadius受到最小伤害, 两者之间受到DamageFalloff指数衰减的伤害
			//如不希望伤害衰减, 可以使用 bFullDamage = true
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				MinDamage,
				GetActorLocation(),
				InnerRadius,
				OuterRadius,
				DamageFalloff,
				UDamageType::StaticClass(),
				TArray<AActor*>(),				//忽略的Actor数组
				this,							//造成伤害的Actor
				FireController					//造成伤害的Actor控制器
			);
		}
	}

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
	if (RocketMesh)
	{
		RocketMesh->SetVisibility(false);
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

	GetWorldTimerManager().SetTimer(DestroyTimer, this, &ThisClass::DestroyTimerFinished, DelayDestroyTime);
}
