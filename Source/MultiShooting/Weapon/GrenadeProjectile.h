// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "GrenadeProjectile.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API AGrenadeProjectile : public AProjectile
{
	GENERATED_BODY()

public:
	AGrenadeProjectile();
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	UPROPERTY(EditDefaultsOnly)
	float InnerRadius = 10.f;
	UPROPERTY(EditDefaultsOnly)
	float OuterRadius = 500.f;
	UPROPERTY(EditDefaultsOnly)
	float MinDamage = 200.f;
	UPROPERTY(EditDefaultsOnly)
	float DamageFalloff = 1.f;		//伤害衰减指数

private:
	//反弹音效
	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;
};
