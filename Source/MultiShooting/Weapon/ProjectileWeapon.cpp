// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	//仅希望在服务器生成子弹, 并通过Replicate将其复制到客户端
	if (!HasAuthority()) return;

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	//生成子弹, Mesh 资产上已经有名为"MuzzleFlash"的Socket, 并且它的方向是正确的(X 轴向前)
	const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleSocket)
	{
		FTransform SocketTransform = MuzzleSocket->GetSocketTransform(GetWeaponMesh());

		//枪口MuzzleFlash Socket 到射线检测位置的向量
		FVector Toarget = HitTarget - SocketTransform.GetLocation();
		//获取枪口到命中目标之间的旋转, 用于生成子弹初始朝向
		FRotator TargetRotation = Toarget.Rotation();

		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			//我们希望子弹的Owner是武器的Owner, 而武器的Owner在装备时SetOwner为Character
			//一般将其设置为生成它的Actor, 如: 子弹的Owner是武器, 但这里我们直接设置为武器的拥有者
			SpawnParams.Owner = GetOwner();
			//该Actor造成的伤害是属于哪个Pawn
			SpawnParams.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
				);
			}
		}
	}
}
