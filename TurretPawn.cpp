#include "Leviathan.h"
#include "Kismet/GameplayStatics.h"
#include "TurretPawn.h"

ATurretPawn::ATurretPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseObject = CreateDefaultSubobject<USceneComponent>(TEXT("BaseObject"));
	RootComponent = BaseObject;

	BaseStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseStaticMesh"));
	BaseStaticMesh->SetupAttachment(BaseObject);

	GimbalStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GimbalStaticMesh"));
	GimbalStaticMesh->SetupAttachment(BaseObject);
	
	GunStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunStaticMesh"));
	GunStaticMesh->SetupAttachment(GimbalStaticMesh);

	FirePoint = CreateDefaultSubobject<USceneComponent>(TEXT("FirePoint"));
	FirePoint->SetupAttachment(GunStaticMesh);

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	AudioComp->SetupAttachment(RootComponent);

	EMPParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("EMPParticles"));
	EMPParticles->SetupAttachment(RootComponent);

	if (ProjectileClass != nullptr)
	{
		bAimAheadOfTarget = true;
	}

	this->Tags.Add("Turret");
}

void ATurretPawn::BeginPlay()
{
	Super::BeginPlay();

	EMPParticles->SetActive(false);
//<<<<<<< HEAD
	if (AudioComp && Soundbase)
	{
		AudioComp->SetSound(Soundbase);
	}
	if (ShotsPerSecond != 0)
		ShotTimer = ShotsPerSecond * 0.9f;

	PlayerActual = Cast<ALeviathanCharacter>(GetWorld()->GetFirstPlayerController()->GetCharacter());
//=======
	AudioComp->SetSound(Soundbase);
	/*if (ShotsPerSecond != 0)
		ShotTimer = ShotsPerSecond * 0.9f;*/
	//PlayerActual = Cast<ALeviathanCharacter>(GetWorld()->GetFirstPlayerController()->GetCharacter());
//>>>>>>> 046ccc7022cd5e1f8152c33f18f453955039aaed

	UMyGameInstance* GameInstance = Cast<UMyGameInstance>(GetGameInstance());
	AudioComp->SetVolumeMultiplier(GameInstance->Volume);
}

void ATurretPawn::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	PlayerActual = Cast<ALeviathanCharacter>(GetWorld()->GetFirstPlayerController()->GetCharacter());
	if (PlayerActual)
	{
		// Healthy turret activities
		if (bActivated && alive)
		{
			//PlayerActual = Cast<ALeviathanCharacter>(GetWorld()->GetFirstPlayerController()->GetCharacter());
			TimeDelta = DeltaTime;
			ShotTimer += TimeDelta;
			
			if (PlayerActual->IsAlive() && SpotPlayer())
			{
				if (!MissileClass)
					TrackTarget();
				
				if (TakeTheShot() && (PlayerActual->GetHealth() > 0)
					&& (ProjectileClass || MissileClass))
					FireProjectile();
			}
		}

/*
		// EMP Shock
		if (!alive)
		{
			EMPTimer += DeltaTime;
			if (EMPTimer >= (EMPDownTime))
			{
				EMPParticles->SetActive(false);
				alive = true;
				EMPTimer = 0.0f;
			}
		}*/
	}
}

bool ATurretPawn::SpotPlayer()
{
	bool result = false;

	float DistanceToPlayer = FVector::Dist(PlayerActual->GetActorLocation(), GetActorLocation());
	if (DistanceToPlayer <= OuterTrackingRange)
	{
		FHitResult Hit;
		FCollisionQueryParams SelflessQuery;
		SelflessQuery.AddIgnoredActor(this);
		//SelflessQuery.AddIgnoredActor(Cast<AActor>(ProjectileClass->GetDefaultObject()));

		FVector EyeOffset = FVector::UpVector * BaseEyeHeight;
		FVector TraceStart = GimbalStaticMesh->GetComponentLocation() + EyeOffset;
		FVector TraceEnd = PlayerActual->GetActorLocation();

		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Pawn,
			SelflessQuery, FCollisionResponseParams::DefaultResponseParam);
		if (Hit.Actor == PlayerActual)
		{
			result = true;
			///DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 0.01f, 0, 5.f);
		}
	}

	return result;
}

void ATurretPawn::TrackTarget()
{
	// Prepare the ingredients
	PerceivedPlayerPosition = PlayerActual->GetActorLocation();
	float DistanceToPlayer = GetDistanceTo(PlayerActual);

	if (DistanceToPlayer <= OuterTrackingRange)
	{
		LastPlayerPosition = PerceivedPlayerPosition;

		// Create vector to 'lead' the target
		if (bAimAheadOfTarget)
		{
			FVector PlayerVelocity = PlayerActual->GetVelocity();
			if (PlayerVelocity.Size() < 100) // No movement input
			{
				// Track independant movement (eg player onboard a moving ship)
				PlayerPositionDelta = PerceivedPlayerPosition - LastPlayerPosition;
				PerceivedPlayerPosition += PlayerPositionDelta;
			}

			float DistanceCompensation = LeadingMargin * FMath::Sqrt(DistanceToPlayer);
			FVector LeadFromPlayer = PlayerVelocity * DistanceCompensation;
			PerceivedPlayerPosition += LeadFromPlayer * 0.1f;

			if (ElevationAngle > 0)
			{
				float DistSqr = FMath::Square(DistanceToPlayer);
				float VerticalAddition = (DistSqr * ElevationAngle) / 1000;
				PerceivedPlayerPosition.Z += VerticalAddition;
			}
		}

		// Make Rotation to goal vector
		FRotator RotToPlayer = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), PerceivedPlayerPosition);
		RotToPlayer = FMath::RInterpTo(FirePoint->GetComponentRotation(), RotToPlayer, TimeDelta, RotationSpeed);
		
		// Handle articulation
		if (bSeparateAxes)
		{
			float HorizontalTurn = RotToPlayer.Yaw;
			float VerticalTurn = RotToPlayer.Pitch;
			FVector HVector = FVector(0, 0, HorizontalTurn);
			FVector VVector = FVector(0, VerticalTurn, 0);
			FRotator HorznRotation = FRotator::MakeFromEuler(HVector);
			FRotator VertlRotation = FRotator::MakeFromEuler(VVector);

			GimbalStaticMesh->SetWorldRotation(HorznRotation);
			GunStaticMesh->SetRelativeRotation(VertlRotation);
		}
		else
		{
			GunStaticMesh->SetRelativeRotation(RotToPlayer);
		}

		/// Draw beam visualize
		/*FVector BeamStart = FirePoint->GetComponentLocation();
		DrawDebugLine(GetWorld(), BeamStart,
			BeamStart + FirePoint->GetForwardVector() * (DistanceToPlayer * 2.f),
			FColor::Green, false, -1.0f, 0, 5.f);*/
	}

	LastPlayerPosition = PerceivedPlayerPosition;
}

bool ATurretPawn::TakeTheShot()
{
	// Returns true if the gun is pointing at goal vector
	bool result = false;
	FVector LocalForward = GunStaticMesh->GetForwardVector();
	FVector LocalPosition = GetActorLocation();
	FVector PlayerPosition = PerceivedPlayerPosition;
	FVector ToPlayer = (PlayerPosition - LocalPosition).GetUnsafeNormal();
	float AngleToPlayer = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(LocalForward, ToPlayer)));
	///GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Angle to player: %f"), AngleToPlayer));

	if (AngleToPlayer <= EngagementAngle)
	{
		if (ShotTimer > (1 / ShotsPerSecond))
		{
			result = true;
			ShotTimer = 0.0f;
		}
	}

	return result;
}

void ATurretPawn::FireProjectile()
{
	// Prepare ingredients
	FActorSpawnParameters SpawnParameters;
	FVector FirePosition = FirePoint->GetComponentLocation();
	FVector FireDirection = FirePoint->GetForwardVector();

	// Bullet Spread
	if (bInaccurate)
	{
		float zOff = FMath::RandRange(-ShotSpread, ShotSpread);
		float yOff = FMath::RandRange(-ShotSpread, ShotSpread);
		FireDirection.Z += zOff;
		FireDirection.Y += yOff;
	}

	// Fire Projectile
	if (ProjectileClass)
	{
		ActiveProjectile = Cast<AProjectile>(GetWorld()->SpawnActor<AProjectile>
			(ProjectileClass, FirePosition, FireDirection.Rotation(), SpawnParameters));
		
		AudioComp->Play();

		if (ParticleClass != nullptr)
		{
			ActiveParticles = Cast<AParticleSource>(GetWorld()->SpawnActor<AParticleSource>
				(ParticleClass, FirePosition, FireDirection.Rotation(), SpawnParameters));
			ActiveParticles->SetBaseActor(ActiveProjectile);
		}
	}

	// Fire ze Missile
	else if (MissileClass)
	{
		ActiveMissile = Cast<AGiantMissile>(GetWorld()->SpawnActor<AGiantMissile>
			(MissileClass, FirePosition, FireDirection.Rotation(), SpawnParameters));
		
		//AudioComp->Play();
	}
}

void ATurretPawn::EMPShock()
{
	/*
	if (alive == true)
	{
		alive = false;
		EMPParticles->SetActive(true);
	}
	else if (alive == false)
	{*/
		FActorSpawnParameters SpawnParameters;

		Explosion = Cast<AParticleSource>(GetWorld()->SpawnActor<AParticleSource>(ParticleExplosion,
			GetActorLocation(), GetActorRotation(), SpawnParameters));

		class AAmbientSound* ExplosionSoundRef = Cast<AAmbientSound>(GetWorld()->SpawnActor<AAmbientSound>(ExplosionSound,
			GetActorLocation(), GetActorRotation(), SpawnParameters));

		if (ExplosionSoundRef && DeathSoundbase)
		{
			ExplosionSoundRef->GetAudioComponent()->SetSound(DeathSoundbase);
			ExplosionSoundRef->GetAudioComponent()->Play();
		}
		Destroy();
	//}

}
