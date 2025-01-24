// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AGrenadeProjectile::AGrenadeProjectile()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;

	ProjectileMovementComponent->InitialSpeed = 15000.f;
	ProjectileMovementComponent->MaxSpeed = 15000.f;
}

void AGrenadeProjectile::BeginPlay()
{
	AActor::BeginPlay();

	SpawnTrailSystem();

	StartDestroyTimer();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &ThisClass::OnBounce);
}

void AGrenadeProjectile::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation()
		);
	}
}

void AGrenadeProjectile::Destroyed()
{
	ExplodeDamage(MinDamage, InnerRadius, OuterRadius, DamageFalloff);

	Super::Destroyed();
}
