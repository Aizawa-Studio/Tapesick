#include "TapesickInventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogTapesickInventory, Log, All);

UTapesickInventoryComponent::UTapesickInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UTapesickInventoryComponent::AddItem(FGameplayTag ItemTag)
{
	if (!ItemTag.IsValid() || HasItem(ItemTag))
	{
		return false;
	}

	Items.AddTag(ItemTag);

	UE_LOG(
		LogTapesickInventory,
		Log,
		TEXT("Item added: %s"),
		*ItemTag.ToString()
	);

	OnItemAdded.Broadcast(ItemTag);
	return true;
}

bool UTapesickInventoryComponent::HasItem(FGameplayTag ItemTag) const
{
	return ItemTag.IsValid() && Items.HasTagExact(ItemTag);
}

bool UTapesickInventoryComponent::RemoveItem(FGameplayTag ItemTag)
{
	if (!ItemTag.IsValid() || !HasItem(ItemTag))
	{
		return false;
	}

	Items.RemoveTag(ItemTag);

	UE_LOG(
		LogTapesickInventory,
		Log,
		TEXT("Item removed: %s"),
		*ItemTag.ToString()
	);

	OnItemRemoved.Broadcast(ItemTag);
	return true;
}