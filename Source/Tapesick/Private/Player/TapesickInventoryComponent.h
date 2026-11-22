#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "TapesickInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FTapesickInventoryItemChanged,
	FGameplayTag,
	ItemTag
);

UCLASS(BlueprintType, ClassGroup = (Tapesick), meta = (BlueprintSpawnableComponent))
class TAPESICK_API UTapesickInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTapesickInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Tapesick|Inventory")
	bool AddItem(FGameplayTag ItemTag);

	UFUNCTION(BlueprintPure, Category = "Tapesick|Inventory")
	bool HasItem(FGameplayTag ItemTag) const;

	UFUNCTION(BlueprintCallable, Category = "Tapesick|Inventory")
	bool RemoveItem(FGameplayTag ItemTag);

	UPROPERTY(BlueprintAssignable, Category = "Tapesick|Inventory")
	FTapesickInventoryItemChanged OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Tapesick|Inventory")
	FTapesickInventoryItemChanged OnItemRemoved;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tapesick|Inventory")
	FGameplayTagContainer Items;
};
