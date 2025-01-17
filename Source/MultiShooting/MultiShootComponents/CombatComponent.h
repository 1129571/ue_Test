// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/MultiShootHUD.h"
#include "Weapon/WeaponTypes.h"
#include "MultiShootTypes/CombatState.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTISHOOTING_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent(); 
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	friend class AMultiShootCharacter;
	void EquipWeaponFun(class AWeapon* WeaponToEquip);
	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishedReloading();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool InbAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool InbAiming);

	UFUNCTION()
	void onRep_EquippedWeapon();

	void WeaponFire(bool bFire);

	void Fire();

	UFUNCTION(NetMulticast, Reliable)
	void MultiCastFire(const FVector_NetQuantize& InHitTarget);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& InHitTarget);

	//命中目标点预测
	void TraceUnderCrosshairs(FHitResult& TraceResult);

	void SetHUDCrossHairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	/**
	 * 换弹相关
	 */
	void HandleReload();

	int32 AmountToReload();

private:
	UPROPERTY()
	class AMultiShootCharacter* OwnedCharacter;
	UPROPERTY()
	class AMultiShootPlayerController* Controller;
	UPROPERTY()
	class AMultiShootHUD* HUD;

	UPROPERTY(ReplicatedUsing = onRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bIsAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;	

	bool bFireButtonPressed = false;

	FHUDPackage HUDPackage;

	/* HUD 准星*/
	float CrosshairVelocityFactor;			//准星散开的速度因子
	float CrosshairInAirFactor;				//准星散开的坠落因子
	float CrosshairAimFactor;				//准星散开的瞄准因子
	float CrosshairShootingFactor;			//准星散开的射击因子

	FVector HitTarget;

	/* 
	* FOV 相关
	*/
	float DefaultFOV;
	UPROPERTY(EditAnywhere, Category = "Combat|FOV")
	float ZoomedFOV = 30.f;
	UPROPERTY(EditAnywhere, Category = "Combat|FOV")
	float ZoomInterpSpeed = 20.f;		// FOV 回归正常的速度, 这里我认为他和武器无关
	float CurrnetFOV;

	void InterpFOV(float DeltaTime);

	/*
	* 自动开火相关
	*/

	bool bCanFire = true;		// 是否正在开火, 防止频繁进入自动开火定时器

	FTimerHandle AutoFireTimer;

	void StartAutoFireTimer();
	void AutoFireTimerFinished();

	bool CanFire();

	/**
	 * 携带弹药
	 */

	//为当前装备的武器所携带的弹药数量
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;		//武器类型--携带弹药数量, 由于Hash特性, TMap无法复制
	UPROPERTY(EditAnywhere)
	int32 StartingAmmo = 30;
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();
};
