// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 扫描武器类(没有实际的发射物Actor), 如刀 手枪
 */
UCLASS()
class MULTISHOOTING_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	//传入射线起点和终点, 返回一个射线终点周围(球范围内)带有随机偏移的点
	FVector TragetEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);

private:
	UPROPERTY(EditAnywhere)
	float HitDamage = 20.f;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;		//子弹击中特效

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;				//子弹烟雾拖尾

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;				//开火枪口闪光(部分武器没有对应动画, 通过这种方式实现)
	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;						//开火音效(部分武器没有对应动画, 通过这种方式实现)

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;						//开火击中音效

	/**
	* TragetEndWithScatter所需的一些参数
	*/
	UPROPERTY(EditAnywhere, Category = WeaponScatter)
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = WeaponScatter)
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere, Category = WeaponScatter)
	bool bUseScatter = false;		//是否使用散弹?


};
