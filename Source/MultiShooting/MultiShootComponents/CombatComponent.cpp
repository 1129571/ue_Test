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
#include "HUD/MultiShootHUD.h"
#include "Camera/CameraComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

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

		if (OwnedCharacter->GetFollowCamera())
		{
			DefaultFOV = OwnedCharacter->GetFollowCamera()->FieldOfView;
			CurrnetFOV = DefaultFOV;
		}
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

//本地机器执行
void UCombatComponent::WeaponFire(bool bFire)
{
	bFireState = bFire;

	//本地机器ServerRpc---服务器MulticaseRpc--->所有客户端
	if (bFireState)
	{
		FHitResult TraceResult;
		TraceUnderCrosshairs(TraceResult);
		//TraceResult.ImpactPoint已经是FVector_NetQuantize类型了
		ServerFire(TraceResult.ImpactPoint);

		//开火时本地机器增加射击因子(用于HUD)
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 2.f;
		}
	}
}

//服务器执行
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& InHitTarget)
{
	MultiCastFire(InHitTarget);
}

//所有客户端执行
void UCombatComponent::MultiCastFire_Implementation(const FVector_NetQuantize& InHitTarget)
{
	if (OwnedCharacter && EquippedWeapon)
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

