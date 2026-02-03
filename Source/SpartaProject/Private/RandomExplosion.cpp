#include "RandomExplosion.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"

ARandomExplosion::ARandomExplosion()
{
 
	PrimaryActorTick.bCanEverTick = false;

	ExplosionDelay = 3.0f;
	ExplosionRadius = 200.0f;
	DamageAmount = 30.0f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Collision->InitSphereRadius(ExplosionRadius);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(Collision);



}

// Called when the game starts or when spawned
void ARandomExplosion::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		ExplosionTimerHandle,
		this,
		&ARandomExplosion::Explode,
		ExplosionDelay, false);
}

void ARandomExplosion::Explode()
{
	if (ExplosionParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticle, GetActorLocation(), GetActorRotation(), true);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());
	}

	TArray<AActor*> OverlappingActors;
	Collision->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && Actor->ActorHasTag("Player"))
		{
			FDamageEvent DamageEvent;
			Actor->TakeDamage(
				DamageAmount,
				DamageEvent,
				nullptr,
				this);
		}
	}

	Destroy();
}

void ARandomExplosion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(ExplosionTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}
	

