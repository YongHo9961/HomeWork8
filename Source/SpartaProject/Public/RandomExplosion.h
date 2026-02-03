// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomExplosion.generated.h"

class USphereComponent;
class UParticleSystem;
class USoundBase;

UCLASS()
class SPARTAPROJECT_API ARandomExplosion : public AActor
{
	GENERATED_BODY()
	
public:	
	ARandomExplosion();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explosion")
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explosion")
	UStaticMeshComponent* StaticMesh; // 폭발반경 확인용

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	UParticleSystem* ExplosionParticle; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	USoundBase* ExplosionSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionDelay; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionRadius; // 폭발 범위

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float DamageAmount;

	FTimerHandle ExplosionTimerHandle;

	void Explode();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

};
