


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Characters/MultiShootCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"

AWeapon::AWeapon()
{
 	PrimaryActorTick.bCanEverTick = false;
	// 我们将武器设计为服务器负责, 此处开启复制
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
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
	if (MultiShootCharacter && PickupWidget)
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

//变量被复制时自动调用
void AWeapon::OnRep_WeaponStateChange(EWeaponState LastState)
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Initial:
		break;
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		break;
	case EWeaponState::EWS_MAX:
		break;
	default:
		break;
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
		//武器被拾取后就不再产生碰撞检测了
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		break;
	case EWeaponState::EWS_MAX:
		break;
	default:
		break;
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::ShowPickupWidget(bool bShouldShow)
{
	PickupWidget->SetVisibility(bShouldShow);
}

//注册要复制的变量
void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
}

