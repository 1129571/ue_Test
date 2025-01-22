// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 近扫描武器类, 如刀 手枪
 */
UCLASS()
class MULTISHOOTING_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual void Fire(const FVector& HitTarget) override;

private:
	UPROPERTY(EditAnywhere)
	float HitDistance = 1.25f;

	UPROPERTY(EditAnywhere)
	float HitDamage = 20.f;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;
};
