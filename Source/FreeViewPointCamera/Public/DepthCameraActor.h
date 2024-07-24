// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DepthCameraActor.generated.h"


class USceneCaptureComponent2D;
class UCineCameraComponent;
class UTextureRenderTarget2D;
class APostProcessVolume;
class UMaterialInstance;

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

	
	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;


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

	void RenderImages(bool bCaptureMask, UMaterialInstance* MetaHumanMaskMaterialInstance);
	void SaveRenderTargetToDisk(UTextureRenderTarget2D* RenderTarget, FString FileName, bool bIsDepth=false);
	void SetFarClipPlane(USceneCaptureComponent2D* SceneCapture2D);
	void SetFarClipDistance(float FarClipDistance);
	void SetDistanceFromLookTarget(float Distance);
	float GetDistanceFromLookTarget();
	void DisableCamera();
	void EnableCamera();
	FString GetCameraName();
	void SetCameraName(int index);
	void RenderImage(USceneCaptureComponent2D* SceneCapture2D, ETextureRenderTargetFormat TargetFormat, ESceneCaptureSource SceneCaptureSource, bool AlwaysPersisting = false);


	FTimerHandle UnusedHandle;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Render Settings")
	FString CameraName;

	void SetupSceneCaptureComponent(UWorld* World, USceneCaptureComponent2D* SceneCaptureComponent);
	void ApplyPostProcessSettingsToSceneCapture(UWorld* World, USceneCaptureComponent2D* SceneCaptureComponent);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	float TimeAccumulator;
	float FarClip;
	float NearClip = 0.1f;
	float DistanceFromLookTarget = 0.0f;

};
