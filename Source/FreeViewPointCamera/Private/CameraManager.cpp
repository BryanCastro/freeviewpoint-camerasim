// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraManager.h"
#include "DepthCameraActor.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "CineCameraComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/PostProcessVolume.h"


// Sets default values
ACameraManager::ACameraManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	if (!CameraActorClassRef) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: Failed to Find Camera Blueprint Class Reference!")));
	}
	if (!MetaHumanMaskMaterialInstance) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: Failed to Find MetaHumanMaskMaterialInstance Reference!")));
	}

}

#if WITH_EDITOR

void ACameraManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property!=nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, NumOfCameras)) {
		
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, SphereRadius)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, CurrentState)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, NumOfCamerasInScene)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, Rows)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, Cols)) {
		SpawnCameras();
	}
}

#endif

// Called when the game starts or when spawned
void ACameraManager::BeginPlay()
{
	Super::BeginPlay();
	
	/*Do Not Spawn Cameras on BeginPlay! Use ones spawned in editor
	if (CameraActorClassRef) {
		SpawnCameras();
	}

	else {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: Failed to Load Camera Blueprint Class Reference!")));
	}
		*/

	// Get all ADepthCameraActor in World and store in TArray called DepthCameras
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADepthCameraActor::StaticClass(), DepthCameras);
	SpawnCameras();
	for (auto Camera : DepthCameras) {
		ADepthCameraActor* DepthCamera = Cast<ADepthCameraActor>(Camera);
		DepthCamera->SetFarClipDistance(DepthCamera->GetDistanceFromLookTarget() * 3);
		DepthCamera->SetFarClipPlane(DepthCamera->SceneRGBCapture);
		DepthCamera->SetFarClipDistance(DepthCamera->GetDistanceFromLookTarget() * 2);
		DepthCamera->SetFarClipPlane(DepthCamera->SceneRGBDCapture);
		DepthCamera->SetFarClipDistance(DepthCamera->GetDistanceFromLookTarget() * 3);
		DepthCamera->SetFarClipPlane(DepthCamera->SceneMaskCapture);

		DepthCamera->SetHidden(true);
		//DepthCamera->SetCameraName(CameraName);
	}
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: Num of Cameras: %d"), DepthCameras.Num()));

}

void ACameraManager::SpawnCameras() {
	if (!CameraActorClassRef) return;

	ClearSpawnedCameras();

	if(!BP_SphereRef) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: No BP_SphereRef Set! Cameras will not spawn")));
		return;
	}

	float SphereScale = (SphereRadius * 0.0225f);
	BP_SphereRef->SetActorScale3D(FVector(SphereScale, SphereScale, SphereScale));

	switch (CurrentState) {
		case CameraSetupEnum::SPHERE:
			SpawnCamerasInSphere();
			break;
		case CameraSetupEnum::CIRCLE_X:
			SpawnCamerasInCircle("X");	
			break;
		case CameraSetupEnum::CIRCLE_Y:
			SpawnCamerasInCircle("Y");
			break;
		case CameraSetupEnum::CIRCLE_Z:
			SpawnCamerasInCircle("Z");
			break;
		case CameraSetupEnum::CIRCLE_XY:
			SpawnCamerasInCircle("X");
			SpawnCamerasInCircle("Y");
			break;
		case CameraSetupEnum::CIRCLE_XZ:
			SpawnCamerasInCircle("X");
			SpawnCamerasInCircle("Z");
			break;
		case CameraSetupEnum::CIRCLE_YZ:
			SpawnCamerasInCircle("Y");
			SpawnCamerasInCircle("Z");
			break;
		case CameraSetupEnum::HEMI_SPHERE:
			SpawnCamerasInHemisphere();
			break;
		case CameraSetupEnum::STEREO_HEMISPHERE:
			SpawnStereoCamerasInHemisphere();
			break;
		case CameraSetupEnum::CUBE_STEREO:
			SpawnCamerasOnCubeWalls();
			break;
		default:
			break;
	}

}
void ACameraManager::SpawnCamerasOnCubeWalls() {
	FVector WallNormals[6] = {FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, 1, 0), FVector(0, -1, 0), FVector(0, 0, 1), FVector(0, 0, -1)};
    FVector UpVectors[6] = {FVector(0, 0, 1), FVector(0, 0, 1), FVector(0, 0, 1), FVector(0, 0, 1), FVector(0, 1, 0), FVector(0, -1, 0)};
    FVector RightVectors[6] = {FVector(0, 1, 0), FVector(0, -1, 0), FVector(-1, 0, 0), FVector(1, 0, 0), FVector(1, 0, 0), FVector(-1, 0, 0)};

    float RowSpacing = CubeSize / Rows;
    float ColSpacing = CubeSize / Cols;

    for (int wall = 0; wall < 6; ++wall) {
        FVector WallCenter = WallNormals[wall] * (CubeSize / 2);
        FVector StartPosition = WallCenter - (RightVectors[wall] * (CubeSize / 2 - ColSpacing / 2)) - (UpVectors[wall] * (CubeSize / 2 - RowSpacing / 2));

        for (int row = 0; row < Rows; ++row) {
            for (int col = 0; col < Cols; ++col) {
                FVector SpawnLocation = StartPosition + (RightVectors[wall] * ColSpacing * col) + (UpVectors[wall] * RowSpacing * row);
                // Adjust SpawnRotation to make the camera face straight ahead relative to the wall
                FRotator SpawnRotation = WallNormals[wall].Rotation()*-1;

                SpawnCameraAtLocation(SpawnLocation, SpawnRotation);
            }
        }
    }
}

void ACameraManager::SpawnCameraAtLocation(FVector Location, FRotator Rotation){
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	AActor* NewCamera = GetWorld()->SpawnActor<AActor>(CameraActorClassRef, Location, Rotation, SpawnParams);
	if (NewCamera) {
		AddCameraToList(Location, Rotation, SpawnParams);
	}
}

void ACameraManager::SpawnCamerasInSphere() {
	float radius, theta;
	FVector SpawnLocation;
	FRotator SpawnRotation;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	for (int32 i = 0; i < NumOfCameras; i++)
	{
		float phi = (sqrt(5.0) - 1.0) / 2.0; // golden ratio
		float y = 1 - (i / float(NumOfCameras - 1)) * 2; // y goes from 1 to -1
		radius = sqrt(1 - y * y); // radius at y

		theta = 2 * PI * phi * i;

		float x = cos(theta) * radius;
		float z = sin(theta) * radius;

		SpawnLocation = FVector(x, y, z) * SphereRadius;
		SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, FVector::ZeroVector);

		AddCameraToList(SpawnLocation, SpawnRotation, SpawnParams);
	}
}

void ACameraManager::AddCameraToList(FVector SpawnLocation, FRotator SpawnRotation, FActorSpawnParameters SpawnParams) {
	float NewDistance = FMath::FRandRange(SphereRadius, SphereRadius * 10);
	if (bRandomRadius)
		SpawnLocation = (SpawnLocation / SphereRadius) * NewDistance;
	
	AActor* NewCamera = GetWorld()->SpawnActor<AActor>(CameraActorClassRef, SpawnLocation, SpawnRotation, SpawnParams);
	if (NewCamera) {
		ADepthCameraActor* DepthCamera = Cast<ADepthCameraActor>(NewCamera);
		DepthCamera->SetCameraName(NumOfCamerasInScene);
		
		if(bRandomRadius)
			DepthCamera->SetDistanceFromLookTarget(NewDistance);
		else
			DepthCamera->SetDistanceFromLookTarget(SphereRadius);

		DepthCamera->Camera->CurrentAperture = 22.0f;
		DepthCamera->SceneRGBDCapture->HiddenActors.Add(BP_SphereRef);
		DepthCameras.Add(NewCamera);
	
		NumOfCamerasInScene++;
	}
}

void ACameraManager::SpawnCamerasInHemisphere() {
 
   const float Phi = (1 + sqrt(5)) / 2; // Golden ratio
    for (int32 i = 0; i < NumOfCameras; i++)
    {
        float theta = 2 * PI * i / Phi;
        float z = 1 - (i / (NumOfCameras - 1.0f)); // z goes from 1 to 0
        float radius = sqrt(i) / sqrt(NumOfCameras); // radius increases as i increases

        float x = cos(theta) * radius;
        float y = sin(theta) * radius;

        FVector SpawnLocation = FVector(x, y, z) * SphereRadius;
        FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, FVector::ZeroVector);

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator();

        AddCameraToList(SpawnLocation, SpawnRotation, SpawnParams);
    }
}

void ACameraManager::SpawnStereoCamerasInHemisphere() {
    const float Phi = (1 + sqrt(5)) / 2; // Golden ratio
    const float EyeSeparation = 3.25f; // Interocular distance

    for (int32 i = 0; i < NumOfCameras; i++)
    {
        float theta = 2 * PI * i / Phi;
        float z = 1 - (i / (NumOfCameras - 1.0f)); // z goes from 1 to 0
        float radius = sqrt(i) / sqrt(NumOfCameras); // radius increases as i increases

        float x = cos(theta) * radius;
        float y = sin(theta) * radius;

        FVector BaseLocation = FVector(x, y, z) * SphereRadius;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

        // Spawn left camera
        FVector LeftEyeLocation = BaseLocation + FVector(0, -EyeSeparation / 2, 0);
        FRotator LeftEyeRotation = UKismetMathLibrary::FindLookAtRotation(LeftEyeLocation, FVector::ZeroVector);
        AddCameraToList(LeftEyeLocation, LeftEyeRotation, SpawnParams);

        // Spawn right camera
        FVector RightEyeLocation = BaseLocation + FVector(0, EyeSeparation / 2, 0);
        FRotator RightEyeRotation = UKismetMathLibrary::FindLookAtRotation(RightEyeLocation, FVector::ZeroVector);
        AddCameraToList(RightEyeLocation, RightEyeRotation, SpawnParams);
    }
}

void ACameraManager::SpawnCamerasInCircle(FString axis) {
     for (int32 i = 0; i < NumOfCameras; i++)
    {
        float theta = 2 * PI * i / NumOfCameras; // angle for each camera

        // Calculate x, y, and z coordinates based on the specified axis.
        float x = 0, y = 0, z = 0;
        if (axis == "X") {
            y = cos(theta);
            z = sin(theta);
        } else if (axis == "Y") {
            x = cos(theta);
            z = sin(theta);
        } else if (axis == "Z") {
            x = cos(theta);
            y = sin(theta);
        }

        FVector SpawnLocation = FVector(x, y, z) * SphereRadius;
        FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, FVector::ZeroVector);

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator();

		AddCameraToList(SpawnLocation, SpawnRotation, SpawnParams);
    }
}


void ACameraManager::ClearSpawnedCameras() {
	TArray<AActor*> SpawnedCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADepthCameraActor::StaticClass(), SpawnedCameras);
	for (AActor* Camera : SpawnedCameras) {
		if (Camera) {
			Camera->Destroy();
		}
	}

	DepthCameras.Empty();
	NumOfCamerasInScene = 0;
}


// Called every frame
void ACameraManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACameraManager::RenderImages() {
    // Create a new JSON object.
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: Rendering Images for Num of Cameras: %d"), DepthCameras.Num()));

	for (auto camera : DepthCameras) {
		ADepthCameraActor* DepthCamera = Cast<ADepthCameraActor>(camera);
		DepthCamera->RenderImages(bCaptureMask, MetaHumanMaskMaterialInstance);

		// Get the camera's world position and rotation.
		FVector Position = camera->GetActorLocation();
		FRotator Rotation = camera->GetActorRotation();

		float Tolerance = 0.00001f;
		Rotation.Pitch = FMath::IsNearlyZero(Rotation.Pitch, Tolerance) ? 0.0f : Rotation.Pitch;
		Rotation.Yaw = FMath::IsNearlyZero(Rotation.Yaw, Tolerance) ? 0.0f : Rotation.Yaw;
		Rotation.Roll = FMath::IsNearlyZero(Rotation.Roll, Tolerance) ? 0.0f : Rotation.Roll;

		// Create new JSON objects for the position and rotation.
		TSharedPtr<FJsonObject> PositionObject = MakeShareable(new FJsonObject);
		PositionObject->SetNumberField("X", Position.X);
		PositionObject->SetNumberField("Y", Position.Y);
		PositionObject->SetNumberField("Z", Position.Z);

		TSharedPtr<FJsonObject> RotationObject = MakeShareable(new FJsonObject);
		RotationObject->SetNumberField("P", Rotation.Pitch);
		RotationObject->SetNumberField("Y", Rotation.Yaw);
		RotationObject->SetNumberField("R", Rotation.Roll);

		// Get the Filmback settings.
		FCameraFilmbackSettings FilmbackSettings = DepthCamera->Camera->Filmback;
		float SensorWidth = FilmbackSettings.SensorWidth;
		float SensorHeight = FilmbackSettings.SensorHeight;
		float SensorAspectRatio = FilmbackSettings.SensorAspectRatio;


		// Create a new JSON object for the Filmback settings.
		TSharedPtr<FJsonObject> FilmbackObject = MakeShareable(new FJsonObject);
		FilmbackObject->SetNumberField("SensorWidth", SensorWidth);
		FilmbackObject->SetNumberField("SensorHeight", SensorHeight);
		FilmbackObject->SetNumberField("SensorAspectRatio", SensorAspectRatio);


		FCameraLensSettings CameraLensSettings = DepthCamera->Camera->LensSettings;

		// Get the Lens settings.
		float MinFocalLength = CameraLensSettings.MinFocalLength;
		float MaxFocalLength = CameraLensSettings.MaxFocalLength;
		float FieldOfView = DepthCamera->Camera->GetHorizontalFieldOfView();
		// Add other lens settings here...

		// Create a new JSON object for the Lens settings.
		TSharedPtr<FJsonObject> LensObject = MakeShareable(new FJsonObject);
		LensObject->SetNumberField("MinFocalLength", MinFocalLength);
		LensObject->SetNumberField("MaxFocalLength", MaxFocalLength);
		LensObject->SetNumberField("Horizontal Field of View", FieldOfView);
		// Add other lens settings here...

		TSharedPtr<FJsonObject> OtherObject = MakeShareable(new FJsonObject);
		OtherObject->SetNumberField("Distance from Scene(cm)", SphereRadius);
		OtherObject->SetNumberField("Near Clip Plane", 0.1f);
		OtherObject->SetNumberField("Far Clip Plane", SphereRadius * 2);
		
		// Create a new JSON object for this camera.
		TSharedPtr<FJsonObject> CameraObject = MakeShareable(new FJsonObject);
		CameraObject->SetObjectField("World Position", PositionObject);
		CameraObject->SetObjectField("World Rotation", RotationObject);
		CameraObject->SetObjectField("Filmback", FilmbackObject);
		CameraObject->SetObjectField("Lens", LensObject);
		CameraObject->SetObjectField("Other", OtherObject);

		// Add the camera object to the main JSON object.
		JsonObject->SetObjectField(DepthCamera->GetCameraName(), CameraObject);
		
		FString CameraName = DepthCamera->GetCameraName();
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: Camera Name: %s"), *CameraName));
	}
    // Convert the JSON object to a string.
    FString JsonString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Save the JSON string to a file.
    FFileHelper::SaveStringToFile(JsonString, *(FPaths::ProjectDir() + FString("/Images/CameraData.json")));
}