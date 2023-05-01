// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AShooterCharacter::AShooterCharacter():
	// Base rates for turning / looking up
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f),
	// Turn rats for aimings / not aimings
	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingTurnRate(20.f),
	AimingLookUpRate(20.f),
	//Mouse look sensitivity scale factors
	MouseHipTurnRate(1.0f),
	MouseHipLookUpRate(1.0f),
	MouseAimingTurnRate(0.2f),
	MouseAimingLookUpRate(0.2f),
	// True when aiming the weapon
	bAiming(false),
	// Camera field of view values 
	CameraDefaultFOV(0.f), // set in BeginPlay
	CameraZoomedFOV(35.f),
	CameraCurrentFOV(0.f),
	ZoomInterpSpeed(20.f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// How to Create a Subobject
	// Create a camera boom (pull in towards the character if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.f; //The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; //Rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent:: SocketName); //Attach camera to end of boom
	FollowCamera-> bUsePawnControlRotation = false; //Camera doesn't rotate realative to arm

	// Dont rotate when the controller rotates. Let the controller only affect the camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; //Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); //... at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if(FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}
}

void AShooterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && 
		Value != 0.0f)
	{
		//Find out which way is formwar
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr &&
		Value != 0.0f)
	{
		//Find out which way is formwar
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	// Calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds()); // DeltaTime: deg/sec * sec/frame
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds()); // DeltaTime: deg/sec * sec/frame
}

void AShooterCharacter::Turn(float Value)
{
	float TurnScaleFactor {};

	TurnScaleFactor = bAiming ? MouseAimingTurnRate : MouseHipTurnRate ;
	
	AddControllerYawInput( Value * TurnScaleFactor);
}

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor {};

	LookUpScaleFactor = bAiming ? MouseAimingLookUpRate : MouseHipLookUpRate ;
	
	AddControllerPitchInput( Value * LookUpScaleFactor);
}

void AShooterCharacter::FireWeapon()
{
	if(FireSound)
	{
		UGameplayStatics::PlaySound2D(this,FireSound);
	}

	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");
	if(BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());

		if(MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),MuzzleFlash, SocketTransform);
		}

		FVector BeamEnd;
		bool bBeamEnd = GetBeamEndLocation(
			SocketTransform.GetLocation(),
			BeamEnd);

		if(bBeamEnd)
		{
			// Spawn impact particles after updating BeamEndPoint
			if(ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					BeamEnd);
			}
			
			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					BeamParticles,
					SocketTransform);
				if(Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}
		/*
		FHitResult FireHit;
		const FVector Start { SocketTransform.GetLocation() };
		const FQuat Rotation { SocketTransform.GetRotation() };
		const FVector RotationAxis { Rotation.GetAxisX() };
		const FVector End {Start + RotationAxis * 50'000.f };

		FVector BeamEndPoint { End };
		
		GetWorld()->LineTraceSingleByChannel(FireHit, Start, End, ECC_Visibility);
		if(FireHit.bBlockingHit)
		{
			//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.f);
			//DrawDebugPoint(GetWorld(), FireHit.Location, 5.f, FColor::Red, false, 2.f);

			BeamEndPoint = FireHit.Location;
			
			if(ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.Location);
			}
		}

		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),BeamParticles,SocketTransform);
			if(Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEndPoint);
			}
		}
		*/
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

bool AShooterCharacter::GetBeamEndLocation(
	const FVector& MuzzleSocketLocation,
	FVector& OutBeamLocation)
{
	// Get current size of the viewport
	FVector2D ViewportSize;
	if(GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// Get screen space location of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	CrosshairLocation.Y -= 50.f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// Get world position and direction of crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this,0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);
	
	if(bScreenToWorld) // Was deprojection successful?
	{
		FHitResult ScreenTraceHit;
		const FVector Start { CrosshairWorldPosition };
		const FVector End { CrosshairWorldPosition + CrosshairWorldDirection * 50'000.f };

		// Set beam end point to line trace end point
		OutBeamLocation = End;
		// Trace outward from crosshairs world location
		GetWorld()->LineTraceSingleByChannel(
			ScreenTraceHit,
			Start,
			End,
			ECC_Visibility);
		
		if(ScreenTraceHit.bBlockingHit) // was there a trace hit?
		{
			// Beam end point is now trace hit location
			OutBeamLocation = ScreenTraceHit.Location;
		}

		// Perform a second trace, this time from the gun barrel
		FHitResult WeaponTraceHit;
		const FVector WeaponTraceStart { MuzzleSocketLocation };
		const FVector WeaponTraceEnd { OutBeamLocation };
		GetWorld()->LineTraceSingleByChannel(
			WeaponTraceHit,
			WeaponTraceStart,
			WeaponTraceEnd,
			ECC_Visibility);

		if(WeaponTraceHit.bBlockingHit) //Objects between barrel and BeamEndPoint?
		{
			OutBeamLocation = WeaponTraceHit.Location;
		}

		return true;
	}
	
	return false;
}

void AShooterCharacter::AimingButtonPressed()
{
	bAiming = true;
}

void AShooterCharacter::AimingButtonReleased()
{
	bAiming = false;
}

void AShooterCharacter::CameraInterpZoom(float DeltaTime)
{
	// Set current camera field of view
	CameraCurrentFOV = bAiming ?
			FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed) : //Interpolate to zoomed FOV
			FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed); //Interpolate to default FOV

	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AShooterCharacter::SetLookRates()
{
	if(bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle interpolation for zoom when aiming 
	CameraInterpZoom(DeltaTime);
	// Change look sensitivity based on aiming
	SetLookRates();
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn ); //&APawn::AddControllerYawInput
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp); //&APawn::AddControllerPitchInput

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireWeapon);
	
	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);
}

