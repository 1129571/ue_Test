// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTISHOOTING_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent(); 
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	friend class AMultiShootCharacter;

	//我们希望准备武器发生在服务器
	void EquipWeaponFun(class AWeapon* WeaponToEquip);

protected:
	virtual void BeginPlay() override;

private:
	class AMultiShootCharacter* OwnedCharacter;
	class AWeapon* EquippedWeapon;
		
};
