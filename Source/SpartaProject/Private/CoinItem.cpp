#include "CoinItem.h"
#include "Engine/World.h"
#include "SpartaGameState.h"
#include "GameFramework/RotatingMovementComponent.h"

ACoinItem::ACoinItem()
{
	PointValue = 0;
	ItemType = "DefaultCoin";

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingComp"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);
}

void ACoinItem::ActivateItem(AActor* Activator)
{
	Super::ActivateItem(Activator);

	if (Activator && Activator->ActorHasTag("Player"))
	{
		if (UWorld* World = GetWorld())
		{
			if (ASpartaGameState* GameState = World->GetGameState<ASpartaGameState>())
			{
				GameState->AddScore(PointValue);
				GameState->OnCoinCollected();
			}
		}
		DestroyItem();
	}
}
