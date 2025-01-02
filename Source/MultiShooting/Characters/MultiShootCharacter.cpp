


#include "MultiShootCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "MultiShootComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

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

	//设置可以蹲下
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	//设置旋转速度(旋转朝向运动)
	GetCharacterMovement()->RotationRate = FRotator(0.f,850.f, 0.f);

	//防止Block摄像机通道, 导致SpringArm探头变化
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	TurningInPlace = ETurningInPlace::ETIP_NoTurning;

	//设置网络更新频率, 快节奏射击游戏中常被设置为33 66
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
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

void AMultiShootCharacter::EquipWeaponPressed()
{
	// Combat 是我们自定义的组件, 武器战斗相关操作由其完成
	if (Combat)
	{
		if (HasAuthority())		// 只有服务器可以执行
		{
			Combat->EquipWeaponFun(OverlappingWeapon);
		}
		else  //客户端 远程调用, 注意这里的函数名没有_Implementation后缀
		{ 
			ServerEquipWeapon();
		}

	}
}

void AMultiShootCharacter::CrouchPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AMultiShootCharacter::AimPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AMultiShootCharacter::AimReleased()
{
	if (Combat)
	{	
		Combat->SetAiming(false);
	}
}

void AMultiShootCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	//此函数在Tick调用
	//我们希望不动时使用Yaw和Pitch的AO, 移动时使用Pitch的AO(因为已经有Lean实现的偏移了, 效果已经比较让人满意)
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool IsFalling = GetCharacterMovement()->IsFalling();
	//后 : 静止时使用最后一次存储的初始旋转, 再每帧通过当前的BaseAimRotation计算偏移Yaw值
	if (Speed == 0.f && !IsFalling)
	{
		//GetBaseAimRotation可以理解为GetControllerRotation(一台机器只有一个Controller)的网络版本
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NoTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		//我们希望Character的Yaw在[-90, 90]之外进行一次Yaw的90度旋转, 其余时间使用AimOffset
		bUseControllerRotationYaw = true;		//通过动画蓝图Rotate Root Bone 节点替代
		TurnInPlaceFun(DeltaTime);
	}
	//先 : 移动 / 掉落时每帧更新当前的BaseAimRotation, 将其作为初始旋转
	if (Speed > 0.f || IsFalling)
	{
		StartAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;
	}

	//在这里, CharacterMovemnetComponent中对Pitch和Yaw进行了压缩, 以便在网络传递
	//造成的效果是, 将本地的值发送到服务器后限制在了[0~360)
	AO_Pitch = GetBaseAimRotation().Pitch;

	//修正Pitch, [270, 360)~[-90, 0)
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AMultiShootCharacter::Jump()
{
	//覆写jump方法, 我们希望在蹲下jump时是站起来
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void AMultiShootCharacter::TurnInPlaceFun(float DeltaTime)
{
	if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	else if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NoTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		//转到位了
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NoTurning;
			StartAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

//客户端远程调用服务器方法
void AMultiShootCharacter::ServerEquipWeapon_Implementation()
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

bool AMultiShootCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AMultiShootCharacter::IsAiming()
{
	return (Combat && Combat->bIsAiming);
}

AWeapon* AMultiShootCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

void AMultiShootCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

void AMultiShootCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipWeaponPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ThisClass::AimPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ThisClass::AimReleased);

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

