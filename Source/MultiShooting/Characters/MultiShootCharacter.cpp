


#include "MultiShootCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "MultiShootComponents/CombatComponent.h"

AMultiShootCharacter::AMultiShootCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());			//不设置到胶囊组件是因为防止下蹲时相机Z轴移动
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;		//鼠标输入作用于控制器时弹簧臂也会旋转
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 160.f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;		//不希望角色和控制器一起旋转
	//希望角色朝向运动方向
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));
	OverHeadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->SetIsReplicated(true);
}

void AMultiShootCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMultiShootCharacter::MoveForward(float AxisValue)
{
	if (Controller != nullptr && AxisValue != 0.f)
	{
		//在这里我们希望以控制器的前为Character的前方
		//C++里是Pitch Yaw Roll
		//这里我们只关心移动, 速度的设置在MovementComponent
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, AxisValue);
	}
}

void AMultiShootCharacter::MoveRight(float AxisValue)
{
	if (Controller != nullptr && AxisValue != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, AxisValue);
	}
}

void AMultiShootCharacter::Turn(float AxisValue)
{
	AddControllerYawInput(AxisValue);
}

void AMultiShootCharacter::LookUp(float AxisValue)
{
	AddControllerPitchInput(AxisValue);
}

void AMultiShootCharacter::EquipWeapon()
{
	// Combat 是我们自定义的组件, 武器战斗相关操作由其完成
	if (Combat)
	{
		if (HasAuthority())		// 只有服务器可以执行
			Combat->EquipWeaponFun(OverlappingWeapon);
		else  //客户端 远程调用, 注意这里的函数名没有_Implementation后缀
			ServerEquipEquipWeapon();
	}
}

//客户端远程调用服务器方法
void AMultiShootCharacter::ServerEquipEquipWeapon_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeaponFun(OverlappingWeapon);
	}
}

void AMultiShootCharacter::onRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	else
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void AMultiShootCharacter::SetOverlappingWeapon(AWeapon* InWeapon)
{
	if (IsLocallyControlled())
	{
		if (!InWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}
	}

	OverlappingWeapon = InWeapon;

	//ListenServer上控制的角色也可以正常显示PickupWidget
	//客户端控制的角色---复制变量改变导致PickupWidget可视性变化
	//ListenServer端控制的角色---服务器自己判断让自己控制的角色PickupWidget可视性变化
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void AMultiShootCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMultiShootCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//直接绑定到ACharacter的Jump, 便于测试, 之后覆写修改
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipWeapon);

	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ThisClass::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUp);

}

//ListenServer永远不会被调用该函数, 只会在客户端执行
void AMultiShootCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//注册要复制的变量
	//参数 : 具有复制变量的类	需要被复制的变量
	//DOREPLIFETIME(AMultiShootCharacter, OverlappingWeapon);
	//增加了把这个变量复制给哪个客户端, 该例只会复制给Pawn的拥有者
	DOREPLIFETIME_CONDITION(AMultiShootCharacter, OverlappingWeapon, COND_OwnerOnly);

}

void AMultiShootCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->OwnedCharacter = this;
	}
}

