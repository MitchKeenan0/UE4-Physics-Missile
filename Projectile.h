#pragma once

#include "GameFramework/Actor.h"
#include "ParticleSource.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.h"
#include "Projectile.generated.h"

UCLASS()
class LEVIATHAN_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:
	AProjectile();
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaSeconds ) override;

	UPROPERTY(EditDefaultsOnly)
	bool bSplashDamage = false;

	UPROPERTY(EditDefaultsOnly)
	float SplashRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bHoming = false;

	UPROPERTY(EditDefaultsOnly)
	float HomingAccelScalar = 1000.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bIsPellet;

	UPROPERTY(EditDefaultsOnly)
	bool bChangeSize;

	UPROPERTY(EditDefaultsOnly)
	float SizeChangeSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	FVector InitialSize;

	UPROPERTY(EditDefaultsOnly)
	FVector FinalSize;

	UPROPERTY(EditDefaultsOnly)
	bool bDestroyOnHit;

	UPROPERTY(EditDefaultsOnly)
	float ParticleSpawnSize = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bAccelerating = false;

	UPROPERTY(EditDefaultsOnly)
	float AccelerationScalar;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AParticleSource> P_Explosion;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AParticleSource> P_ShipExplosion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditDefaultsOnly)
	UProjectileMovementComponent* MovementComponent;

	UPROPERTY(EditDefaultsOnly)
	UParticleSystemComponent* ParticleComponent;

	UPROPERTY(EditDefaultsOnly)
	UBoxComponent* ColliderComponent;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* HomingComponent;

	UPROPERTY(EditDefaultsOnly)
	float Damage = 1;

	UPROPERTY(EditDefaultsOnly)
	float ImpactForce;


protected:
	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	bool bSpeedSet = false;
	bool bDmgDelivered = false;
	bool bSparkedOnSelf = false;
	bool bReflected = false;

	class AParticleSource* ExplosionActual;

	float LifeTime = 0.0f;

	UPROPERTY(EditAnywhere, Category = Sound)
		USoundBase* ImpactSoundBase;
	UPROPERTY(EditDefaultsOnly)
		TSubclassOf<AAmbientSound> ImpactSound;
};
