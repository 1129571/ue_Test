// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Weapon/Weapon.h"
#include "Characters/MultiShootCharacter.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 400.f;
}

void UCombatComponent::EquipWeaponFun(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr || OwnedCharacter == nullptr) return;
	//设置已装备的武器
	EquippedWeapon = WeaponToEquip;
	//更新武器状态
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	//Weapon附加到Character模型手上
	const USkeletalMeshSocket* HandSocket = OwnedCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, OwnedCharacter->GetMesh());
	}
	// 武器的Owner设置为OwnedCharacter, 确保可以正确复制Weapon对象
	// 通常用来标识某个 Actor 的控制者或拥有者。
	// 通过设置所有者，可以方便地管理与该 Actor 相关的逻辑，比如权限、所有权、状态同步等。
	EquippedWeapon->SetOwner(OwnedCharacter);

	//持有武器时不希望朝向运动方向(此时只发生在服务器, 还需要本地设置)
	OwnedCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	OwnedCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (OwnedCharacter)
	{
		OwnedCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::SetAiming(bool InbAiming)
{
	if (OwnedCharacter && EquippedWeapon)
	{
		//这里不进行是否在服务器的检查, 因为
		//服务器拥有的Character不会执行ServerSetAiming
		//客户端拥有的Character会执行ServerSetAiming覆盖上一行赋值
		//具体参考官方文档RPC部分
		bIsAiming = InbAiming;
		OwnedCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
		ServerSetAiming(InbAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool InbAiming)
{
	if (OwnedCharacter && EquippedWeapon)
	{
		bIsAiming = InbAiming;
		OwnedCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::onRep_EquippedWeapon()
{
 	if (OwnedCharacter && EquippedWeapon)
 	{
		//持有武器时不希望朝向运动方向(此处是来自服务器的Replicated通知)
		OwnedCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		OwnedCharacter->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

