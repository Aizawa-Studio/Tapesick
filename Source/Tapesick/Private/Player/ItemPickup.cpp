#include "ItemPickup.h"

#include "Components/BoxComponent.h"
#include "Engine/EngineTypes.h"
#include "ItemDefinition.h"
#include "TapesickInventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogTapesickItemPickup, Log, All);

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);

	InteractionCollision->InitBoxExtent(FVector(40.0f));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(false);
	InteractionCollision->SetHiddenInGame(true);
}

bool AItemPickup::CanInteract_Implementation(APawn* Interactor) const
{
	if (bPickedUp || !Interactor || !IsValid(ItemDefinition))
	{
		return false;
	}

	const FGameplayTag ItemTag = ItemDefinition->ItemTag;
	if (!ItemTag.IsValid())
	{
		return false;
	}

	const UTapesickInventoryComponent* InventoryComponent =
		Interactor->FindComponentByClass<UTapesickInventoryComponent>();

	return IsValid(InventoryComponent) && !InventoryComponent->HasItem(ItemTag);
}

void AItemPickup::Interact_Implementation(APawn* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	UTapesickInventoryComponent* InventoryComponent =
		Interactor->FindComponentByClass<UTapesickInventoryComponent>();

	if (!IsValid(InventoryComponent))
	{
		return;
	}

	bPickedUp = true;

	const FGameplayTag ItemTag = ItemDefinition->ItemTag;
	if (!InventoryComponent->AddItem(ItemTag))
	{
		bPickedUp = false;
		return;
	}

	UE_LOG(
		LogTapesickItemPickup,
		Log,
		TEXT("%s: picked up %s"),
		*GetName(),
		*ItemTag.ToString()
	);

	UE_LOG(
		LogTapesickItemPickup,
		Log,
		TEXT("%s: HasItem(%s) = %s"),
		*GetName(),
		*ItemTag.ToString(),
		InventoryComponent->HasItem(ItemTag) ? TEXT("true") : TEXT("false")
	);

	BP_OnPickedUp(Interactor);

	if (bDestroyOnPickup)
	{
		Destroy();
		return;
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

FText AItemPickup::GetInteractionPrompt_Implementation() const
{
	if (bPickedUp || !IsValid(ItemDefinition))
	{
		return FText::GetEmpty();
	}

	return ItemDefinition->InteractionPrompt;
}