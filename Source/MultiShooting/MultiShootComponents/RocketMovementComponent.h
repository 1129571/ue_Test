// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RocketMovementComponent.generated.h"

/**
 * SocketProjectile有时候会和WeaponOwner发生重叠, 造成击杀自己的异常情况
 * 可以通过将WeaponMesh的Projectile生成位置适当前移解决, 这里使用专门的组件解决
 * 实现的效果是, 如果发射物被阻挡, 阻挡的东西离开后发射物会继续移动
 */
UCLASS()
class MULTISHOOTING_API URocketMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()
	
protected:
	/**
	 * @brief 处理物体在移动时遇到“阻挡”碰撞（Blocking Hit）时的行为。触发时机：当一个物体在移动过程中检测到阻挡碰撞（通常由物理引擎的碰撞通道设置决定）时，这个函数会被调用。
	 * @param Hit 碰撞的详细信息，包含了碰撞的位置、法线、被碰撞的物体等。
	 * @param TimeTick 当前的移动时间步长，即此次更新处理的时间间隔。
	 * @param MoveDelta 物体尝试移动的向量，表示当前帧物体的移动方向和距离。
	 * @param SubTickTimeRemaining 剩余未处理的时间步长，如果在碰撞后需要继续移动，该值可能会被更新。
	 * @return 枚举值，用于指示碰撞处理的结果，例如是否停止移动、调整位置、继续尝试移动等。
	 * 典型用途:
	 *		调整物体的位置或方向以避免穿透障碍物。
	 *		根据碰撞法线计算反射向量。
	*		触发碰撞相关事件（如播放声音或生成粒子效果）。
	 */
	virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining) override;
	
	/**
	 * @brief 处理非阻挡碰撞（通常是碰撞通道配置为“Overlap”或“Touch”时）的行为。触发时机：当物体在移动过程中与其他物体发生“接触”而非“阻挡”时调用。通常是为了处理非物理性的交互效果。
	 * @param Hit 同样是碰撞的详细信息，描述了接触的位置、方向以及与之接触的对象。
	 * @param TimeSlice 当前的时间片段，用于指定此次更新的时间间隔（默认为 0）。
	 * @param MoveDelta 物体尝试的移动向量，与 HandleBlockingHit 中的类似。
	 * 典型用途:
	 *		处理轻微接触效果，例如触发粒子效果、声音或动画。
	 *		在角色移动组件中，用于处理软碰撞，例如角色轻微接触墙壁但不受阻碍时的逻辑。
	 */
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override;
};
