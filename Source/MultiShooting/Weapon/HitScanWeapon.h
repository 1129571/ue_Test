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
	FVector TargetEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);

	void WeaponTraceHit(const FVector& InStart, const FVector& InHitTarget, FHitResult& OutHitResult);

	//开火击中音效
	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon")
	USoundCue* HitSound;

	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon")
	float HitDamage = 20.f;

	//子弹击中特效
	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon")
	class UParticleSystem* ImpactParticles;

private:
	//子弹烟雾拖尾
	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon")
	UParticleSystem* BeamParticles;

	//开火枪口闪光(部分武器没有对应动画, 通过这种方式实现)
	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon")
	UParticleSystem* MuzzleFlash;	
	//开火音效(部分武器没有对应动画, 通过这种方式实现)
	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon")
	USoundCue* FireSound;	
					

	/**
	* TragetEndWithScatter所需的一些参数
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon|Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon|Scatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|HitScanWeapon|Scatter")
	bool bUseScatter = false;		//是否使用散弹?


};
