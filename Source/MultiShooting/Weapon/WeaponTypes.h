#pragma once



#define TRACE_LENGTH 80000

UENUM(BlueprintType)
enum class EWeaponType : uint8 
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),
	EWT_ShotGun UMETA(DisplayName = "Shot Gun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrendaLauncher UMETA(DisplayName = "Grenda Launcher"),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};