// Fill out your copyright notice in the Description page of Project Settings.


#include "MyUserWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"


void UMyUserWidget::Init(FString TypeStr,UObject** ValueObject,FString PropertyName)
{
	TypeStr = TypeStr.Replace(TEXT("*"),TEXT(""));
	
	if(TypeStr[0] == 'U')
	{
		Type = EType::Obejct;
	}
	else if(TypeStr[0] == 'A')
	{
		Type = EType::Actor;
	}
	
	TypeStr.RemoveAt(0);
	
	UObject* Object = FindObject<UObject>(ANY_PACKAGE,*TypeStr);

	Value = ValueObject;

	if(Type == EType::Obejct)
	{
		TArray<FAssetData> ObjectAssets;

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssetsByClass(*(Object->GetName()), ObjectAssets);

		for (FAssetData ObjectAsset : ObjectAssets)
		{
			ObjectSelectHelpers.Add(ObjectAsset.GetAsset());
			ObjectSelectName.Add(ObjectAsset.GetAsset()->GetName());
		}
	}

	InitSelectObject_UI(ObjectSelectName,PropertyName);
}

void UMyUserWidget::SetValue(FString Name)
{
	UObject* Object = FindObject<UObject>(ANY_PACKAGE,*Name);

	*Value = Object;
}
