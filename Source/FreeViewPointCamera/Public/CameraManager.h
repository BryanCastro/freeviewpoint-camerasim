// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraManager.generated.h"

class ADepthCameraActor;
class UMaterialInstance;

UENUM(BlueprintType)
enum class CameraSetupEnum : uint8
{
	SPHERE UMETA(DisplayName = "Sphere"),
	CUBE   UMETA(DisplayName = "Cube"),
};

UCLASS()
class FREEVIEWPOINTCAMERA_API ACameraManager : public AActor
{
	GENERATED_BODY()

	
public:	
	// Sets default values for this actor's properties
	ACameraManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	TArray<AActor*> DepthCameras;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Render Settings", meta=(AllowPrivateAccess="true"))
	int NumOfCameras = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Settings", meta = (AllowPrivateAccess = "true"))
	float SphereRadius = 1000.0f;

	void SpawnCamerasInSphere();
	void ClearSpawnedCameras();
	void SpawnCameras();

	UPROPERTY(EditAnywhere, Category = "Render Settings")
	TSubclassOf<AActor> CameraActorClassRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Settings", meta = (AllowPrivateAccess = "true"))
	CameraSetupEnum CurrentState=CameraSetupEnum::SPHERE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Settings")
	UMaterialInstance* DepthMaterialInstance;
	
	UFUNCTION(BlueprintCallable)
	void RenderImages();


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
