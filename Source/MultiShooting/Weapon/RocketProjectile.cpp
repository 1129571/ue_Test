// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketProjectile.h"
#include "Kismet/GameplayStatics.h"

ARocketProjectile::ARocketProjectile()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//拥有爆炸伤害的发射物
	APawn* FirePawn = GetInstigator();		//生成发射物Actor时设置的
	if (FirePawn)
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

	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
