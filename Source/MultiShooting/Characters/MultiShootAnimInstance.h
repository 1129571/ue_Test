// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
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

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	bool bIsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;		//是否正在加速
};
