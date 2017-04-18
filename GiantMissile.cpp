// Fill out your copyright notice in the Description page of Project Settings.

#include "Leviathan.h"
#include "Kismet/KismetMathLibrary.h"
#include "GiantMissile.h"


AGiantMissile::AGiantMissile()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(true);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	SetRootComponent(StaticMesh);

	JetParticleComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleComp"));
	JetParticleComp->SetupAttachment(RootComponent);

	EMPParticleComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("EMPParticleComp"));
	EMPParticleComp->SetupAttachment(RootComponent);
	EMPParticleComp->bAutoActivate = false;

	JetLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("JetLight"));
	JetLight->SetupAttachment(RootComponent);
	JetLight->Intensity = 500.0f;
	JetLight->AttenuationRadius = 10000.0f;
	JetLight->SetRelativeLocation(FVector(-1350, 0, 0));

	WarheadCollider = CreateDefaultSubobject<USphereComponent>(TEXT("WarheadCollider"));
	//WarheadCollider->SetSphereRadius(10.0f);
	WarheadCollider->AttachToComponent(StaticMesh, FAttachmentTransformRules::KeepRelativeTransform);
	WarheadCollider->OnComponentBeginOverlap.AddDynamic(this, &AGiantMissile::OnWarheadBeginOverlap);

	ObjectiveMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObjectiveMarker"));
	ObjectiveMarker->SetupAttachment(RootComponent);
	ObjectiveMarker->SetVisibility(true);

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	AudioComp->SetupAttachment(RootComponent);
}


void AGiantMissile::BeginPlay()
{
	Super::BeginPlay();
	TurningSpeed = FMath::RandRange(TurningSpeed * 0.9f, TurningSpeed * 1.9f);
	
	Dest = (GetActorForwardVector() * 20000) - GetActorLocation();

	if (FlySoundbase)
	{
		AudioComp->SetSound(FlySoundbase);
		if (AudioComp->Sound)
			AudioComp->Play();
	}

	UMyGameInstance* GameInstance = Cast<UMyGameInstance>(GetGameInstance());
	AudioComp->SetVolumeMultiplier(GameInstance->Volume);
}


void AGiantMissile::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	SparkTimer += DeltaTime;
	SightTimer += DeltaTime;
	TimeSinceFired += DeltaTime;
	PlayerActual = Cast<ALeviathanCharacter>(GetWorld()->GetFirstPlayerController()->GetCharacter());

	//handles draw of objective marker over missile
	ShouldMissileObjMarkerBeVisible();

	if (!bAlive)
	{
		HandleEMP(DeltaTime);
	}
	else
	{
		// Missile is born with propulsion, then tracks after delay
		if (ActivationDelay >= TimeSinceFired)
		{
			StaticMesh->AddForce(StaticMesh->GetForwardVector() * (BaseSpeed + SpeedDynamic));
		}
		else if (PlayerActual)
		{
			// Active Missile -> Check LOS, set Dest for FlightDynamics below
			if (PlayerActual)
			{
				if (!PlayerActual->IsAlive())
				{
					Dest = (GetActorLocation() + FVector::UpVector * 9000) - GetActorLocation();
				}
				else if (!bDomesticated && SightTimer >= (1 / ReactionTime))
				{
					if (LineOfSight())
					{
						Dest = PlayerActual->GetActorLocation();
					}
					else
					{
						Dest = Redirection; /// set in LineOfSight() below
					}
					SightTimer = 0.0f;
				}

				// Get domesticated by player (done by proximity)
				Domesticate(DeltaTime);

				// Fly to either goal
				FlightDynamics(Dest, DeltaTime);
			}
		}
	}
}

void AGiantMissile::Domesticate(float DeltaTime)
{
	// Process is done by distance to Player
	float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerActual->GetActorLocation());
	
	// Be a good missile
	if (bDomesticated || (DistToPlayer <= 850 && bAlive))
	{
		DomesticationTimer += DeltaTime;
		if (DomesticationTimer >= 1.1f && PlayerActual->bHooked)
		{
			bDomesticated = true;
			StaticMesh->SetEnableGravity(false);
			Dest = GetActorLocation() + (8000 * PlayerActual->GetCamera()->GetForwardVector());
		}
	}
	else if (DomesticationTimer > 0.0f)
	{
		DomesticationTimer -= DeltaTime;
	}

	// Un-Domesticate
	if (DistToPlayer >= 3333.0f)
	{
		bDomesticated = false;
		StaticMesh->SetEnableGravity(true);
	}
}


void AGiantMissile::FlightDynamics(FVector goalTarget, float deltaT)
{
	// Get flyable vector
	FVector DestPosition	= goalTarget;
	FVector ToDest			= goalTarget - GetActorLocation();
	float DistanceToGoal	= ToDest.Size();

	if (DistanceToGoal >= DamageOuterRadius)
	{
		// Create 'pilot feel' by counteracting our velocity
		FVector CurrentVelocity = StaticMesh->GetComponentVelocity() - ToDest;
		CurrentVelocity.X *= 0.0f; /// prevents missile turning away at the moment of le kill
		CurrentVelocity.Z *= 1.333f;
		DestPosition -= (CurrentVelocity * (DistanceToGoal / 2000.0f));

		// Regard gravity
		if (DistanceToGoal >= (DamageOuterRadius * 3.0f))
			DestPosition += (FVector::UpVector * CounterGravityEffort);
	}

	// Rotate smooth to DestPosition
	FRotator RoteToGoal = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), DestPosition);
	RoteToGoal = FMath::RInterpTo(GetActorRotation(), RoteToGoal, deltaT, TurningSpeed);
	RootComponent->SetWorldRotation(RoteToGoal);

	// Prepare dynamic speed based on nose-angle to Player
	FVector VecToGoal = (PlayerActual->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector LocalFord = StaticMesh->GetForwardVector();
	float tempAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(VecToGoal, LocalFord)));
	SpeedDynamic = (BaseSpeed / (tempAngle + 0.1f)) * KillVectorAcceleration;
	//GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Green, FString::Printf(TEXT("Speed:  %f"), SpeedDynamic / 100));
	
	// Propulse forward
	StaticMesh->AddForce(StaticMesh->GetForwardVector() * (BaseSpeed + SpeedDynamic));
}


// LOS -- handles Redirection
bool AGiantMissile::LineOfSight() 
{
	// Prepare variables
	bool result = false;
	FVector ToPlayer = PlayerActual->GetActorLocation() - GetActorLocation();
	float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerActual->GetActorLocation());

	// do LOS by raycast
	if (DistanceToPlayer <= VisionRange)
	{
		FHitResult Hit;
		FCollisionQueryParams SelflessQuery;
		SelflessQuery.AddIgnoredActor(this);
		FVector TraceStart = GetActorLocation();
		FVector TraceEnd = PlayerActual->GetActorLocation();

		// Cast that trace
		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Pawn,
			SelflessQuery, FCollisionResponseParams::DefaultResponseParam);
		///DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::White, false, 0.2f, 0, 5.0f);

		if (Hit.Actor == PlayerActual)
			{ result = true; }
		else
		{
			FVector Detour = Hit.Normal * 10000.0f + ToPlayer + (FVector::UpVector * 10000.0f); /// create detour around obstruction
			Redirection = Detour;
			//DrawDebugLine(GetWorld(), GetActorLocation(), Detour, FColor::Green, false, 0.1f, 0, 10.f);
		}
	}

	return result;
}


void AGiantMissile::EMPShock()
{
	bAlive = false;
	bPreviouslyHit = true;
	JetLight->Intensity = 100.0f;
	JetParticleComp->Deactivate();
	EMPParticleComp->SetActive(true);
	
	StaticMesh->SetLinearDamping(0.2f);
	StaticMesh->SetAngularDamping(0.1f);
	StaticMesh->AddAngularImpulse(FVector(2.0f, 2.0f, 0.5f) * BaseSpeed);

	AudioComp->Stop();
	if (EMPSoundbase)
	{
		AudioComp->SetSound(EMPSoundbase);
		AudioComp->Play();
	}
}

void AGiantMissile::ShouldMissileObjMarkerBeVisible()
{
	ObjectiveMarker->SetVisibility(PlayerActual->bMissileObjMarkerShouldShow);
}

void AGiantMissile::EMPRecover()
{
	bAlive = true;
	JetLight->Intensity = 500.0f;
	EMPParticleComp->SetActive(false);
	JetParticleComp->SetActive(true);

	StaticMesh->SetLinearDamping(2.0f);
	StaticMesh->SetAngularDamping(1.3f);

	AudioComp->Stop();
	if (FlySoundbase)
	{
		AudioComp->SetSound(FlySoundbase);
		AudioComp->Play();
	}
	EMPTimer = 0.0f;
}

void AGiantMissile::HandleEMP(float deltaT)
{
	EMPTimer += deltaT;
	if (EMPTimer >= EMPDownTime)
		EMPRecover();

	/// extra gravity for effect
	StaticMesh->AddForce(FVector::UpVector * -BaseSpeed * 0.279f);
}

// Make sparks!!
void AGiantMissile::ReceiveHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherComp->ComponentHasTag(FName("EMP")) && bAlive)
	{
		EMPShock();
	}
	else
	{
		/// collision sound
		if (SparkTimer >= (0.2f / SparksPerSecond))
		{
			int arraySize = CollideSounds.Num();
			int choice = FMath::FloorToInt(FMath::FRandRange(0, arraySize - 1));
			AudioComp->SetSound(CollideSounds[choice]);
			AudioComp->Play();
		}

		/// SPARKS
		if ((StaticMesh->GetComponentVelocity().Size() > 100.0f)
			&& (SparkTimer >= (1 / SparksPerSecond)))
		{
			FActorSpawnParameters SpawnPars;
			AParticleSource* Sparks =
				Cast<AParticleSource>(GetWorld()->SpawnActor<AParticleSource>
				(ParticleSparks, HitLocation, (StaticMesh->ComponentVelocity.GetSafeNormal() + HitNormal).Rotation(), SpawnPars)
					);

			if (Sparks)
				SparkTimer = 0.0f;
		}
	}
}

void AGiantMissile::OnWarheadBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherComp->ComponentHasTag("Bridge"))
	{
		///GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("oh look a bridge"));
	}
	if (OtherComp->ComponentHasTag(FName("EMP")) && bAlive)
	{
		EMPShock();
		bAlive = false;
	}

	// Detonation
	else if (bAlive && (!OtherComp->ComponentHasTag(FName("Camera"))) && OtherActor != NULL  &&  OtherComp->GetOwner() != this) /// bAlive represents active EMP effects
	{
		// DAMAGE
		if (PlayerActual && LineOfSight())
		{
			TArray<AActor*> IgnoredActors;
			IgnoredActors.Add(this);
			IgnoredActors.Add(Instigator);

			UGameplayStatics::ApplyRadialDamageWithFalloff(
				GetWorld(),
				BaseDamage,
				MinimumDamage,
				GetActorLocation(),
				DamageInnerRadius,
				DamageOuterRadius,
				DamageFalloff,
				UDamageType::StaticClass(),
				IgnoredActors,
				this,
				Instigator != NULL ? Instigator->GetController() : NULL,
				ECC_Visibility);

			// PHYSICS FORCE
			TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjects;
			TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
			TArray<AActor*> SphereHits;
			TArray<AActor*> ActorsIgnored;
			ActorsIgnored.Add(this);
			if (UKismetSystemLibrary::SphereOverlapActors_NEW(
				GetWorld(), RootComponent->GetComponentLocation(), DamageOuterRadius, 
				TraceObjects, AActor::StaticClass(), ActorsIgnored, SphereHits))
			{
				for (int i = 0; i < SphereHits.Num(); i++)
				{
					UStaticMeshComponent* SM = Cast<UStaticMeshComponent>((SphereHits[i])->GetRootComponent());
					SM->AddRadialImpulse(GetActorLocation(), DamageOuterRadius, ExplosionForce * 1.5f, ERadialImpulseFalloff::RIF_Linear, true);
				}
			}

			// PLAYER FORCE
			float DistToPlayer = FVector::Dist(PlayerActual->GetActorLocation(), GetActorLocation());
			if (PlayerActual && DistToPlayer <= DamageOuterRadius)
				PlayerActual->GetMovementComponent()->AddRadialImpulse(GetActorLocation(), DamageOuterRadius, ExplosionForce, ERadialImpulseFalloff::RIF_Linear, true);
		}
		AudioComp->Stop();
		
		if (DeathSoundbase)
		{
			AudioComp->SetSound(DeathSoundbase);
			AudioComp->Play();
		}
		
		// PARTICLES
		if (UWorld* ThisWorld = GetWorld())
		{
			FActorSpawnParameters SpawnParameters;
			Explosion = Cast<AParticleSource>(ThisWorld->SpawnActor<AParticleSource>(ParticleExplosion,
				GetActorLocation(), SweepResult.Normal.Rotation(), SpawnParameters));
			
			class AAmbientSound* ExplosionSoundRef = Cast<AAmbientSound>(ThisWorld->SpawnActor<AAmbientSound>(ExplosionSound, GetActorLocation(), SweepResult.Normal.Rotation(), SpawnParameters));

			if (ExplosionSoundRef)
			{
				UMyGameInstance* GameInstance = Cast<UMyGameInstance>(GetGameInstance());
				ExplosionSoundRef->GetAudioComponent()->SetVolumeMultiplier(GameInstance->Volume);

				ExplosionSoundRef->GetAudioComponent()->SetSound(DeathSoundbase);
				ExplosionSoundRef->GetAudioComponent()->Play();
			}
			if (Explosion)
			{
				Destroy();
			}
		}
	}
}