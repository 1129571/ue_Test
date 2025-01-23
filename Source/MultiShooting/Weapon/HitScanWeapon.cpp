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
		// *1.25是为了防止射线刚刚好时, 有可能会检测不到
		FVector End = Start + (HitTarget - Start) * 1.25f;

		FHitResult FireHit;
		UWorld* World = GetWorld();
		if (World)
		{
			World->LineTraceSingleByChannel(
				FireHit,
				Start,
				End,
				ECollisionChannel::ECC_Visibility
			);

			FVector BeamEnd = End;

			if (FireHit.bBlockingHit)
			{
				/************/ // 击中自己的直接return?
				if (FireHit.GetActor() == GetOwner()) return;
				/************/
				BeamEnd = FireHit.ImpactPoint;

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

				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						World,
						ImpactParticles,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
					);
				}

				//击中音效
				if (HitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						HitSound,
						FireHit.ImpactPoint
					);
				}
			}

			if (BeamParticles)
			{
				UParticleSystemComponent* BeamParticleComponent = UGameplayStatics::SpawnEmitterAtLocation(
					World,
					BeamParticles,
					SocketTransform
					);
				if (BeamParticleComponent)
				{
					BeamParticleComponent->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}

		}
		//部分武器没有对应动画, 通过这种方式实现枪口闪光 开火音效
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
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

FVector AHitScanWeapon::TragetEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	//随机方向随机长度(球体半径内)的向量
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, SphereRadius);
	//在DistanceToSphere距离时的随机射线End点
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	FVector ResultEnd = (TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());

	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);			//调试球
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);						//调试球内随机点
	DrawDebugLine(GetWorld(), TraceStart, ResultEnd, FColor::Blue, true);					//调试目标线段

	// 实际的射击距离是TRACE_LENGTH
	return ResultEnd;
}
