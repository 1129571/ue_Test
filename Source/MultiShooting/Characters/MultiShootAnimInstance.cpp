// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootAnimInstance.h"
#include "MultiShootCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

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

	bWeaponEquipped = MultiShootCharacter->IsWeaponEquipped();

	bIsCrouched = MultiShootCharacter->bIsCrouched;

	bIsAiming = MultiShootCharacter->IsAiming();

	EquippedWeapon = MultiShootCharacter->GetEquippedWeapon();

	//获取到Pawn的基础旋转(鼠标旋转可以观察Pawn), 表示 [玩家] 正在看的方向
	// 是类似世界坐标的全局旋转, 像东南西北一样有固定方向
	FRotator AimRotation = MultiShootCharacter->GetBaseAimRotation();
	// 根据Pawn速度获取到当前移动方向的旋转, 表示 [玩家角色] 正在移动的方向
	// 也是类似世界坐标的全局旋转
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(MultiShootCharacter->GetVelocity());
	//以移动方向, 获取旋转的差量
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	//通过这种方式可以防止从-180直接跳到180, 从而出现"抽搐"
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	// Lean 表示角色的左右偏移
	CharacterRotationLastFrame = CharacterRotationThisFrame;
	CharacterRotationThisFrame = MultiShootCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotationThisFrame, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = MultiShootCharacter->GetAO_Yaw();
	AO_Pitch = MultiShootCharacter->GetAO_Pitch();

	//CharacterMesh---WeaponMesh之间进行IK绑定
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && MultiShootCharacter->GetMesh())
	{
		//获取武器Mesh上Socket(通过命名获取)的世界变换
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		//计算Socket变换在目标(Character)骨骼空间上的位置和旋转
		FVector OutPosition;
		FRotator OutRotation;
		//这里使用hand_r作为参考骨骼是因为我们将武器附加到这个骨骼的, 它和武器之间相对位置是不会变的
		MultiShootCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}