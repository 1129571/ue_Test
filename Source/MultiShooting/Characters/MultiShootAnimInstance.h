// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MultiShootTypes/TurningInPlace.h"
#include "MultiShootAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class MULTISHOOTING_API UMultiShootAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	//只调用一次, 类似BeginPlay
	virtual void NativeInitializeAnimation() override;
	//每帧调用, 类似Tick, 动画蓝图的 事件蓝图更新动画 就是通过它
	virtual void NativeUpdateAnimation(float DeltaTime) override;

private:
	//私有变量只有在meta = (AllowPrivateAccess = "true")才能被BlueprintReadOnly访问到
	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	class AMultiShootCharacter* MultiShootCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bIsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;		//是否正在加速

	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bWeaponEquipped;

	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bIsCrouched;

	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bIsAiming;

	//驱动混合空间
	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float YawOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float Lean;

	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotationThisFrame;
	FRotator DeltaRotation;	
	
	//驱动瞄准偏移
	UPROPERTY(BlueprintReadOnly, Category = "AimOffset", meta = (AllowPrivateAccess = "true"))
	float AO_Yaw;

	UPROPERTY(BlueprintReadOnly, Category = "AimOffset", meta = (AllowPrivateAccess = "true"))
	float AO_Pitch;

	//进行IK反向运动学解算
	UPROPERTY(BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	class AWeapon* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	//AimOffset我们希望只在-90,90使用, 超出时自动转身
	UPROPERTY(BlueprintReadOnly, Category = "TurningInPlace", meta = (AllowPrivateAccess = "true"))
	ETurningInPlace TurningInPlace;
};
