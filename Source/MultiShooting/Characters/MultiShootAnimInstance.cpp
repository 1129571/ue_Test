// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiShootAnimInstance.h"
#include "MultiShootCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"
#include "MultiShootTypes/CombatState.h"

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

	TurningInPlace = MultiShootCharacter->GetTurningInPlace();

	bRotateRootBone = MultiShootCharacter->ShouldRtateRootBone();

	bElimmed = MultiShootCharacter->IsElimmed();

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

		/*射击射击目标(屏幕中心射出)和动画(枪口射出)差距过大, 希望通过持枪手的动画修正*/
		if (MultiShootCharacter->IsLocallyControlled())		// 只是美观, 非自己控制的角色没有必要计算
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = MultiShootCharacter->GetMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
			// 射击目标从远处变为近处时持枪手动画会瞬间跳转, 这里加了平滑
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(MultiShootCharacter->GetHitTarget(), RightHandTransform.GetLocation());
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	}

	// Reload 时就不应该继续使用IK
	bUseFABRIK = MultiShootCharacter->GetCombatState() != ECombatState::ECS_Reloading;
	// Reload && Cooldown状态 时不要使用AimOffset
	bUseAimOffset = MultiShootCharacter->GetCombatState() != ECombatState::ECS_Reloading && !MultiShootCharacter->GetDisableGameplay();
	// Reload && Cooldown状态 时不要使用RightHand旋转骨骼
	bFixRightHand = MultiShootCharacter->GetCombatState() != ECombatState::ECS_Reloading && !MultiShootCharacter->GetDisableGameplay();
}