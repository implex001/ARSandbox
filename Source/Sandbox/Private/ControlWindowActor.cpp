// Fill out your copyright notice in the Description page of Project Settings.


#include "ControlWindowActor.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/UserInterfaceSettings.h"


// Sets default values
AControlWindowActor::AControlWindowActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AControlWindowActor::BeginPlay()
{
	Super::BeginPlay();
	
	Window = SNew(SWindow)
		.AutoCenter(EAutoCenter::None)
		.Title(FText::FromString(TEXT("Control Window")))
		.IsInitiallyMaximized(false)
		.ScreenPosition(FVector2D(0, 0))
		.ClientSize(FVector2D(900, 800))
		.CreateTitleBar(true)
		.SizingRule(ESizingRule::UserSized)
		.SupportsMaximize(false)
		.SupportsMinimize(true)
		.HasCloseButton(true);

	FGeometry WindowGeometry = Window->GetWindowGeometryInScreen();

	float DPIFactor = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())
		   ->GetDPIScaleBasedOnSize(FIntPoint(WindowGeometry.GetAbsoluteSize().X,WindowGeometry.GetAbsoluteSize().Y));

	Window->SetDPIScaleFactor(DPIFactor);
	
	FSlateApplication & SlateApp = FSlateApplication::Get();

	SlateApp.AddWindow(Window.ToSharedRef(), true);
 
	// Make sure our new window doesn't hide behind some others
	Window.Get()->BringToFront(true);
 
	// Bind a method to the OnClosed event, to clean up should the user, well, close the window
	WindowClosedDelegate.BindUObject(this, &AControlWindowActor::OnWindowClose);
	Window->SetOnWindowClosed(WindowClosedDelegate);
	
	// Finally fetch the Slate widget from the UMG one
	if (UserWidget != nullptr)
	{
		TSharedRef<SWidget> SlateWidget = UserWidget->TakeWidget();
		Window->SetContent(SlateWidget);
	}
	
}

void AControlWindowActor::BeginDestroy()
{
	Super::BeginDestroy();
	if (Window.IsValid())
	{
		Window->RequestDestroyWindow();
		OnWindowClose(Window.ToSharedRef());
	}
}

void AControlWindowActor::OnWindowClose(const TSharedRef<SWindow>& EventWindow)
{
	UserWidget->RemoveFromParent();
	Window.Reset();
}

// Called every frame
void AControlWindowActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

