// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

ABulletProjectile::ABulletProjectile()
{
	//发射物移动组件, 不需要附加根组件
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	//子弹的旋转跟随速度方向, 如应用重力时速度回向下呈抛物线, 弹头方向会每帧更新和速度保持一致
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);

	ProjectileMovementComponent->InitialSpeed = 15000.f;
	ProjectileMovementComponent->MaxSpeed = 15000.f;

}

void ABulletProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}

	//父类中进行Destroy, 所以应该在最后调用
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}