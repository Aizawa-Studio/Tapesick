#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

class APawn;

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class TAPESICK_API IInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tapesick|Interaction")
	bool CanInteract(APawn* Interactor) const;

	virtual bool CanInteract_Implementation(APawn* Interactor) const
	{
		return true;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tapesick|Interaction")
	void Interact(APawn* Interactor);

	virtual void Interact_Implementation(APawn* Interactor)
	{
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tapesick|Interaction")
	FText GetInteractionPrompt() const;

	virtual FText GetInteractionPrompt_Implementation() const
	{
		return FText::GetEmpty();
	}
};