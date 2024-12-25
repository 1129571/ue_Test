


#include "MultiShootCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"


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

void AMultiShootCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMultiShootCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//直接绑定到ACharacter的Jump, 便于测试, 之后覆写修改
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ThisClass::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUp);

}


