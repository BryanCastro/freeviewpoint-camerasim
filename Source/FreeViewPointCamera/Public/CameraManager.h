// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraManager.generated.h"

class ADepthCameraActor;
class UMaterialInstance;
class AStaticMeshActor;
class ACharacter;
class APostProcessVolume;

UENUM(BlueprintType)
enum class CameraSetupEnum : uint8
{
	SPHERE UMETA(DisplayName = "Sphere"),
	CIRCLE_X   UMETA(DisplayName = "CIRCLE_PLANE_X"),
	CIRCLE_Y   UMETA(DisplayName = "CIRCLE_PLANE_Y"),
	CIRCLE_Z   UMETA(DisplayName = "CIRCLE_PLANE_Z"),
	CIRCLE_XY   UMETA(DisplayName = "CIRCLE_PLANE_XY"),
	CIRCLE_XZ  UMETA(DisplayName = "CIRCLE_PLANE_XZ"),
	CIRCLE_YZ  UMETA(DisplayName = "CIRCLE_PLANE_YZ"),
	HEMI_SPHERE UMETA(DisplayName = "SEMI_SPHERE"),
	STEREO_HEMISPHERE UMETA(DisplayName = "STEREO_HEMISPHERE"),
	CUBE_STEREO UMETA(DisplayName = "CUBE_STEREO"),
	HEXAGON UMETA(DisplayName = "HEXAGON")
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
	#if WITH_EDITOR
    	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	TArray<AActor*> DepthCameras;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Settings", meta = (AllowPrivateAccess = "true"))
	float CubeSize = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Settings", meta = (AllowPrivateAccess = "true"))
	int CubeRows = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Settings", meta = (AllowPrivateAccess = "true"))
	int CubeCols = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Settings", meta = (AllowPrivateAccess = "true"))
	float BaseCubeHeightOffset = 0.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sphere Settings", meta=(AllowPrivateAccess="true"))
	int NumOfCameras = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sphere Settings", meta = (AllowPrivateAccess = "true"))
	float SphereRadius = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sphere Settings", meta = (AllowPrivateAccess = "true"))
	bool bRandomRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings", meta = (AllowPrivateAccess = "true"))
	float FarClipDistance=1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexagon Settings", meta = (AllowPrivateAccess = "true"))
	float HexagonHeight = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexagon Settings", meta = (AllowPrivateAccess = "true"))
	float HexagonWidth = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexagon Settings", meta = (AllowPrivateAccess = "true"))
	float BaseHexagonHeightOffset = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexagon Settings", meta = (AllowPrivateAccess = "true"))
	int HexagonRows = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexagon Settings", meta = (AllowPrivateAccess = "true"))
	int HexagonCols = 1;

	void SpawnCamerasInSphere();
	void SpawnCamerasInHemisphere();
	void SpawnCamerasInCircle(FString axis);
	void ClearSpawnedCameras();
	void SpawnCameras();
	void SpawnStereoCamerasInHemisphere();
	void SpawnCamerasOnCubeWalls();
	void SpawnCamerasOnHexagonWalls();

	void AddCameraToList(FVector SpawnLocation, FRotator SpawnRotation, FActorSpawnParameters SpawnParams);
	void SpawnCameraAtLocation(FVector Location, FRotator Rotation);


	UPROPERTY(EditAnywhere, Category = "Render Settings")
	TSubclassOf<AActor> CameraActorClassRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Settings", meta = (AllowPrivateAccess = "true"))
	CameraSetupEnum CurrentState=CameraSetupEnum::SPHERE;
	
	UFUNCTION(BlueprintCallable)
	void RenderImages();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Settings")
	bool bCaptureMask = true;

	int32 NumOfCamerasInScene = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Settings")
	UMaterialInstance* MetaHumanMaskMaterialInstance;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
