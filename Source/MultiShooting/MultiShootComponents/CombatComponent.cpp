// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Weapon/Weapon.h"
#include "Characters/MultiShootCharacter.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController/MultiShootPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 400.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (OwnedCharacter)
	{
		OwnedCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (OwnedCharacter->GetFollowCamera())
		{
			DefaultFOV = OwnedCharacter->GetFollowCamera()->FieldOfView;
			CurrnetFOV = DefaultFOV;
		}

		if (OwnedCharacter->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 仅本地控制的角色需要执行(提高沉浸式, 避免资源浪费)
	if (OwnedCharacter && OwnedCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		//HUD更新
		SetHUDCrossHairs(DeltaTime);

		//FOV更新
		InterpFOV(DeltaTime);
	}

}

void UCombatComponent::EquipWeaponFun(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr || OwnedCharacter == nullptr) return;

	//防止同时装备多把武器
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

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
	EquippedWeapon->SetHUDWeaponAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(OwnedCharacter->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmoAmount(CarriedAmmo);
	}

	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, OwnedCharacter->GetActorLocation());
	}

	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}

	//持有武器时不希望朝向运动方向(此时只发生在服务器, 还需要本地设置)
	OwnedCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	OwnedCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::onRep_EquippedWeapon()
{

	if (OwnedCharacter && EquippedWeapon)
	{
		//为什么还要在客户端执行将Weapon附加到Socket?
		//因为武器掉落会启用模拟物理, 而模拟物理会和AttachActor冲突
		//我们无法保证掉落后立即拾取到低是模拟物理先执行到客户端, 还是AttachActor先执行到客户端(取决于网络)
		//以防万一, 希望一定能AttachActor, 所以客户端在EquippedWeapon复制后再次执行AttachActor
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = OwnedCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, OwnedCharacter->GetMesh());
		}

		//持有武器时不希望朝向运动方向(此处是来自服务器的Replicated通知)
		OwnedCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		OwnedCharacter->bUseControllerRotationYaw = true;

		if (EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, OwnedCharacter->GetActorLocation());
		}
	}
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

//Client 调用 会在Server执行, Server 调用 会在Server执行
void UCombatComponent::ServerReload_Implementation()
{
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::HandleReload()
{
	//Character播放Reload动画
	OwnedCharacter->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;

	int32 RoomInMag = EquippedWeapon->GetReloadNeedAmmo();						//弹夹内需要的数量
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmmoCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];	//携带的弹药数量
		int32 Least = FMath::Min(RoomInMag, AmmoCarried);						//实际可换的数量
		return FMath::Clamp(RoomInMag, 0, Least);
	}

	return 0;
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		//换弹完成, 继续开火(客户端变量被复制时)
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	}
}

void UCombatComponent::FinishedReloading()
{
	if (OwnedCharacter == nullptr) return;
	if (OwnedCharacter->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	//换弹完成, 继续开火, CombatState是复制的, 所以还需要再OnRep处理客户端被复制的人逻辑
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (OwnedCharacter == nullptr && EquippedWeapon == nullptr) return;

	//实际可换弹的数量
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		//用于复制, 表示当前武器的携带弹药数量
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(OwnedCharacter->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmoAmount(CarriedAmmo);
	}

	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, OwnedCharacter->GetActorLocation());
	}

	EquippedWeapon->AddAmmo(-ReloadAmount);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
	//只有Owner关心这个数据,因为需要显示到HUD, 其他可以不用复制, 避免额外网络占用
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
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

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(OwnedCharacter->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmoAmount(CarriedAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingSocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSubMachineGunAmmo);
}

//本地机器执行
void UCombatComponent::WeaponFire(bool bFire)
{
	if (EquippedWeapon == nullptr) return;
	bFireButtonPressed = bFire;

	//本地机器ServerRpc---服务器MulticaseRpc--->所有客户端
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		// Tick 中进行了击中目标检测, 并进行 HitTarget = TraceResult.ImpactPoint
		// TraceResult.ImpactPoint 已经是FVector_NetQuantize类型了
		ServerFire(HitTarget);
		//相当于立即响应一次后开始执行定时器
		StartAutoFireTimer();

		//开火时本地机器增加射击因子(用于HUD散开程度)
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 2.f;
		}
	}
}

void UCombatComponent::StartAutoFireTimer()
{
	if (EquippedWeapon == nullptr || OwnedCharacter == nullptr) return;
	//不循环, 延迟后调用该函数
	OwnedCharacter->GetWorldTimerManager().SetTimer(
		AutoFireTimer,
		this,
		&ThisClass::AutoFireTimerFinished,
		EquippedWeapon->AutoFireDelay
	);
}

void UCombatComponent::AutoFireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bCanAutoFire)
	{
		Fire();
	}
	//自动换弹
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;

	bool bResult =
		!EquippedWeapon->IsEmpty() &&
		bCanFire &&
		CombatState == ECombatState::ECS_Unoccupied;

	return bResult;
}

//服务器执行
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& InHitTarget)
{
	MultiCastFire(InHitTarget);
}

//所有客户端执行
void UCombatComponent::MultiCastFire_Implementation(const FVector_NetQuantize& InHitTarget)
{
	if (EquippedWeapon == nullptr) return;

	//保证Fire动画不会打断Reload1动画(自动武器Reload过程持续按住左键时)
	if (OwnedCharacter && CombatState == ECombatState::ECS_Unoccupied)
	{
		//Character和Weapon各自处理开火逻辑
		OwnedCharacter->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(InHitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceResult)
{
	//从屏幕中心发射射线检测.
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// 或视口中心FVector2D, 将2D屏幕空间坐标转换为3D世界空间坐标
	FVector2D CrosshairLocation = ViewportSize / 2.f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		// 直接用会有BUG: 实际检测位置是在 屏幕场景位置--控制的Character之间, 
		// 这就会将我们自己的角色或者Chracter身后的角色检测到, 这不合理
		// 解决办法是将射线检测的起点移动至我们的角色身体前侧
		if (OwnedCharacter)
		{
			float DistanceToCharacter = (OwnedCharacter->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 30.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		//转换失败, 就将子弹目标点设置为射线终点
		if (!TraceResult.bBlockingHit)
		{
			TraceResult.ImpactPoint = End;
		}

		//检测到的对象实现了CrosshairInterface接口(准星变红色)
		if (TraceResult.GetActor() && TraceResult.GetActor()->Implements<UCrosshairInterface>())
		{
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrossHairs(float DeltaTime)
{
	if (!OwnedCharacter || OwnedCharacter->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<AMultiShootPlayerController>(OwnedCharacter->Controller) : Controller;

	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<AMultiShootHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
			}
			else
			{
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairTop = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
			}

			//计算CrosshairSpread, 准星散开值
			//速度因子
			FVector2D FromSpeedRange(0.f, OwnedCharacter->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D ToCrosshairSpreadRange(0.f, 1.f);
			FVector Velocity = OwnedCharacter->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(FromSpeedRange, ToCrosshairSpreadRange, Velocity.Size());
			//坠落因子
			if (OwnedCharacter->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bIsAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			//射击动作是瞬时的, 所以射击因子在开火时增加, 同时也在Tick降低至0
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 30.f);

			//准星上下左右距离中心的距离
			HUDPackage.CrosshairSpreadScale =
				0.6f								//基础的距离
				+ CrosshairVelocityFactor			//速度因子--散开
				+ CrosshairInAirFactor				//坠落因子--散开
				- CrosshairAimFactor				//瞄准因子--聚拢
				+ CrosshairShootingFactor;			//射击因子--散开

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bIsAiming)
	{
		CurrnetFOV = FMath::FInterpTo(CurrnetFOV, EquippedWeapon->GetWeaponZoomedFOV(), DeltaTime, EquippedWeapon->GetWeaponZoomInterpSpeed());
	}
	else
	{
		CurrnetFOV = FMath::FInterpTo(CurrnetFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (OwnedCharacter && OwnedCharacter->GetFollowCamera())
	{
		OwnedCharacter->GetFollowCamera()->SetFieldOfView(CurrnetFOV);
	}
}


