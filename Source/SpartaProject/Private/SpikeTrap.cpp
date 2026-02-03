// Fill out your copyright notice in the Description page of Project Settings.


#include "SpikeTrap.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"

// Sets default values
ASpikeTrap::ASpikeTrap()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Collision->SetBoxExtent(FVector(50.0f, 50.0f, 30.0f));

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(Collision);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Collision->OnComponentBeginOverlap.AddDynamic(this, &ASpikeTrap::OnOverlapBegin);

	DamageAmount = 15.0f;

}


void ASpikeTrap::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASpikeTrap::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->ActorHasTag("Player"))
	{
		FDamageEvent DamageEvent;
		OtherActor->TakeDamage(
			DamageAmount,
			DamageEvent,
			nullptr,
			this);
	}
}



