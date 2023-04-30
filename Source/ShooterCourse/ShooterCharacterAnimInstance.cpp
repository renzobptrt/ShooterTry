// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	}
}

void UShooterCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	
}

