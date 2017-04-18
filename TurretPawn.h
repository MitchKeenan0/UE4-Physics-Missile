#pragma once

#include "GameFramework/Actor.h"
#include "LeviathanCharacter.h"
#include "Projectile.h"
#include "GiantMissile.h"
#include "ParticleSource.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.h"
#include "TurretPawn.generated.h"

UCLASS()
class LEVIATHAN_API ATurretPawn : public APawn
{
	GENERATED_BODY()

public:	
	ATurretPawn();
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaSeconds ) override;
	virtual ALeviathanCharacter* GetPlayer() { return PlayerActual; }

	virtual bool SpotPlayer();
	virtual void TrackTarget();
	virtual bool TakeTheShot();
	virtual void FireProjectile();

	// On Hit by Smokey emp shot
	virtual void EMPShock();

	bool alive = true;
	//bool bIsEMPAble = true;

	UPROPERTY(EditDefaultsOnly)
	bool bActivated = true;

	UPROPERTY(Category = Turret, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* BaseObject;

	UPROPERTY(Category = Turret, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* BaseStaticMesh;

	UPROPERTY(Category = Turret, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* GimbalStaticMesh;

	UPROPERTY(Category = Turret, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* GunStaticMesh;

	UPROPERTY(Category = Turret, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* FirePoint;

	/*UPROPERTY(Category = Turret, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class ACharacter* PlayerExistsInWorld;*/

	UPROPERTY(Category = Turret, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class ALeviathanCharacter* PlayerActual;

	UPROPERTY(EditDefaultsOnly)
	class UParticleSystemComponent* EMPParticles;

	/*UPROPERTY(Category = Turret, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* SkelComponent;*/

	class AProjectile* ActiveProjectile;
	class AParticleSource* ActiveParticles;
	class AGiantMissile* ActiveMissile;
	

protected:
	float ShotTimer = 0.0f;
	float TimeDelta = 0.0f;
	float EMPTimer = 0.0f;
	bool bCanFire = false;

	FVector PerceivedPlayerPosition = FVector::ZeroVector;
	FVector LastPlayerPosition = FVector::ZeroVector;
	FVector PlayerPositionDelta = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AGiantMissile> MissileClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AParticleSource> ParticleClass;

	UPROPERTY(EditAnywhere, Category = Sound)
	USoundBase* Soundbase;
	
	UPROPERTY(EditAnywhere, Category = Sound)
	UAudioComponent* AudioComp;

	UPROPERTY(EditDefaultsOnly)
	bool bAimAheadOfTarget;

	UPROPERTY(EditDefaultsOnly)
	bool bInaccurate;

	UPROPERTY(EditDefaultsOnly)
	bool bSeparateAxes;

	UPROPERTY(EditDefaultsOnly)
	float OuterTrackingRange;

	UPROPERTY(EditDefaultsOnly)
	float InnerTrackingRange;

	UPROPERTY(EditDefaultsOnly)
	float RotationSpeed = 1.5f;

	UPROPERTY(EditDefaultsOnly)
	float ShotsPerSecond = 1;

	UPROPERTY(EditDefaultsOnly)
	float LeadingMargin;

	UPROPERTY(EditDefaultsOnly)
	float ElevationAngle = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float EngagementAngle = 9.0f;

	UPROPERTY(EditDefaultsOnly)
	float ShotSpread = 0.1f;

	UPROPERTY(EditDefaultsOnly)
	float EMPDownTime = 3.0f;

	UPROPERTY(EditDefaultsOnly)
	float KillZ = -35000.0f;

	//death stuff
	class AParticleSource* Explosion;

	UPROPERTY(EditDefaultsOnly)
		TSubclassOf<AParticleSource> ParticleExplosion;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
		TSubclassOf<AAmbientSound> ExplosionSound;
	UPROPERTY(EditAnywhere, Category = Sound)
		USoundBase* DeathSoundbase;
};