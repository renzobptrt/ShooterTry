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
#include "BulletHitInterface.h"

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
	ZoomInterpSpeed(20.f),
	// Crosshair spread factor
	CrosshairSpreadMultiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),
	// Bullet fire timer variables
	ShootTimeDuration(0.05f),
	bFiringBullet(false),
	// Automatic fire variables
	AutomaticFireRate(0.1f),
	bShouldFire(true),
	bFireButtonPressed(false)
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

		FHitResult BeamHitResult;
		bool bBeamEnd = GetBeamEndLocation(
			SocketTransform.GetLocation(),
			BeamHitResult);

		if(bBeamEnd)
		{

			// Does hit Actor implement BulletHitInterface?
			if(BeamHitResult.Actor.IsValid())
			{
				IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.Actor.Get());
				if(BulletHitInterface)
				{
					BulletHitInterface->BulletHit_Implementation(BeamHitResult);
				}
			}
			else
			{
				// Spawn impact particles after updating BeamEndPoint
				// Spwan default particles
				if(ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactParticles,
						BeamHitResult.Location);
				}
			}
			
			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					BeamParticles,
					SocketTransform);
				if(Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamHitResult.Location);
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
	
	// Start bullet fire timer for crosshairs
	StartCrosshairBulletFire();
}

bool AShooterCharacter::GetBeamEndLocation(
	const FVector& MuzzleSocketLocation,
	FHitResult& OutHitResult)
	//FVector& OutBeamLocation)
{
	FVector OutBeamLocation;
	
	// Get current size of the viewport
	FVector2D ViewportSize;
	if(GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// Get screen space location of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
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
		//FHitResult WeaponTraceHit;
		const FVector WeaponTraceStart { MuzzleSocketLocation };
		const FVector WeaponTraceEnd { OutBeamLocation };
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult,
			WeaponTraceStart,
			WeaponTraceEnd,
			ECC_Visibility);

		if(OutHitResult.bBlockingHit) //Objects between barrel and BeamEndPoint?
		{
			OutHitResult.Location = OutBeamLocation;
			return false;
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

void AShooterCharacter::CalculatesCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange {0.f, 600.f};
	FVector2D VelocityMultiplierRange { 0.f, 1.f};
	FVector Velocity { GetVelocity()};
	Velocity.Z = 0.f;

	// Calculate crosshair velocity factor
	CrosshairVelocityFactor = FMath:: GetMappedRangeValueClamped(
		WalkSpeedRange,
		VelocityMultiplierRange,
		Velocity.Size());

	// Calculate crosshair in air factor
	if(GetCharacterMovement()->IsFalling()) // In in air?
	{
		//Spread the crosshairs slowly while in air
		CrosshairInAirFactor = FMath:: FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else // Character is on the ground
	{
		//Shrink the crosshairs rapidly while on the ground
		CrosshairInAirFactor = FMath:: FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	// Calculate crosshair aim factor
	if(bAiming) // Are we aiming?
	{
		// Shrink crosshairs a small amount very quickly
		CrosshairAimFactor = FMath:: FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.f);
	}
	else // Not aiming
	{
		// Spread crosshairs vack to nomral very quickly
		CrosshairAimFactor = FMath:: FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	}

	// True 0.05 second after firing
	if(bFiringBullet)
	{
		CrosshairShootingFactor = FMath:: FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.f );
	}
	else
	{
		CrosshairShootingFactor = FMath:: FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f );
	}
	
	CrosshairSpreadMultiplier =
		0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	StartFireTimer();
}

void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AShooterCharacter::StartFireTimer()
{
	if(bShouldFire)
	{
		FireWeapon();
		bShouldFire = false;
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, AutomaticFireRate);
	}
}

void AShooterCharacter::AutoFireReset()
{
	bShouldFire = true;
	if(bFireButtonPressed)
	{
		StartFireTimer();
	}
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;

	GetWorldTimerManager().SetTimer(
		CrosshairShootTimer,
		this,
		&AShooterCharacter::FinishCrosshairBulletFire,
		ShootTimeDuration);
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle interpolation for zoom when aiming 
	CameraInterpZoom(DeltaTime);
	// Change look sensitivity based on aiming
	SetLookRates();
	// Calculate crosshair spread multiplier
	CalculatesCrosshairSpread(DeltaTime);
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
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);
	
	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);
}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

