// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class MULTISHOOTING_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

	//拖尾烟雾特效(Niagara):子类的榴弹和火箭弹需要
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* SocketTrailSystem;			

	UPROPERTY()
	class UNiagaraComponent* SocketNiagaraComponent;

	void SpawnTrailSystem();


protected:
	virtual void BeginPlay() override;

	void ExplodeDamage(float MinDamage, float InnerRadius, float OuterRadius, float DamageFalloff);

	void StartDestroyTimer();

	void DestroyTimerFinished();

	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		FVector NormalImpulse, 
		const FHitResult& Hit
	);

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	//命中特效
	UPROPERTY(EditAnywhere, Category = Projectile)
	class UParticleSystem* ImpactParticle;	

	//命中音效
	UPROPERTY(EditAnywhere, Category = Projectile)
	class USoundCue* ImpactSound;		

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;		//子类的榴弹和火箭弹需要

private:
	//拖尾特效(旧)
	UPROPERTY(EditAnywhere, Category = Projectile)
	class UParticleSystem* Tracer;		

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	//我们希望Niagara特效不要在Actor OnHit后直接销毁, 而是等待一段时间, 将拖尾残留在场景中
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DelayDestroyTime = 3.f;

};
