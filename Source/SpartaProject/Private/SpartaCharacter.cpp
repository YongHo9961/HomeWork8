
#include "SpartaCharacter.h"
#include "SpartaPlayerController.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFrameWork/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "SpartaGameState.h"

ASpartaCharacter::ASpartaCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 300.0f;
	SpringArmComp->bUsePawnControlRotation = true;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));
	OverHeadWidget->SetupAttachment(GetMesh(),FName("head"));
	OverHeadWidget->SetWidgetSpace(EWidgetSpace::Screen);

	NormalSpeed = 600.0f;
	SprintSpeedMultiplier = 1.7f;
	SprintSpeed = NormalSpeed * SprintSpeedMultiplier;

	CurrentSpeedMultiplier = 1.0f;

	MaxHealth = 100.0f;
	Health = MaxHealth;
}





void ASpartaCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateOverHeadHP();
	GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
}

void ASpartaCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SlowingTimerHandle);
		GetWorldTimerManager().ClearTimer(ReverseControlTimerHandle);
	}
}

void ASpartaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ASpartaPlayerController* PlayerController = Cast<ASpartaPlayerController>(GetController()))
		{
			if (PlayerController->MoveAction)
			{
				EnhancedInput->BindAction(
					PlayerController->MoveAction,
					ETriggerEvent::Triggered,
					this,
					&ASpartaCharacter::Move
				);
			}

			if (PlayerController->JumpAction)
			{
				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Triggered,
					this,
					&ASpartaCharacter::StartJump
				);

				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Completed,
					this,
					&ASpartaCharacter::StopJump
				);
			}

			if (PlayerController->LookAction)
			{
				EnhancedInput->BindAction(
					PlayerController->LookAction,
					ETriggerEvent::Triggered,
					this,
					&ASpartaCharacter::Look
				);
			}

			if (PlayerController->SprintAction)
			{
				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Triggered,
					this,
					&ASpartaCharacter::StartSprint
				);

				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Completed,
					this,
					&ASpartaCharacter::StopSprint
				);
			}
		}
	}
}


void ASpartaCharacter:: Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// [수정] 컨트롤 반전 상태라면 입력값 반전
		if (bIsReverseControl)
		{
			MovementVector *= -1.0f;
		}

		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.X);
		AddMovementInput(RightDirection, MovementVector.Y);
	}
}

void ASpartaCharacter::StartJump(const FInputActionValue& value)
{

	if (value.Get<bool>())
	{
		Jump();
	}
}

void ASpartaCharacter::StopJump(const FInputActionValue& value)
{
	if (!value.Get<bool>())
	{
		StopJumping();
	}
}

void ASpartaCharacter::Look(const FInputActionValue& value)
{
	FVector2D LookInput = value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);

}

void ASpartaCharacter::StartSprint(const FInputActionValue& value)
{
	bIsSprinting = true;
	UpdateMaxWalkSpeed();
}

void ASpartaCharacter::StopSprint(const FInputActionValue& value)
{
	bIsSprinting = false;
	UpdateMaxWalkSpeed();
}

void ASpartaCharacter::UpdateMaxWalkSpeed()
{
	float BaseSpeed = bIsSprinting ? SprintSpeed : NormalSpeed;

	// 기본 속도에 현재 배율(1.0 또는 0.5 등)을 곱해서 적용
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * CurrentSpeedMultiplier;
	}
}




float ASpartaCharacter::GetHealth() const
{
	return Health;
}

void ASpartaCharacter::AddHealth(float Amount)
{
	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	UpdateOverHeadHP();
}

float ASpartaCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
	UpdateOverHeadHP();

	if (Health <= 0.0f)
	{
		OnDeath();
	}

	return ActualDamage;
}

void ASpartaCharacter::OnDeath()
{
	GetWorldTimerManager().ClearTimer(SlowingTimerHandle);
	GetWorldTimerManager().ClearTimer(ReverseControlTimerHandle);

	ASpartaGameState* SpartaGameState = GetWorld() ? GetWorld()->GetGameState<ASpartaGameState>() : nullptr;
	if (SpartaGameState)
	{
		SpartaGameState->OnGameOver(false);
	}
}

void ASpartaCharacter::UpdateOverHeadHP()
{
	if (!OverHeadWidget) return;

	UUserWidget* OverHeadWidgetInstance = OverHeadWidget->GetUserWidgetObject();
	if (!OverHeadWidgetInstance) return;

	if (UProgressBar* HPBar = Cast<UProgressBar>(OverHeadWidgetInstance->GetWidgetFromName(TEXT("OverHeadHP"))))
	{
		// MaxHealth가 0이면 나눗셈 오류가 나므로 안전장치 추가
		float HPPercent = (MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f;

		HPBar->SetPercent(HPPercent);
	}
}

float ASpartaCharacter::GetSlowingRemainingTime() const
{
	if (GetWorldTimerManager().IsTimerActive(SlowingTimerHandle))
	{
		return GetWorldTimerManager().GetTimerRemaining(SlowingTimerHandle);
	}
	return 0.0f;
}

float ASpartaCharacter::GetReverseRemainingTime() const
{
	if (GetWorldTimerManager().IsTimerActive(ReverseControlTimerHandle))
	{
		return GetWorldTimerManager().GetTimerRemaining(ReverseControlTimerHandle);
	}
	return 0.0f;
}

void ASpartaCharacter::ApplySlowingEffect(float Duration, float Multiplier)
{
	CurrentSpeedMultiplier = Multiplier;
	UpdateMaxWalkSpeed(); // 즉시 속도 갱신

	
	GetWorldTimerManager().SetTimer(
		SlowingTimerHandle,
		this,
		&ASpartaCharacter::ResetSpeed,
		Duration, false);
}

void ASpartaCharacter::ApplyReverseControlEffect(float Duration)
{
	bIsReverseControl = true;

	
	GetWorldTimerManager().SetTimer(
		ReverseControlTimerHandle,
		this,
		&ASpartaCharacter::ResetControl,
		Duration,
		false);
}

void ASpartaCharacter::ResetSpeed()
{
	CurrentSpeedMultiplier = 1.0f;
	UpdateMaxWalkSpeed();
}

void ASpartaCharacter::ResetControl()
{
	bIsReverseControl = false;
}
