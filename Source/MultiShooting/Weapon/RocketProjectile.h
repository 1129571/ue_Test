// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "RocketProjectile.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API ARocketProjectile : public AProjectile
{
	GENERATED_BODY()

public:
	ARocketProjectile();
	
protected:
	virtual void BeginPlay() override;

	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	) override;

	UPROPERTY(EditDefaultsOnly)
	float InnerRadius = 10.f;
	UPROPERTY(EditDefaultsOnly)
	float OuterRadius = 500.f;
	UPROPERTY(EditDefaultsOnly)
	float MinDamage = 200.f;
	UPROPERTY(EditDefaultsOnly)
	float DamageFalloff = 1.f;		//伤害衰减指数

	UPROPERTY(EditAnywhere)
	USoundCue* SocketLoopSound;				//飞行途中的音效

	UPROPERTY(EditAnywhere)
	USoundAttenuation* SocketLoopAttenuation;

	UPROPERTY()
	class UAudioComponent* SocketLoopComponent;

	UPROPERTY(VisibleAnywhere)
	class URocketMovementComponent* RocketMovementComponent;
};
