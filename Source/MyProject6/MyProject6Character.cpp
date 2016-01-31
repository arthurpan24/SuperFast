// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MyProject6.h"
#include "MyProject6Character.h"
#include "PaperFlipbookComponent.h"
#include "Components/TextRenderComponent.h"



DEFINE_LOG_CATEGORY_STATIC(SideScrollerCharacter, Log, All);
//////////////////////////////////////////////////////////////////////////
// AMyProject6Character

AMyProject6Character::AMyProject6Character()
{
	// Setup the assets
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UPaperFlipbook> RunningAnimationAsset;
		ConstructorHelpers::FObjectFinderOptional<UPaperFlipbook> IdleAnimationAsset;
		ConstructorHelpers::FObjectFinderOptional<UPaperFlipbook> BeginJumpAnimationAsset;
		ConstructorHelpers::FObjectFinderOptional<UPaperFlipbook> FollowThroughJumpAnimationAsset;
		FConstructorStatics()
			: RunningAnimationAsset(TEXT("/Game/SuperFastSprites/Run_FlipBook"))
			, IdleAnimationAsset(TEXT("/Game/SuperFastSprites/Ready_FlipBook"))
			, BeginJumpAnimationAsset(TEXT("/Game/SuperFastSprites/Jump_1_FlipBook"))
			, FollowThroughJumpAnimationAsset(TEXT("/Game/SuperFastSprites/Jump_2_Flipbook"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	RunningAnimation = ConstructorStatics.RunningAnimationAsset.Get();
	IdleAnimation = ConstructorStatics.IdleAnimationAsset.Get();
	BeginJumpAnimation = ConstructorStatics.BeginJumpAnimationAsset.Get();
	FollowThroughJumpAnimation = ConstructorStatics.FollowThroughJumpAnimationAsset.Get();

	GetSprite()->SetFlipbook(IdleAnimation);

	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);
	

	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->AttachTo(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 10.0f; //originally: 2.0f
	GetCharacterMovement()->AirControl = 0.80f; //originally 0.8 f
	GetCharacterMovement()->JumpZVelocity = 1500.f; //originally: 1000.0 f
	GetCharacterMovement()->GroundFriction = 3.0f; //originally 3.0 f
	GetCharacterMovement()->MaxWalkSpeed = 3000.0f; //originally 600 f
	GetCharacterMovement()->MaxFlySpeed = 600.0f; //originally: 600f

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
	// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
	// behavior on the edge of a ledge versus inclines by setting this to true or false
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

// 	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
// 	TextComponent->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.0f));
// 	TextComponent->SetRelativeLocation(FVector(35.0f, 5.0f, 20.0f));
// 	TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
// 	TextComponent->AttachTo(RootComponent);

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;

	acceptsMoveRightCommands = true;
	isSliding = false;
	isMovingLaterally = false;
	mayDoubleJump = true;
}

//////////////////////////////////////////////////////////////////////////
// Animation

void AMyProject6Character::UpdateAnimation()
{
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeed = PlayerVelocity.Size();

	// Are we moving or standing still?
	UPaperFlipbook* DesiredAnimation;
	if (GetCharacterMovement()->IsFalling() == true) {
		DesiredAnimation = (PlayerVelocity.Z < 0) ? FollowThroughJumpAnimation : BeginJumpAnimation;
	} else if (isMovingLaterally && !GetCharacterMovement()->IsFalling())
	{
		DesiredAnimation = RunningAnimation;
	}
	else {
		DesiredAnimation = IdleAnimation;
	}

	if( GetSprite()->GetFlipbook() != DesiredAnimation 	)
	{
		GetSprite()->SetFlipbook(DesiredAnimation);
	}
}

void AMyProject6Character::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateCharacter();	
}


//////////////////////////////////////////////////////////////////////////
// Input

void AMyProject6Character::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	InputComponent->BindAction("Jump", IE_Pressed, this, &AMyProject6Character::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &AMyProject6Character::StopJumping);
	InputComponent->BindAction("Slide", IE_Pressed, this, &AMyProject6Character::startSliding);
	InputComponent->BindAction("Slide", IE_Released, this, &AMyProject6Character::stopSliding);
	InputComponent->BindAction("Grapple", IE_Pressed, this, &AMyProject6Character::startGrappling);
	InputComponent->BindAction("Grapple", IE_Released, this, &AMyProject6Character::stopGrappling);
	InputComponent->BindAction("UseItem", IE_Released, this, &AMyProject6Character::useItem);
	InputComponent->BindAxis("MoveRight", this, &AMyProject6Character::MoveRight);

	InputComponent->BindTouch(IE_Pressed, this, &AMyProject6Character::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &AMyProject6Character::TouchStopped);
}

void AMyProject6Character::MoveRight(float Value)
{
	/*UpdateChar();*/

	// Apply the input to the character motion
	if (acceptsMoveRightCommands && Value != 0) {
		
		isMovingLaterally = true;
		AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
	} else {
		isMovingLaterally = false;

	}
}

void AMyProject6Character::Jump()
{
	UE_LOG(LogTemp, Warning, TEXT("jump called"));
	if (GetCharacterMovement()->IsFalling()) {
		UE_LOG(LogTemp, Warning, TEXT("falling"));
		if (mayDoubleJump == true) {
			UE_LOG(LogTemp, Warning, TEXT("may double jump was true"));

			APaperCharacter::Jump();
			mayDoubleJump = false;
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("not falling"));
		mayDoubleJump = true;
		APaperCharacter::Jump();
	}
} 

void AMyProject6Character::doubleJump()
{

}

void AMyProject6Character::startSliding()
{
	acceptsMoveRightCommands = false;
	isMovingLaterally = false;
	isSliding = true;
	// todo: implement slide animation and hit box change
}

void AMyProject6Character::stopSliding()
{
	acceptsMoveRightCommands = true;
	isSliding = true;
	// todo: implement slide animation exit and hit box change
}

void AMyProject6Character::startGrappling()
{
}

void AMyProject6Character::stopGrappling()
{

}

void AMyProject6Character::useItem()
{

}

void AMyProject6Character::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// jump on any touch
	Jump();
}

void AMyProject6Character::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	StopJumping();
}

void AMyProject6Character::UpdateCharacter()
{
	// Update animation to match the motion
	UpdateAnimation();

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();	
	float TravelDirection = PlayerVelocity.X;
	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}
