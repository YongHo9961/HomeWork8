#include "SpartaGameState.h"
#include "SpartaGameInstance.h"
#include "SpartaPlayerController.h"
#include "SpartaCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "SpawnVolume.h"
#include "CoinItem.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "SpikeTrap.h"
#include "RandomExplosion.h"

ASpartaGameState::ASpartaGameState()
{
	Score = 0;
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	LevelDuration = 0.0f;
	CurrentLevelIndex = 0;
	MaxLevels = 3;
	CurrentWaveIndex = 0;
	MaxWaves = 3;
	ItemsPerWave = 40;
}

void ASpartaGameState::BeginPlay()
{
	Super::BeginPlay();

	StartLevel();

	GetWorldTimerManager().SetTimer(
		HUDUpdateTimerHandle,
		this,
		&ASpartaGameState::UpdateHUD,
		0.1f,
		true
	);
}

void ASpartaGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(Wave3ExplosionTimerHandle);
		GetWorldTimerManager().ClearTimer(LevelTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

int32 ASpartaGameState::GetScore() const
{
	return Score;
}

void ASpartaGameState::AddScore(int32 Amount)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToScore(Amount);
		}
	}
}

float ASpartaGameState::GetWaveDuration(int32 LevelIndex, int32 WaveIndex) const
{

	if (LevelIndex == 0) // 레벨 1
	{
		switch (WaveIndex)
		{
		case 1: return 60.0f;
		case 2: return 50.0f;
		case 3: return 40.0f;
		default: return 30.0f; // 예외 처리
		}
	}
	else if (LevelIndex == 1) // 레벨 2
	{
		switch (WaveIndex)
		{
		case 1: return 50.0f;
		case 2: return 40.0f;
		case 3: return 30.0f;
		default: return 30.0f;
		}
	}
	else // 레벨 3
	{
		switch (WaveIndex)
		{
		case 1: return 40.0f;
		case 2: return 35.0f;
		case 3: return 30.0f;
		default: return 30.0f;
		}
	}
}

void ASpartaGameState::SpawnRandomExplosionActor()
{
	if (!RandomExplosionClass) return;

	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);

	if (FoundVolumes.Num() > 0)
	{
		ASpawnVolume* SpawnVolume = Cast<ASpawnVolume>(FoundVolumes[0]);
		if (SpawnVolume)
		{
			FVector SpawnLocation = SpawnVolume->GetRandomPointvolume();
			GetWorld()->SpawnActor<ARandomExplosion>(RandomExplosionClass, SpawnLocation, FRotator::ZeroRotator);
		}
	}
}

void ASpartaGameState::StartLevel()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->ShowGameHUD();
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			CurrentLevelIndex = SpartaGameInstance->CurrentLevelIndex;
		}
	}

	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	CurrentWaveIndex = 0;

	StartWave();
}

void ASpartaGameState::StartWave()
{
	CurrentWaveIndex++;

	LevelDuration = GetWaveDuration(CurrentLevelIndex, CurrentWaveIndex);

	// 웨이브마다 타이머 설정
	GetWorldTimerManager().ClearTimer(LevelTimerHandle);
	GetWorldTimerManager().SetTimer(
		LevelTimerHandle,
		this,
		&ASpartaGameState::OnLevelTimeUp,
		LevelDuration,
		false
	);

	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);

	int32 CoinSpawnThisWave = 0;

	if (FoundVolumes.Num() > 0)
	{
		ASpawnVolume* SpawnVolume = Cast<ASpawnVolume>(FoundVolumes[0]);
		if (SpawnVolume)
		{
			for (int32 i = 0; i < ItemsPerWave; i++)
			{
				AActor* SpawnedActor = SpawnVolume->SpawnRandomItem();
				// 스폰된 것이 코인인지 확인 (코인만 카운트)
				if (SpawnedActor && SpawnedActor->IsA(ACoinItem::StaticClass()))
				{
					SpawnedCoinCount++; // 전체 코인 수 증가
					CoinSpawnThisWave++;
				}
			}

			if (CurrentWaveIndex == 2 && SpikeTrapClass)
			{
				for (int32 i = 0; i < 15; i++) // 10개 생성
				{
					FVector SpawnLocation = SpawnVolume->GetRandomPointvolume();
					SpawnLocation.Z = 0.0f;
					GetWorld()->SpawnActor<ASpikeTrap>(SpikeTrapClass, SpawnLocation, FRotator::ZeroRotator);
				}
			}
		}
	}

	if (CurrentWaveIndex == 3)
	{
		GetWorldTimerManager().SetTimer(
			Wave3ExplosionTimerHandle,
			this,
			&ASpartaGameState::SpawnRandomExplosionActor,
			2.0f,
			true
		);
	}

	WaveTargetCoinCount = CollectedCoinCount + CoinSpawnThisWave;

}

void ASpartaGameState::EndWave()
{
	GetWorldTimerManager().ClearTimer(LevelTimerHandle);
	GetWorldTimerManager().ClearTimer(Wave3ExplosionTimerHandle);

	if (CurrentWaveIndex >= MaxWaves)
	{
		EndLevel();
	}
	else
	{
		// 다음 웨이브 시작
		StartWave();
	}
}

void ASpartaGameState::OnLevelTimeUp()
{
	OnGameOver(false);
}

void ASpartaGameState::OnCoinCollected()
{
	CollectedCoinCount++;
	UE_LOG(LogTemp, Warning, TEXT("Coin Collected: %d / %d"),
		CollectedCoinCount,
		SpawnedCoinCount);

	if (SpawnedCoinCount > 0 && CollectedCoinCount >= WaveTargetCoinCount)
	{
		EndWave();
	}

}

void ASpartaGameState::EndLevel()
{
	GetWorldTimerManager().ClearTimer(HUDUpdateTimerHandle);
	GetWorldTimerManager().ClearTimer(LevelTimerHandle);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			AddScore(Score);
			CurrentLevelIndex++;
			SpartaGameInstance->CurrentLevelIndex = CurrentLevelIndex;
		}

		if (CurrentLevelIndex >= MaxLevels)
		{
			OnGameOver(true);
			return;
		}


		if (LevelMapNames.IsValidIndex(CurrentLevelIndex))
		{
			UGameplayStatics::OpenLevel(GetWorld(), LevelMapNames[CurrentLevelIndex]);
		}
		else
		{
			OnGameOver(true);
		}
	}
}

void ASpartaGameState::OnGameOver(bool bIsWin)
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->SetPause(true);
			SpartaPlayerController->ShowGameOver(bIsWin);
		}
	}
}

void ASpartaGameState::UpdateHUD()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SPartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			if (UUserWidget* HUDWidget = SPartaPlayerController->GetHUDWidget())
			{
				if (UTextBlock* TimeText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Time"))))
				{
					float RemainingTime = GetWorldTimerManager().GetTimerRemaining(LevelTimerHandle);
					TimeText->SetText(FText::FromString(FString::Printf(TEXT("Time: %.1f"), RemainingTime)));
				}

				if (UTextBlock* ScoreText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Score"))))
				{
					if (UGameInstance* GameInstance = GetGameInstance())
					{
						USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
						if (SpartaGameInstance)
						{
							ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), SpartaGameInstance->TotalScore)));
						}
					}
				}

				if (UTextBlock* LevelText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
				{
					LevelText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d - Wave: %d"),
						CurrentLevelIndex + 1, CurrentWaveIndex)));
				}

				ASpartaCharacter* PlayerCharacter = Cast<ASpartaCharacter>(PlayerController->GetCharacter());
				if (PlayerCharacter)
				{
					if (UTextBlock* SlowText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("SlowDebuff"))))
					{
						float SlowTime = PlayerCharacter->GetSlowingRemainingTime();

						if (SlowTime > 0.0f)
						{
							SlowText->SetVisibility(ESlateVisibility::Visible);
							FString InfoString = FString::Printf(TEXT("SLOW\n%.1f"), SlowTime);
							SlowText->SetText(FText::FromString(InfoString));
						}
						else
						{
							SlowText->SetVisibility(ESlateVisibility::Collapsed);
						}
					}

					if (UTextBlock* CoinText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("RemainCoin"))))
					{
						int32 RemainCoin = SpawnedCoinCount - CollectedCoinCount;
						RemainCoin = FMath::Max(0, RemainCoin);

						CoinText->SetText(FText::FromString(FString::Printf(TEXT("Coin: %d"), RemainCoin)));
					}

					if (UTextBlock* ReverseText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("ReverseDebuff"))))
					{
						float ReverseTime = PlayerCharacter->GetReverseRemainingTime();

						if (ReverseTime > 0.0f)
						{
							ReverseText->SetVisibility(ESlateVisibility::Visible);

							FString InfoString = FString::Printf(TEXT("REVERSE\n%.1f"), ReverseTime);
							ReverseText->SetText(FText::FromString(InfoString));
						}
						else
						{
							ReverseText->SetVisibility(ESlateVisibility::Collapsed);
						}
					}
				}
			}
		}
	}
}



