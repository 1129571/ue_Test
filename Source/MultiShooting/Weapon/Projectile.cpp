// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Characters/MultiShootCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "MultiShooting.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	// 发射物的根组件必须支持碰撞。通常，发射物的根组件应该是一个带碰撞的组件，例如Sphere Component或Capsule Component，而不是Scene Component。
	//	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	//碰撞设置
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	//使用自定义通道, 对Character模型检测
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	//生成粒子特效
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	//绑定命中事件
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}
}

//处理径向伤害 : 火箭弹和榴弹等
void AProjectile::ExplodeDamage(float MinDamage, float InnerRadius, float OuterRadius, float DamageFalloff)
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
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &ThisClass::DestroyTimerFinished, DelayDestroyTime);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

//仅在服务器绑定
void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//销毁自身, Projectile 是复制的, 因此可以利用它的变化让客户端也播放命中音效和特效
	Destroy();
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	//命中特效
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticle,
			GetActorTransform()
		);
	}
	//命中音效 
	// Todo: 命中不同音效不同
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::SpawnTrailSystem()
{
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
}

