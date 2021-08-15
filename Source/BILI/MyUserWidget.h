// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyUserWidget.generated.h"

/**
 * 
 */

UENUM()
enum EType
{
	Obejct = 0,
	Actor = 1,
};

UCLASS()
class BILI_API UMyUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Init(FString TypeStr,UObject** ValueObject,FString PropertyName);
	
	EType Type;

	UFUNCTION(BlueprintImplementableEvent)
	void InitSelectObject_UI(const TArray<FString>& ObjectSelectNames,const FString& Name);
	
	UFUNCTION(BlueprintCallable)
	void SetValue(FString Name);

	UObject** Value;
private:
	TArray<FAssetData> CanSelectObject;
	TArray<UObject*> ObjectSelectHelpers;
	TArray<FString> ObjectSelectName;

	
};
