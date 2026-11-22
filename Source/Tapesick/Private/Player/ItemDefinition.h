#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDefinition.generated.h"

UCLASS(BlueprintType)
class TAPESICK_API UItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (Categories = "Item"))
	FGameplayTag ItemTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText InteractionPrompt;
};
