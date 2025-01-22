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

	void DestroyTimerFinished();

	virtual void Destroyed() override;

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
	class UNiagaraSystem* SocketTrailSystem;

	UPROPERTY()
	class UNiagaraComponent* SocketNiagaraComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* SocketLoopSound;				//飞行途中的音效

	UPROPERTY(EditAnywhere)
	USoundAttenuation* SocketLoopAttenuation;

	UPROPERTY()
	class UAudioComponent* SocketLoopComponent;
private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	//我们希望Niagara特效不要在Actor OnHit后直接销毁, 而是等待一段时间, 将拖尾残留在场景中
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DelayDestroyTime = 3.f;
};
