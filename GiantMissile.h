// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ParticleSource.h"
#include "LeviathanCharacter.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.h"
#include "GiantMissile.generated.h"

UCLASS()
class LEVIATHAN_API AGiantMissile : public AActor
{
	GENERATED_BODY()
	
public:	
	AGiantMissile();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	void Domesticate(float DeltaTime);
	bool IsDomesticated() { return bDomesticated; }
	void EMPShock();

	void ShouldMissileObjMarkerBeVisible();

	UPROPERTY(EditDefaultsOnly, Category = "Flight")
	float ActivationDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flight")
	float BaseSpeed = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flight")
	float KillVectorAcceleration = 1.1f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Flight")
	float TurningSpeed = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flight")
	float CounterGravityEffort = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pilot")
	float VisionRange = 9000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pilot")
	float ReactionTime = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float ExplosionForce = 3333.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float MinimumDamage = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageFalloff = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageOuterRadius = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageInnerRadius = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Sparks")
	float SparksPerSecond = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "EMP")
	float EMPDownTime = 4.5f;

protected:
	UFUNCTION()
	void FlightDynamics(FVector goalTarget, float deltaT);

	UFUNCTION()
	bool LineOfSight();

	void EMPRecover();

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AParticleSource> ParticleSparks;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AParticleSource> ParticleExplosion;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAmbientSound> ExplosionSound;

	UPROPERTY(VisibleAnywhere)
	class ALeviathanCharacter* PlayerActual;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* StaticMesh;

	UPROPERTY(VisibleAnywhere)
	class UParticleSystemComponent* JetParticleComp;

	UPROPERTY(VisibleAnywhere)
	class UParticleSystemComponent* EMPParticleComp;

	UPROPERTY(VisibleAnywhere)
	class UPointLightComponent* JetLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USphereComponent* WarheadCollider;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* ObjectiveMarker;

	UPROPERTY(EditAnywhere, Category = Sound)
	USoundBase* DeathSoundbase;
	UPROPERTY(EditAnywhere, Category = Sound)
	USoundBase* EMPSoundbase;
	UPROPERTY(EditAnywhere, Category = Sound)
	USoundBase* FlySoundbase;
	UPROPERTY(EditAnywhere, Category = Sound)
	TArray<USoundBase*> CollideSounds;
	UPROPERTY(EditAnywhere, Category = Sound)
	UAudioComponent* AudioComp;

private:
	UFUNCTION()
	void HandleEMP(float deltaT);

	UFUNCTION()
	void ReceiveHit
	(
		class UPrimitiveComponent* MyComp,
		class AActor* Other,
		class UPrimitiveComponent* OtherComp,
		bool bSelfMoved,
		FVector HitLocation,
		FVector HitNormal,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	UFUNCTION()
	void OnWarheadBeginOverlap
	(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	class AParticleSource* Explosion;

	UPROPERTY(EditAnywhere)
	TArray<AActor*> ExplosionActors;
	
	float TimeSinceFired = 0.0f;
	float SparkTimer = 0.0f;
	float SightTimer = 0.0f;
	float EMPTimer = 0.0f;
	float SpeedDynamic = 0.0f;
	float TurnSpeedActual = 0.0f;
	float DomesticationTimer = 0.0f;

	bool bAlive = true;
	bool bRedirectionSet = false;
	bool bDestSet = false;
	bool bPreviouslyHit = false;
	bool bDomesticated = false;

	FVector Dest = FVector::ZeroVector;
	FVector Redirection = FVector::ZeroVector;

	
};
