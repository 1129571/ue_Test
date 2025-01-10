

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MultiShootTypes/TurningInPlace.h"
#include "Interface/CrosshairInterface.h"
#include "MultiShootCharacter.generated.h"

UCLASS()
class MULTISHOOTING_API AMultiShootCharacter : public ACharacter, public ICrosshairInterface
{
	GENERATED_BODY()

public:
	AMultiShootCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	//注册需要复制的变量的函数
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//在所有组件被初始化并且基本属性设置好后调用. 构造之后, BeginPlay之前
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitMontage();

	//Actor 运动发生变化时调用
	virtual void OnRep_ReplicatedMovement() override;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void Turn(float AxisValue);
	void LookUp(float AxisValue);
	void EquipWeaponPressed();
	void CrouchPressed();
	void AimPressed();
	void AimReleased();
	void FirePressed();
	void FireReleased();
	void AimOffset(float DeltaTime);		//处理本地控制--动画每帧执行, 旋转根骨骼效果很好
	void CalculateAO_Pitch();
	void SimProxiesTurn();					//处理模拟角色--动画执行频率低, 旋转根骨骼会导致动画不流畅
	virtual void Jump() override;
	void PlayHitReactMontage();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverHeadWidget;

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	// 标记为需要复制的变量
	UPROPERTY(ReplicatedUsing = onRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartAimRotation;

	UPROPERTY(EditAnywhere, Category=Combat)
	class UAnimMontage* WeaponFireMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	//用于在AimOffset(-90, 90)超出时原地转身
	ETurningInPlace TurningInPlace;
	void TurnInPlaceFun(float DeltaTime);

	//定义一个无返回值的函数,作为复制变量改变使调用的函数
	//注意 : 需要在复制变量的使用 UPROPERTY(ReplicatedUsing = 本函数名称) 标记
	UFUNCTION()
	void onRep_OverlappingWeapon(AWeapon* LastWeapon);

	// 该函数只能客户端调用, 服务器执行. 
	// 并且是可靠的调用, 不会因为网络不稳定而丢失
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon();

	//角色距离摄像机太近时, 将其隐藏(该行为只是为了本地玩家视觉效果, 因此只发生在本地机器)
	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	//最初我们通过旋转根骨骼解决转身和AimOffset, 但是它在非本地控制角色上很卡顿
	//因为本地控制角色的动画蓝图执行频率比非本地控制角色的动画蓝图执行频率高
	//所以我们对非本地控制角色采用不同的处理方法
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

public:	
	void SetOverlappingWeapon(AWeapon* InWeapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	AWeapon* GetEquippedWeapon();
	
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRtateRootBone() const { return bRotateRootBone; }
	FVector GetHitTarget() const;
};
