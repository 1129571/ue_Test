// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Weapon/Weapon.h"
#include "Characters/MultiShootCharacter.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

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
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
}


void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

