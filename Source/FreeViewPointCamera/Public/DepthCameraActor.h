// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DepthCameraActor.generated.h"

class USceneCaptureComponent2D;
class UCineCameraComponent;
class UTextureRenderTarget2D;

UCLASS()
class FREEVIEWPOINTCAMERA_API ADepthCameraActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADepthCameraActor();

	UPROPERTY(VisibleAnywhere)
	UCineCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* SceneCapture;

	UPROPERTY(EditAnywhere, Category = "Render Settings")
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY(EditAnywhere, Category = "Render Settings")
	int ResolutionX=256;
	UPROPERTY(EditAnywhere, Category = "Render Settings")
	int ResolutionY=256;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
