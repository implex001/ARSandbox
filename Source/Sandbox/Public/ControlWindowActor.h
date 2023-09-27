// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ControlWindowActor.generated.h"

UCLASS()
class SANDBOX_API AControlWindowActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AControlWindowActor();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UUserWidget* UserWidget;
	
	TSharedPtr<SWindow> Window;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	
	FOnWindowClosed WindowClosedDelegate;
	void OnWindowClose(const TSharedRef<SWindow>& Window);


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
