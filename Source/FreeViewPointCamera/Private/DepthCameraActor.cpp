// Fill out your copyright notice in the Description page of Project Settings.
#include "DepthCameraActor.h"
#include "CineCameraComponent.h"
#include "Engine/SceneCapture.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

// Sets default values
ADepthCameraActor::ADepthCameraActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));
	SceneCapture->SetupAttachment(Camera);

	SceneCapture->TextureTarget = RenderTarget;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	
}

// Called when the game starts or when spawned
void ADepthCameraActor::BeginPlay()
{
	Super::BeginPlay();
	
	if(RenderTarget)
		RenderTarget->ResizeTarget(ResolutionX, ResolutionY);

}

// Called every frame
void ADepthCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

