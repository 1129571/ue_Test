// Fill out your copyright notice in the Description page of Project Settings.


#include "ShotGun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Characters/MultiShootCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotGun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = Cast<AController>(OwnerPawn->GetController());

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		//散弹终点
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FVector End = TragetEndWithScatter(Start, HitTarget);
		}
	}
}
