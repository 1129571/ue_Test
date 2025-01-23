


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Characters/MultiShootCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Characters/MultiShootCharacter.h"
#include "PlayerController/MultiShootPlayerController.h"

AWeapon::AWeapon()
{
 	PrimaryActorTick.bCanEverTick = false;
	// 我们将武器设计为服务器负责, 此处开启复制
	bReplicates = true;

	//复制运动, 防止客户端和服务器位置不同步的问题
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	//相当于碰撞设置
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);				//所有碰撞通道Block
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);		//忽略Pawn通道
	//相当于碰撞开关(默认)
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);		//默认模型禁用碰撞

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollsion"));
	AreaSphere->SetupAttachment(RootComponent);
	// 希望球形碰撞用于检测, 并且我们只希望在服务器启用, 
	// 因此构造中默认将碰撞关闭和检测忽略, (服务器中)BeginPlay时再开启
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
	PickupWidget->SetVisibility(false);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	//通常用于判断当前设备是否为 服务器
	//也可以通过GetWorld()->GetNetMode()等方法判断
	//if (GetLocalRole() == ENetRole::ROLE_Authority)
	if(HasAuthority())
	{
		//在服务器启用碰撞 查询和物理
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		//设置pawn的检测通道为重叠
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);;
		//我们只希望在服务器进行武器的拾取检测, 因此在这里进行绑定
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereBeginOverlapCallback);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlapCallback);
	}
}

void AWeapon::OnSphereBeginOverlapCallback(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMultiShootCharacter* MultiShootCharacter = Cast<AMultiShootCharacter>(OtherActor);
	if (MultiShootCharacter && PickupWidget && MultiShootCharacter->GetOverlappingWeapon() == nullptr)
	{
		MultiShootCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlapCallback(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AMultiShootCharacter* MultiShootCharacter = Cast<AMultiShootCharacter>(OtherActor);
	if (MultiShootCharacter && PickupWidget)
	{
		MultiShootCharacter->SetOverlappingWeapon(nullptr);
	}
}

//通过Character在服务器调用
void AWeapon::SetWeaponState(EWeaponState NewWeaponState)
{
	WeaponState = NewWeaponState;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Initial:
		break;
	case EWeaponState::EWS_Equipped:
		//拾取武器后就不再显示PickupWidget了
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//防止Drop过程中玩家试图拾取时因为碰撞导致异常
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		//冲锋枪有一个可以布带, 被装备时我们通过物理效果模拟出布带飘动的效果
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			//拾取武器只发生在服务器, 客户端无权利知道
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		//冲锋枪有一个可以布带, 掉落时我们还原(到初始化时)碰撞效果
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);				//所有碰撞通道Block
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);		//忽略Pawn通道

		// 设置组件之间的位置, 不然检测组件和控件组件永远在初始位置
//		AreaSphere->SetRelativeLocation(WeaponMesh->GetComponentLocation());
//		PickupWidget->SetRelativeLocation(WeaponMesh->GetComponentLocation());
		break;
	case EWeaponState::EWS_MAX:
		break;
	default:
		break;
	}
}

//变量被复制时自动调用
void AWeapon::OnRep_WeaponStateChange(EWeaponState LastState)
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Initial:
		break;
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		//防止Drop过程中玩家试图拾取时因为碰撞导致操作不流畅
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); 

		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}
		break;
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);				//所有碰撞通道Block
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);		//忽略Pawn通道

		// 设置组件之间的位置, 不然检测组件和控件组件永远在初始位置
//		AreaSphere->SetRelativeLocation(WeaponMesh->GetComponentLocation());
//		PickupWidget->SetRelativeLocation(WeaponMesh->GetComponentLocation());
		break;
	case EWeaponState::EWS_MAX:
		break;
	default:
		break;
	}
}

void AWeapon::SpendRound()
{
	//仅在服务器上执行
	//更新当前弹药数量
	CurrentAmmoNum = FMath::Clamp(CurrentAmmoNum - 1, 0, MagCapacity);

	SetHUDWeaponAmmo();
}

void AWeapon::OnRep_CurrentAmmoNum()
{
	SetHUDWeaponAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (Owner == nullptr)
	{
		OwnerCharacter = nullptr;
		OwnerController = nullptr;
	}
	else
	{
		// 确保武器被装备(客户端Owner被复制成功)了之后, 再尝试更新HUD

		SetHUDWeaponAmmo();
	}

}

bool AWeapon::IsEmpty()
{
	return CurrentAmmoNum <= 0;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::ShowPickupWidget(bool bShouldShow)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShouldShow);
	}
}

//注册要复制的变量
void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, CurrentAmmoNum);
}

void AWeapon::SetHUDWeaponAmmo()
{
	//尝试更新HUD
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<AMultiShootCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter)
	{
		OwnerController = OwnerController == nullptr ? Cast<AMultiShootPlayerController>(OwnerCharacter->Controller) : OwnerController;
		if (OwnerController)
		{
			OwnerController->SetHUDWeaponAmmoAmount(CurrentAmmoNum);
		}
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	if (CasingClass)
	{
		const USkeletalMeshSocket* CasingSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (CasingSocket)
		{
			FTransform SocketTransfrom = CasingSocket->GetSocketTransform(WeaponMesh);

			//仅视觉效果, 和伤害系统无关, 因此可以不用设置Owner等属性
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransfrom.GetLocation(),
					SocketTransfrom.GetRotation().Rotator()
				);
			}
		}
	}
	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);

	SetOwner(nullptr);
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
}

void AWeapon::AddAmmo(int32 InAmmoToAdd)
{
	CurrentAmmoNum = FMath::Clamp(CurrentAmmoNum - InAmmoToAdd, 0, MagCapacity);
	SetHUDWeaponAmmo();
}

