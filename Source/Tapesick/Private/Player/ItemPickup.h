#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "ItemPickup.generated.h"

class UBoxComponent;
class UItemDefinition;

UCLASS(Blueprintable)
class TAPESICK_API AItemPickup : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AItemPickup();

	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tapesick|Interaction")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Item")
	TObjectPtr<UItemDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tapesick|Item")
	bool bDestroyOnPickup = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tapesick|Item")
	bool bPickedUp = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "Tapesick|Item")
	void BP_OnPickedUp(APawn* Interactor);
};
