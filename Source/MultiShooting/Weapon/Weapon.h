

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

//武器状态枚举
UENUM(BlueprintType)
enum class EWeaponState : uint8 
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	//占位, 防止越界 或 获取枚举最大值等
	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class MULTISHOOTING_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;

	void ShowPickupWidget(bool bShouldShow);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Owner() override;

	void SetHUDWeaponAmmo();

	// 只有Virtual函数才可以被重写
	// 非虚拟函数会被子类同名函数隐藏, 编译器调用哪个取决于声明的指针或引用类型.
	virtual void Fire(const FVector& HitTarget);

	//武器掉落
	void Dropped();

	void AddAmmo(int32 InAmmoToAdd);

	//武器的准星资源
	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshairs")
	class UTexture2D* CrosshairCenter;
	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshairs")
	UTexture2D* CrosshairLeft;
	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshairs")
	UTexture2D* CrosshairRight;
	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshairs")
	UTexture2D* CrosshairTop;
	UPROPERTY(EditAnywhere, Category = "Weapon|Crosshairs")
	UTexture2D* CrosshairBottom;

	/* 音效资源 */
	UPROPERTY(EditAnywhere, Category = "Weapon|SoundCue")
	class USoundCue* EquipSound;

	/*
	* 自动开火武器相关
	*/
	UPROPERTY(EditAnywhere, Category = "Weapon|AutoFire")
	bool bCanAutoFire = true;

	UPROPERTY(EditAnywhere, Category = "Weapon|AutoFire")
	float AutoFireDelay = 0.2f;

protected:
	virtual void BeginPlay() override;

	//用于处理Sphere的重叠回调
	UFUNCTION()
	virtual void OnSphereBeginOverlapCallback(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	virtual void OnSphereEndOverlapCallback(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);
private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	class USphereComponent* AreaSphere;

	//复制变量
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WeaponStateChange, Category = "Weapon")
	EWeaponState WeaponState;

	//复制变量变化时自动调用的函数
	UFUNCTION()
	void OnRep_WeaponStateChange(EWeaponState LastState);

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	class UAnimationAsset* FireAnimation;

	//武器弹出的子弹壳类
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	//Aiming时对FOV进行放大
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;		//该武器的开镜速度

	/**
	 * 武器弹药
	 */

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_CurrentAmmoNum)
	int32 CurrentAmmoNum;

	void SpendRound();			//CurrentAmmoNum--, 更新HUD		

	UFUNCTION()
	void OnRep_CurrentAmmoNum();

	UPROPERTY(EditAnywhere)		//弹夹容量
	int32 MagCapacity;

	UPROPERTY()
	class AMultiShootCharacter* OwnerCharacter;
	UPROPERTY()
	class AMultiShootPlayerController* OwnerController;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

public:
	void SetWeaponState(EWeaponState NewWeaponState);
	FORCEINLINE class USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE class USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetWeaponZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetWeaponZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetCurrentAmmo() const { return CurrentAmmoNum; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE int32 GetReloadNeedAmmo() const { return MagCapacity - CurrentAmmoNum; }
	bool IsEmpty();
};
