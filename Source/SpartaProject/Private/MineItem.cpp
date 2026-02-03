// Fill out your copyright notice in the Description page of Project Settings.


#include "MineItem.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SpartaGameState.h"
#include "Particles/ParticleSystemComponent.h"


AMineItem::AMineItem()
{
	
	ExplosionDelay = 5.0f;
	ExplosionRadius = 300.0f;
	ExplosionDamage = 30.0f;
	ItemType = "Mine";
	bHasExploded = false;

	ExplosionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ExplosionCollision"));
	ExplosionCollision->InitSphereRadius(ExplosionRadius);
	ExplosionCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	ExplosionCollision->SetupAttachment(Scene);
}

float AMineItem::GetDelayByLevel(int32 LevelIndex) const
{
	switch (LevelIndex)
	{
	case 0:  return 5.0f; // 레벨 1은 5초
	case 1:  return 4.0f; // 레벨 2는 4초
	case 2:  return 3.0f; // 레벨 3은 3초
	default: return ExplosionDelay; // 기본값
	}
}

void AMineItem::ActivateItem(AActor* Activator)
{
	if (bHasExploded) return;
	// 게임 월드 -> 타이머 매니저 존재(타이머 핸들러 관리)
	// 타이머 핸들러
	Super::ActivateItem(Activator);

	float FinalDelay = ExplosionDelay;
	if (UWorld* World = GetWorld())
	{
		if (ASpartaGameState* GameState = World->GetGameState<ASpartaGameState>())
		{
			// GameState에 저장된 CurrentLevelIndex 사용
			FinalDelay = GetDelayByLevel(GameState->CurrentLevelIndex);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(
		ExplosionTimerHandle,
		this,
		&AMineItem::Explode,
		FinalDelay,
		false);

	bHasExploded = true;
}

void AMineItem::Explode()
{
	UParticleSystemComponent* Particle = nullptr;
	if (ExplosionParticle)
	{
		Particle = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ExplosionParticle,
			GetActorLocation(),
			GetActorRotation(),
			false
		);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ExplosionSound,
			GetActorLocation()
		);
	}
	TArray<AActor*> OverlappingActors;
	ExplosionCollision->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && Actor->ActorHasTag("Player"))
		{
			UGameplayStatics::ApplyDamage(
				Actor,
				ExplosionDamage,
				nullptr,
				this,
				UDamageType::StaticClass()
			);
		}
	}

	DestroyItem();

	if (Particle)
	{

		GetWorld()->GetTimerManager().SetTimer(
			DestroyParticleTimerHandle,
			[Particle]()
			{
				if (IsValid(Particle))
				{
					Particle->DestroyComponent();
				}
			},
			2.0f,
			false
		);
	}
}
