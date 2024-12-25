// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootAnimInstance.h"
#include "MultiShootCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMultiShootAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	MultiShootCharacter = Cast<AMultiShootCharacter>(TryGetPawnOwner());
}

void UMultiShootAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (MultiShootCharacter == nullptr)
	{
		MultiShootCharacter = Cast<AMultiShootCharacter>(TryGetPawnOwner());
	}

	if (MultiShootCharacter == nullptr)return;

	FVector Velocity = MultiShootCharacter->GetVelocity();
	Velocity.Z = 0.f;					//只需要水平面上的速度
	Speed = Velocity.Size();			//向量长度

	bIsFalling = MultiShootCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = MultiShootCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

}
