// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool InbAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool InbAiming);

	UFUNCTION()
	void onRep_EquippedWeapon();

	void WeaponFire(bool bFire);

	UFUNCTION(NetMulticast, Reliable)
	void MultiCastFire(const FVector_NetQuantize& InHitTarget);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& InHitTarget);

	//命中目标点预测
	void TraceUnderCrosshairs(FHitResult& TraceResult);

	void SetHUDCrossHairs(float DeltaTime);

private:
	class AMultiShootCharacter* OwnedCharacter;
	class AMultiShootPlayerController* Controller;
	class AMultiShootHUD* HUD;

	UPROPERTY(ReplicatedUsing = onRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bIsAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;	

	bool bFireState = false;

	float CrosshairVelocityFactor;			//准星散开的速度因子
	float CrosshairInAirFactor;				//准星散开的坠落因子
};
