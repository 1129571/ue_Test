


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
#include "MultiShootAnimInstance.h"
#include "MultiShooting.h"
#include "PlayerController/MultiShootPlayerController.h"
#include "GameMode/MultiShootGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerState/MultiShootPlayerState.h"
#include "Weapon/WeaponTypes.h"

AMultiShootCharacter::AMultiShootCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	//生成时碰撞, 使用总是生成, 如果碰撞就尝试移动位置. 防止生成时因为碰撞导致生成失败
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());			//不设置到胶囊组件是因为防止下蹲时相机Z轴移动
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->bUsePawnControlRotation = true;		//鼠标输入作用于控制器时弹簧臂也会旋转
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	CameraBoom->SocketOffset = FVector(0.f, 75.f, 75.f);

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
	//防止Block摄像机通道, 导致SpringArm探头变化
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	//便于子弹高精度检测
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	//开启可视性通道, 便于射击目标检测, 从而改变HUD颜色
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	//设置旋转速度(旋转朝向运动)
	GetCharacterMovement()->RotationRate = FRotator(0.f, 850.f, 0.f);

	TurningInPlace = ETurningInPlace::ETIP_NoTurning;

	//设置网络更新频率, 快节奏射击游戏中常被设置为33 66
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeComponent"));
}

void AMultiShootCharacter::BeginPlay()
{
	Super::BeginPlay();

	//这个函数内部需要Controller, 在玩家重生时有可能Controller还在控制上一个Character, 所以会更新HUD失败
	//处理办法是在Controller中OnPossess时对其控制的Character再次进行UpdateHUDHealth
	UpdateHUDHealth();

	//在服务器绑定OnTakeAnyDamage事件
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}
}

void AMultiShootCharacter::MoveForward(float AxisValue)
{
	if (bDisableGameplay) return;
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
	if (bDisableGameplay) return;
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
	if (bDisableGameplay) return;
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
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AMultiShootCharacter::ReloadPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->Reload();
	}
}

void AMultiShootCharacter::AimPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AMultiShootCharacter::AimReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{	
		Combat->SetAiming(false);
	}
}

void AMultiShootCharacter::FirePressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->WeaponFire(true);
	}
}

void AMultiShootCharacter::FireReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->WeaponFire(false);
	}
}

void AMultiShootCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	//此函数在Tick调用
	//我们希望不动时使用Yaw和Pitch的AO, 移动时使用Pitch的AO(因为已经有Lean实现的偏移了, 效果已经比较让人满意)
	float Speed = CalculateSpeed();
	bool IsFalling = GetCharacterMovement()->IsFalling();
	//后 : 静止时使用最后一次存储的初始旋转, 再每帧通过当前的BaseAimRotation计算偏移Yaw值
	if (Speed == 0.f && !IsFalling)
	{
		bRotateRootBone = true;
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
		bRotateRootBone = false;
		StartAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;
	}

	CalculateAO_Pitch();
}

void AMultiShootCharacter::CalculateAO_Pitch()
{
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

void AMultiShootCharacter::SimProxiesTurn()
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;

	bRotateRootBone = false;

	if (CalculateSpeed() > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;
		return;
	}

	//单独处理 Yaw 旋转, 两帧之间超过某一旋转值就原地播放动画
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold )
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if(ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else 
		{
			TurningInPlace = ETurningInPlace::ETIP_NoTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NoTurning;
}

void AMultiShootCharacter::Jump()
{
	if (bDisableGameplay) return;
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

void AMultiShootCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

//接收到伤害后若死亡(只绑定到了服务器)调用
void AMultiShootCharacter::Elim()
{
	//武器掉落
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim();

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ThisClass::ElimTimerFinished,
		ElimDelay
	);
}

void AMultiShootCharacter::Destroyed()
{
	Super::Destroyed();

	//销毁ElimBot粒子组件
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}
}

void AMultiShootCharacter::MulticastElim_Implementation()
{
	//武器弹药HUD
	if (MultiShootPlayerController)
	{
		MultiShootPlayerController->SetHUDWeaponAmmoAmount(0);
	}

	bElimmed = true;
	PlayElimMontage();

	//使用溶解材质并初始化材质参数
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	//禁用角色移动相关能力
	GetCharacterMovement()->DisableMovement();					//不能移动, 但是可以旋转角色
	GetCharacterMovement()->StopMovementImmediately();			//不能再旋转角色
	bDisableGameplay = true;									//禁用大部分输入

	//禁用角色碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//生成ElimBot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(), 
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
			);
	}
	//ElimBot声音
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}

void AMultiShootCharacter::ElimTimerFinished()
{
	AMultiShootGameMode* MultiShootGameMode = GetWorld()->GetAuthGameMode<AMultiShootGameMode>();
	//玩家重生
	if (MultiShootGameMode)
	{
		MultiShootGameMode->RequestRespawn(this, GetController());
	}
}

void AMultiShootCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void AMultiShootCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		//Timeline 使用指定Curve 资产和 函数(类似于蓝图TimeLine后面的执行内容?)
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
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

void AMultiShootCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if( (FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			//没有区别
			//Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
			Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(false);
		}
	}
	else {
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			//Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
			Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(true);
		}
	}
}

float AMultiShootCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void AMultiShootCharacter::SetOverlappingWeapon(AWeapon* InWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
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

FVector AMultiShootCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState AMultiShootCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

void AMultiShootCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	RotateInPlace(DeltaTime);

	HideCameraIfCharacterClose();
	// 初始化HUD分数, 因为PlayerState要比BenginPlay晚
	PollInit();
}

void AMultiShootCharacter::RotateInPlace(float DeltaTime)
{
	//禁用原地旋转, 动画
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;

		return;
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{	//本地模拟角色
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void AMultiShootCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipWeaponPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchPressed);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ThisClass::ReloadPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ThisClass::AimPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ThisClass::AimReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ThisClass::FirePressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ThisClass::FireReleased);

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
	DOREPLIFETIME(AMultiShootCharacter, CurrentHealth);
	DOREPLIFETIME(AMultiShootCharacter, bDisableGameplay);
}

void AMultiShootCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->OwnedCharacter = this;
	}
}

void AMultiShootCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && WeaponFireMontage)
	{
		AnimInstance->Montage_Play(WeaponFireMontage);
		FName SectionName;		//我们的Montage有两个Section, Hip和Aim部分
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMultiShootCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void AMultiShootCharacter::PlayHitReactMontage()
{
	//因为我们的Montage动画是持枪的, 所有需要进行武器检查
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName;
		SectionName = FName("FromLeft");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMultiShootCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);

		FName SelectName;		//不同武器类型的换弹动画不一样, 使用不同的Montage Select
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SelectName = FName("Rifle");
			break;
		}

		AnimInstance->Montage_JumpToSection(SelectName);
	}
}

void AMultiShootCharacter::PollInit()
{
	if (MultiShootPlayerState == nullptr)
	{
		MultiShootPlayerState = GetPlayerState<AMultiShootPlayerState>();
		if (MultiShootPlayerState)
		{
			MultiShootPlayerState->AddToScore(0.f);
			MultiShootPlayerState->AddToDefeats(0);
		}
	}
}

//仅在服务器执行, 客户端通过变量复制执行
void AMultiShootCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.f, MaxHealth);
	
	UpdateHUDHealth();
	PlayHitReactMontage();

	if (CurrentHealth == 0.f)
	{
		AMultiShootGameMode* MultiShootGameMode = GetWorld()->GetAuthGameMode<AMultiShootGameMode>();
		if (MultiShootGameMode)
		{
			MultiShootPlayerController = MultiShootPlayerController == nullptr ? Cast<AMultiShootPlayerController>(Controller) : MultiShootPlayerController;
			AMultiShootPlayerController* AttackerController = Cast<AMultiShootPlayerController>(InstigatedBy);
			MultiShootGameMode->PlayerEliminated(this, MultiShootPlayerController, AttackerController);
		}
	}
}

//仅在客户端执行
void AMultiShootCharacter::OnRep_CurrentHealth()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void AMultiShootCharacter::UpdateHUDHealth()
{
	MultiShootPlayerController = MultiShootPlayerController == nullptr ? Cast<AMultiShootPlayerController>(Controller) : MultiShootPlayerController;
	if (MultiShootPlayerController)
	{
		MultiShootPlayerController->SetHUDHealth(CurrentHealth, MaxHealth);
	}
}

