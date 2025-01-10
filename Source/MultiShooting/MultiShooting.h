// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

//在编辑器我们自定义了一个碰撞检测通道, 为了在C++更直观, 所以定义宏ECC_SkeletalMesh
#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1
