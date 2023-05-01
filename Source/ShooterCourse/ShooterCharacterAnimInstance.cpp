// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UShooterCharacterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if(ShooterCharacter == nullptr)
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}

	if(ShooterCharacter)
	{
		//Get the lateral speed of the character from velocity
		FVector Velocity {ShooterCharacter->GetVelocity()};
		Velocity.Z= 0;
		Speed = Velocity.Size();

		// Is the character in the air
		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		// Is the character accelerating?
		bIsAccelerating = ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ;

		FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
		if(ShooterCharacter->GetVelocity().Size()>0.f)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;	
		}
		//FString RotationMessage = FString::Printf(TEXT("Base Aim Rotation: %f"), AimRotation.Yaw); 
		//FString MovementRotationMessage = FString::Printf(TEXT("Movement Rotation: %f"), MovementRotation.Yaw)
		//FString OffsetMessage = FString::Printf(TEXT("Movement Offset Yaw: %f"), MovementRotation.Yaw);

		bAiming = ShooterCharacter->GetAiming();
	}
}

void UShooterCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	
}

