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
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, CubeRows)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, CubeCols)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, CubeSize)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, BaseCubeHeightOffset)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, HexagonHeight)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, HexagonWidth)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, BaseHexagonHeightOffset)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, HexagonRows)) {
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, HexagonCols)) {
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
		DepthCamera->SetFarClipDistance(FarClipDistance);
		DepthCamera->SetFarClipPlane(DepthCamera->SceneCapture);
		DepthCamera->DisableCamera();

		//DepthCamera->SetCameraName(CameraName);
	}
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: Num of Cameras: %d"), DepthCameras.Num()));

}

void ACameraManager::SpawnCameras() {
	if (!CameraActorClassRef) return;

	ClearSpawnedCameras();




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
		case CameraSetupEnum::HEXAGON:
			SpawnCamerasOnHexagonWalls();
			break;
		default:
			break;
	}

}

void ACameraManager::SpawnCamerasOnCubeWalls() {
	FVector WallNormals[6] = { FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, 1, 0), FVector(0, -1, 0), FVector(0, 0, 1), FVector(0, 0, -1) };
	FVector UpVectors[6] = { FVector(0, 0, 1), FVector(0, 0, 1), FVector(0, 0, 1), FVector(0, 0, 1), FVector(0, 1, 0), FVector(0, -1, 0) };
	FVector RightVectors[6] = { FVector(0, 1, 0), FVector(0, -1, 0), FVector(-1, 0, 0), FVector(1, 0, 0), FVector(1, 0, 0), FVector(-1, 0, 0) };

	float RowSpacing = CubeSize / CubeRows;
	float ColSpacing = CubeSize / CubeCols;

	// Adjust the loop to skip the last element, which represents the lower wall
	for (int wall = 0; wall < 5; ++wall) {
		FVector WallCenter = WallNormals[wall] * (CubeSize / 2);
		FVector StartPosition = WallCenter - (RightVectors[wall] * (CubeSize / 2 - ColSpacing / 2)) - (UpVectors[wall] * (CubeSize / 2 - RowSpacing / 2));

		for (int row = 0; row < CubeRows; ++row) {
			for (int col = 0; col < CubeCols; ++col) {
				FVector SpawnLocation = StartPosition + (RightVectors[wall] * ColSpacing * col) + (UpVectors[wall] * RowSpacing * row);
				FRotator SpawnRotation = WallNormals[wall].Rotation();

				SpawnLocation.Z += BaseCubeHeightOffset;

				// Rotate the camera to face outward from the cube wall
				SpawnRotation.Yaw += 180.0f;

				// Special adjustment for cameras on the upper wall
				if (wall == 4) { // Upper wall
					SpawnRotation.Pitch += 180.0f;
				}

				SpawnCameraAtLocation(SpawnLocation, SpawnRotation);
			}
		}
	}

}

void ACameraManager::SpawnCamerasOnHexagonWalls() {

	FarClipDistance = HexagonWidth;

	FVector WallNormals[8] = {
		FVector(FMath::Cos(FMath::DegreesToRadians(0)), FMath::Sin(FMath::DegreesToRadians(0)), 0),
		FVector(FMath::Cos(FMath::DegreesToRadians(60)), FMath::Sin(FMath::DegreesToRadians(60)), 0),
		FVector(FMath::Cos(FMath::DegreesToRadians(120)), FMath::Sin(FMath::DegreesToRadians(120)), 0),
		FVector(FMath::Cos(FMath::DegreesToRadians(180)), FMath::Sin(FMath::DegreesToRadians(180)), 0),
		FVector(FMath::Cos(FMath::DegreesToRadians(240)), FMath::Sin(FMath::DegreesToRadians(240)), 0),
		FVector(FMath::Cos(FMath::DegreesToRadians(300)), FMath::Sin(FMath::DegreesToRadians(300)), 0),
		FVector(0, 0, 1), // Top
		FVector(0, 0, -1) // Bottom
	};
	FVector UpVectors[8] = {
		FVector(0, 0, 1), FVector(0, 0, 1), FVector(0, 0, 1),
		FVector(0, 0, 1), FVector(0, 0, 1), FVector(0, 0, 1),
		FVector(0, 1, 0), // Top
		FVector(0, -1, 0) // Bottom
	};
	// Right vectors are calculated as cross product of wall normal and up vector for each wall
	FVector RightVectors[8];
	for (int i = 0; i < 8; ++i) {
		RightVectors[i] = FVector::CrossProduct(WallNormals[i], UpVectors[i]).GetSafeNormal();
	}

	float RowSpacing = HexagonHeight / HexagonRows; // Use HexagonHeight for vertical spacing
	float ColSpacing = HexagonWidth / HexagonCols; // Use HexagonWidth for horizontal spacing

	for (int wall = 0; wall < 7; ++wall) { // Include all walls
		FVector WallCenter;
		if (wall < 6) { // Side walls
			WallCenter = WallNormals[wall] * (HexagonWidth / 2);
		}
		else { // Top and bottom walls
			// Adjust WallCenter calculation for top and bottom walls
			WallCenter = WallNormals[wall] * (HexagonHeight / 2);
		}

		FVector StartPosition;
		if (wall < 6) { // Side walls
			StartPosition = WallCenter - (RightVectors[wall] * (HexagonWidth / 2 - ColSpacing / 2)) - (UpVectors[wall] * (HexagonHeight / 2 - RowSpacing / 2));
		}
		else { // Top and bottom walls
			// Adjust StartPosition for top and bottom walls to ensure cameras are centered and evenly distributed
			StartPosition = WallCenter - (RightVectors[wall] * (HexagonHeight / 2 - RowSpacing / 2)) - (UpVectors[wall] * (HexagonWidth / 2 - ColSpacing / 2));
		}

		for (int row = 0; row < HexagonRows; ++row) {
			for (int col = 0; col < HexagonCols; ++col) {
				FVector SpawnLocation;
				if (wall < 6) { // Side walls
					SpawnLocation = StartPosition + (RightVectors[wall] * ColSpacing * col) + (UpVectors[wall] * RowSpacing * row);
				}
				else { // Top and bottom walls
					// Adjust SpawnLocation calculation for top and bottom walls
					SpawnLocation = StartPosition + (RightVectors[wall] * RowSpacing * row) + (UpVectors[wall] * ColSpacing * col);
				}

				SpawnLocation.Z += BaseHexagonHeightOffset;

				        // Define the end point of the line trace
				FVector EndPoint = SpawnLocation + (WallNormals[wall] * -FarClipDistance); // Example: 1000 units in the direction of the wall normal
				FHitResult HitResult;
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this); // Ignore the camera manager actor itself in the trace

			    // Perform the line trace
				bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, SpawnLocation, EndPoint, ECC_Visibility, QueryParams);


				if (bHit && HitResult.GetActor() != nullptr && HitResult.GetActor()->Tags.Contains(FName("PointOfInterest"))) {

					// Visualize the line trace
					FColor LineColor = FColor::Green;
					float LineDuration = 2.0f; // Duration in seconds for how long the line should be visible
					bool bPersistentLines = false; // Set to true if you want the line to be unaffected by the duration and persist in the world
					float LineThickness = 1.0f; // Thickness of the line in world units
					//DrawDebugLine(GetWorld(), SpawnLocation, EndPoint, LineColor, bPersistentLines, LineDuration, 0, LineThickness);

					// If hit a point of interest, spawn the camera
					FRotator SpawnRotation = WallNormals[wall].Rotation();
					SpawnRotation.Yaw += 180.0f; // Rotate the camera to face outward from the hexagon wall

					if (wall == 6) { // Top wall
						SpawnRotation.Pitch += 180.0f;
					}

					SpawnCameraAtLocation(SpawnLocation, SpawnRotation);
				}
				else {
					FColor LineColor = FColor::Red;
					float LineDuration = 2.0f; // Duration in seconds for how long the line should be visible
					bool bPersistentLines = false; // Set to true if you want the line to be unaffected by the duration and persist in the world
					float LineThickness = 1.0f; // Thickness of the line in world units
					//DrawDebugLine(GetWorld(), SpawnLocation, EndPoint, LineColor, bPersistentLines, LineDuration, 0, LineThickness);

				}
			}
		}
	}
}

void ACameraManager::SpawnCameraAtLocation(FVector Location, FRotator Rotation){
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	AddCameraToList(Location, Rotation, SpawnParams);

}

void ACameraManager::SpawnCamerasInSphere() {
	float radius, theta;
	FVector SpawnLocation;
	FRotator SpawnRotation;

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

		SpawnCameraAtLocation(SpawnLocation, SpawnRotation);
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
		DepthCamera->SetActorHiddenInGame(false);

		
		if(bRandomRadius)
			DepthCamera->SetDistanceFromLookTarget(NewDistance);
		else
			DepthCamera->SetDistanceFromLookTarget(SphereRadius);

		DepthCamera->Camera->CurrentAperture = 22.0f;
		DepthCameras.Add(NewCamera);
	
		NumOfCamerasInScene++;
	}

	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Green, FString::Printf(TEXT("Num of Cameras: %d"), NumOfCamerasInScene));

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


		SpawnCameraAtLocation(SpawnLocation, SpawnRotation);
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



        // Spawn left camera
        FVector LeftEyeLocation = BaseLocation + FVector(0, -EyeSeparation / 2, 0);
        FRotator LeftEyeRotation = UKismetMathLibrary::FindLookAtRotation(LeftEyeLocation, FVector::ZeroVector);
		SpawnCameraAtLocation(LeftEyeLocation, LeftEyeRotation);

        // Spawn right camera
        FVector RightEyeLocation = BaseLocation + FVector(0, EyeSeparation / 2, 0);
        FRotator RightEyeRotation = UKismetMathLibrary::FindLookAtRotation(RightEyeLocation, FVector::ZeroVector);
		SpawnCameraAtLocation(RightEyeLocation, RightEyeRotation);

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

		SpawnCameraAtLocation(SpawnLocation, SpawnRotation);
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
		DepthCamera->EnableCamera();
		DepthCamera->RenderImages(bCaptureMask, MetaHumanMaskMaterialInstance);
		DepthCamera->DisableCamera();

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
		FilmbackObject->SetNumberField("SensorWidth (mm)", SensorWidth);
		FilmbackObject->SetNumberField("SensorHeight (mm)", SensorHeight);
		FilmbackObject->SetNumberField("SensorAspectRatio", SensorAspectRatio);


		FCameraLensSettings CameraLensSettings = DepthCamera->Camera->LensSettings;

		// Get the Lens settings.
		float MinFocalLength = CameraLensSettings.MinFocalLength;
		float MaxFocalLength = CameraLensSettings.MaxFocalLength;
		float FieldOfView = DepthCamera->Camera->GetHorizontalFieldOfView();
		// Add other lens settings here...

		// Create a new JSON object for the Lens settings.
		TSharedPtr<FJsonObject> LensObject = MakeShareable(new FJsonObject);
		LensObject->SetNumberField("MinFocalLength (mm)", MinFocalLength);
		LensObject->SetNumberField("MaxFocalLength (mm)", MaxFocalLength);
		LensObject->SetNumberField("Horizontal Field of View (degrees)", FieldOfView);
		// Add other lens settings here...

		TSharedPtr<FJsonObject> OtherObject = MakeShareable(new FJsonObject);
		OtherObject->SetNumberField("Near Clip Plane (cm)", 0.1f);
		OtherObject->SetNumberField("Far Clip Plane (cm)", FarClipDistance);
		
		// Create a new JSON object for this camera.
		TSharedPtr<FJsonObject> CameraObject = MakeShareable(new FJsonObject);
		CameraObject->SetObjectField("Camera World Position", PositionObject);
		CameraObject->SetObjectField("Camera World Rotation", RotationObject);
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

