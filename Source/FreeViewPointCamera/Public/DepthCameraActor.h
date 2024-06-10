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
	USceneCaptureComponent2D* SceneRGBDCapture;
	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* SceneRGBCapture;
	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* SceneMaskCapture;
	
	UPROPERTY()
	UTextureRenderTarget2D* RenderRGBDTarget;
	UPROPERTY()
	UTextureRenderTarget2D* RenderRGBTarget;
	UPROPERTY()
		UTextureRenderTarget2D* RenderMaskTarget;

	UPROPERTY(EditAnywhere, Category = "Render Settings")
	int ResolutionX=256;
	UPROPERTY(EditAnywhere, Category = "Render Settings")
	int ResolutionY=256;
	UPROPERTY(EditAnywhere, Category = "Render Settings")
	int FramesToCapture = 10;

	int CurrentFramesCaptured = 0;

	UFUNCTION()
	const FVector GetPosition();
	UFUNCTION()
	const FRotator GetRotation();

	void RenderImages();
	void SaveRenderTargetToDisk(UTextureRenderTarget2D* RenderTarget, FString FileName, bool bIsDepth=false);
	void SetFarClipPlane(USceneCaptureComponent2D* SceneCapture);
	void SetFarClipDistance(float FarClipDistance);
	void SetDistanceFromLookTarget(float Distance);
	float GetDistanceFromLookTarget();
	FString GetCameraName();
	void SetCameraName(int index);


	UPROPERTY()
	UMaterialInstance* DepthMaterialInstance;
	FTimerHandle UnusedHandle;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Render Settings")
	FString CameraName;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	float TimeAccumulator;
	float FarClip;
	float NearClip = 0.1f;
	float DistanceFromLookTarget = 0.0f;

};
