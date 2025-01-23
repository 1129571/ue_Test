// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Characters/MultiShootCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponTypes.h"

#include "DrawDebugHelpers.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = Cast<AController>(OwnerPawn->GetController());

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		//希望(如散弹)可以把伤害一次性计算后Apply
		AMultiShootCharacter* MultiShootCharacter = Cast<AMultiShootCharacter>(FireHit.GetActor());
		if (MultiShootCharacter && HasAuthority() && InstigatorController)
		{
			UGameplayStatics::ApplyDamage(
				MultiShootCharacter,
				HitDamage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);
		}

		//Todo : 不同部位的击中特效有区别
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}

		//Todo: 不同质地的击中音效有区别
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}

		//部分武器没有对应动画, 通过这种方式实现枪口闪光 开火音效
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTransform
			);
		}
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

FVector AHitScanWeapon::TargetEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	//随机方向随机长度(球体半径内)的向量
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, SphereRadius);
	//在DistanceToSphere距离时的随机射线End点
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	FVector ResultEnd = (TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());

//	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Green, true);			//调试球
//	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Red, true);						//调试球内随机点
//	DrawDebugLine(GetWorld(), TraceStart, ResultEnd, FColor::Yellow, true);					//调试目标线段

	// 实际的射击距离是TRACE_LENGTH
	return ResultEnd;
}

// (射击)射线检测 + 烟雾拖尾
void AHitScanWeapon::WeaponTraceHit(const FVector& InStart, const FVector& InHitTarget, FHitResult& OutHitResult)
{
	UWorld* World = GetWorld();
	if (World)
	{
		// *1.25是为了防止射线刚刚好时, 有可能会检测不到
		FVector End = bUseScatter ? TargetEndWithScatter(InStart, InHitTarget) : InStart + (InHitTarget - InStart) * 1.25f;
		World->LineTraceSingleByChannel(
			OutHitResult,
			InStart,
			End,
			ECollisionChannel::ECC_Visibility
		);		

		FVector BeamEnd = End;
		if (OutHitResult.bBlockingHit)
		{
			BeamEnd = OutHitResult.ImpactPoint;
		}
		if (BeamParticles)
		{
			UParticleSystemComponent* BeamParticleComponent = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				InStart,
				FRotator::ZeroRotator,
				true
			); 
			if (BeamParticleComponent)
			{
				BeamParticleComponent->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}
