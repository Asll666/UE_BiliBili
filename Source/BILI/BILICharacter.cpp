// Copyright Epic Games, Inc. All Rights Reserved.

#include "BILICharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "Trace/Detail/Field.h"

//////////////////////////////////////////////////////////////////////////
// ABILICharacter

ABILICharacter::ABILICharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++
}

//////////////////////////////////////////////////////////////////////////
// Input

void ABILICharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABILICharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABILICharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ABILICharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ABILICharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ABILICharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ABILICharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ABILICharacter::OnResetVR);
}


TArray<UUserWidget*> ABILICharacter::GetProperty(TSubclassOf<UMyUserWidget> ObjectWidgetClass)
{
	TArray<UUserWidget*> toR;
	
	for(TFieldIterator<FProperty> It(GetClass()); It;++It)
	{
		if(It->HasAnyPropertyFlags(CPF_SaveGame) && It->GetNameCPP() != "bCanBeDamaged")
		{
			FString Name = It->GetCPPType();
			
			if(Name[0] == 'U' || Name[0] == 'A')
			{
				UMyUserWidget* ObjectWidget =
				Cast<UMyUserWidget>(UWidgetBlueprintLibrary::Create(this,ObjectWidgetClass,UGameplayStatics::GetPlayerController(this,0)));

				ObjectWidget->Init(Name,It->ContainerPtrToValuePtr<UObject*>(this),It->GetNameCPP());
			
				toR.Add(ObjectWidget);
			}
		}
	}

	return toR;
}

void ABILICharacter::SaveMap()
{
	FString SaveJsonStr;
	
	TSharedRef<TJsonWriter<TCHAR,TPrettyJsonPrintPolicy<TCHAR>>> JsonWrite = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&SaveJsonStr);
	JsonWrite->WriteObjectStart();

	JsonWrite->WriteArrayStart("Items");
	
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this,ACharacter::StaticClass(),Actors);

	for (int i = 0; i < Actors.Num(); ++i)
	{
		AActor* OnFor = Actors[i];
		
		TSharedPtr<FJsonObject> toSave = MakeShareable(new FJsonObject);
		FJsonObjectConverter::UStructToJsonObject(OnFor->GetClass(),OnFor,toSave.ToSharedRef(),CPF_SaveGame,0);
		FJsonSerializer::Serialize(toSave.ToSharedRef(), JsonWrite, false);
	}

	JsonWrite->WriteArrayEnd();
	JsonWrite->WriteObjectEnd();
	JsonWrite->Close();

	FFileHelper::SaveStringToFile(SaveJsonStr,*(FPaths::ProjectContentDir() + "Maps/MyMap.json"));
}

void ABILICharacter::M_LoadMap()
{
	FString LoadJsonStr;
	FFileHelper::LoadFileToString(LoadJsonStr, *(FPaths::ProjectContentDir() + "Maps/MyMap.json"));

	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(LoadJsonStr);
	TSharedPtr<FJsonObject> JsonObject;
	FJsonSerializer::Deserialize(JsonReader, JsonObject);
	
	TArray<TSharedPtr<FJsonValue>> OutArray = JsonObject->GetArrayField("Items");

	for (int i = 0; i < OutArray.Num(); ++i)
	{
		TSharedPtr<FJsonObject> Item = OutArray[i]->AsObject();

		FJsonObjectConverter::JsonObjectToUStruct(Item.ToSharedRef(), this->GetClass(), this);
	}
}

void ABILICharacter::CallMyFun()
{
	void* Malloc = FMemory::Malloc(sizeof(float) * 3 + sizeof(FVector));

	float x = 10;
	float y = 20;
	float z = 30;

	FMemory::Memcpy(Malloc,&x,sizeof(float));
	FMemory::Memcpy((char*)Malloc + sizeof(float),&y,sizeof(float));
	FMemory::Memcpy((char*)Malloc + sizeof(float) * 2,&z,sizeof(float));

	UFunction* MakeVectorFun = FindFunction("MakeVector");
	

	ProcessEvent(MakeVectorFun,Malloc);

	FVector* Vector = (FVector*)((char*)Malloc + (sizeof(float) * 3));
	if(Vector)
	{
		UE_LOG(LogTemp,Log,TEXT("%s"),*Vector->ToString())
	}
}

FVector ABILICharacter::MakeVector(float x, float y, float z)
{
	return FVector(x,y,z);
}

void ABILICharacter::OnResetVR()
{
	// If BILI is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in BILI.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ABILICharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ABILICharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ABILICharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ABILICharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ABILICharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ABILICharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
