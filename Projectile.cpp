#include "Leviathan.h"
#include "Projectile.h"
#include "LeviathanCharacter.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = StaticMesh;

	ColliderComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("ColliderComponent"));
	ColliderComponent->SetCollisionProfileName(TEXT("Projectile"));
	ColliderComponent->SetupAttachment(RootComponent);

	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleComponent"));
	ParticleComponent->SetupAttachment(RootComponent);

	HomingComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HomingComponent"));

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	
	OnActorBeginOverlap.AddDynamic(this, &AProjectile::OnOverlapBegin);
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (bHoming)
	{
		MovementComponent->bIsHomingProjectile = true;
		MovementComponent->HomingAccelerationMagnitude = HomingAccelScalar;
	}

	float RandomSizeChangeSpeed = FMath::FRandRange(0.7f, 1.5f);
	SizeChangeSpeed *= RandomSizeChangeSpeed;

	float RandomInitialSize = FMath::FRandRange(0.66f, 1.0f);
	if (bChangeSize && StaticMesh && InitialSize != FVector::ZeroVector)
		StaticMesh->SetRelativeScale3D(InitialSize * RandomInitialSize);


}

// Called every frame
void AProjectile::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
	
	if (LifeTime <= 0.15f)
		LifeTime += DeltaTime;

	if (bHoming)
	{
		if (ACharacter* Player = GetWorld()->GetFirstPlayerController()->GetCharacter())
		{
			//HomingComponent->AttachToComponent(Player->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			//HomingComponent->AttachTo(Player->GetRootComponent());
			//HomingComponent->SetRelativeLocation(FVector::ZeroVector);
			MovementComponent->HomingTargetComponent = Player->GetCapsuleComponent();
		}
	}

	// Life-time effects
	if (bAccelerating)
	{
		MovementComponent->Velocity *= AccelerationScalar;
	}
	if (bChangeSize && !IsPendingKill())
	{
		FVector ShiftingSize = FMath::VInterpTo
			(StaticMesh->GetComponentScale(), FinalSize, DeltaTime, SizeChangeSpeed);
		StaticMesh->SetRelativeScale3D(ShiftingSize);
	}

	// Projectile's purpose is done
	if (ParticleComponent && bDmgDelivered)
	{
		if (ParticleComponent->bWasCompleted)
			Destroy();
	}
}


void AProjectile::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	bool DoDamage = true;
				///0.1f
	// Ignore clones or false starts
	if (LifeTime < 0.1f || (OverlappedActor->GetClass() == OtherActor->GetClass()))
	{
		///GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, FString::Printf(TEXT("Other: %s"), *OtherActor->GetClass()->GetName()));
		DoDamage = false;
	}

	// Get Reflected by Player
	if (ALeviathanCharacter* Player = Cast<ALeviathanCharacter>(OtherActor))
	{
		if (Player->bShielded)
		{
			DoDamage = false;
			MovementComponent->Velocity *= -1;
			bReflected = true;
		}
		else { DoDamage = true; }
	}

	// Hit Success
	if (DoDamage)
	{
		//	GEngine->AddOnScreenDebugMessage
		//		(-1, 1.5f, FColor::Red, FString::Printf(TEXT("Other: %s"), *OtherActor->GetClass()->GetName()));

		FActorSpawnParameters SpawnParameters;

		// Damage
		if (bSplashDamage)
		{
			TArray<AActor*> IgnoredActors;
			IgnoredActors.Add(this);
			IgnoredActors.Add(Instigator);

			UGameplayStatics::ApplyRadialDamageWithFalloff(
				GetWorld(),
				Damage,
				1,///minimumdamage
				GetActorLocation(),
				1,///damageinnerradius
				SplashRadius,
				0.1f,///falloff
				UDamageType::StaticClass(),
				IgnoredActors,
				this,
				Instigator != NULL ? Instigator->GetController() : NULL,
				ECC_Visibility);
		}
		else
		{
			TSubclassOf<UDamageType> const ValidDamageTypeClass = TSubclassOf<UDamageType>(UDamageType::StaticClass());
			FDamageEvent DamageEvent(ValidDamageTypeClass);
			OtherActor->TakeDamage(Damage, DamageEvent, GetInstigatorController(), this);
		}
		
		// Explosion
		if (P_Explosion)
		{
			// Rotation for striking head on
			FVector flip = FVector(0, 180, 0);
			FRotator Rote = flip.Rotation() + GetActorRightVector().Rotation();
			ExplosionActual = Cast<AParticleSource>
				(GetWorld()->SpawnActor<AParticleSource>
					(P_Explosion, GetActorLocation(), Rote, SpawnParameters)
				);
			ExplosionActual->SetActorRelativeScale3D(GetActorScale3D() * ParticleSpawnSize);
		}
		
		// Impact Sound
		if (ImpactSoundBase)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("I am playing my sound"));
			//FActorSpawnParameters SpawnParameters;
			class AAmbientSound* ImpactSoundRef = Cast<AAmbientSound>(GetWorld()->SpawnActor<AAmbientSound>(ImpactSound, GetActorLocation(), GetActorRotation(), SpawnParameters));
			
			UMyGameInstance* GameInstance = Cast<UMyGameInstance>(GetGameInstance());
			ImpactSoundRef->GetAudioComponent()->SetVolumeMultiplier(GameInstance->Volume);

			ImpactSoundRef->GetAudioComponent()->SetSound(ImpactSoundBase);
			ImpactSoundRef->GetAudioComponent()->Play();
		}

		Destroy();
	}
	
}


