

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MultiShootTypes/TurningInPlace.h"
#include "MultiShootCharacter.generated.h"

UCLASS()
class MULTISHOOTING_API AMultiShootCharacter : public ACharacter
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
	void AimOffset(float DeltaTime);
	virtual void Jump() override;

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

public:	
	void SetOverlappingWeapon(AWeapon* InWeapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	AWeapon* GetEquippedWeapon();
	
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FVector GetHitTarget() const;
};
