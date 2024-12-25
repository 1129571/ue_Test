

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MultiShootCharacter.generated.h"

UCLASS()
class MULTISHOOTING_API AMultiShootCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMultiShootCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void Turn(float AxisValue);
	void LookUp(float AxisValue);

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverHeadWidget;
public:	

	
};
