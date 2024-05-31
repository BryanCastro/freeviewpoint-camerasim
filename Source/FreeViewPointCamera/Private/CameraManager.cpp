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

// Sets default values
ACameraManager::ACameraManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	if (!CameraActorClassRef) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::Printf(TEXT("CameraManager.cpp: Failed to Find Camera Blueprint Class Reference!")));
	}

}

void ACameraManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property!=nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, NumOfCameras)) {
		
		SpawnCameras();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACameraManager, SphereRadius)) {
		SpawnCameras();
	}
}
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
		DepthCamera->SetFarClipDistance(SphereRadius * 2);
		DepthCamera->SetFarClipPlane(DepthCamera->SceneRGBCapture);
		DepthCamera->SetFarClipPlane(DepthCamera->SceneRGBDCapture);
	}
}

void ACameraManager::SpawnCameras() {
	if (!CameraActorClassRef) return;

	ClearSpawnedCameras();

	switch (CurrentState) {
		case CameraSetupEnum::SPHERE:
			SpawnCamerasInSphere();
			break;
		default:
			SpawnCamerasInSphere();
			break;
	}

}

void ACameraManager::SpawnCamerasInSphere() {
	const float Phi = (1 + sqrt(5)) / 2; // Golden ratio
	for (int32 i = 0; i < NumOfCameras; i++)
	{
		float theta = 2 * PI * i / Phi;
		float z = 1 - (i / (NumOfCameras - 1.0f)) * 2; // z goes from 1 to -1
		float radius = sqrt(1 - z * z); // radius at z

		float x = cos(theta) * radius;
		float y = sin(theta) * radius;

		FVector SpawnLocation = FVector(x, y, z) * SphereRadius;
		FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, FVector::ZeroVector);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		AActor* NewCamera = GetWorld()->SpawnActor<AActor>(CameraActorClassRef, SpawnLocation, SpawnRotation, SpawnParams);
		if(NewCamera){
			DepthCameras.Add(NewCamera);
		}
	}
}

void ACameraManager::ClearSpawnedCameras() {
	for (AActor* Camera : DepthCameras) {
		if (Camera) {
			Camera->Destroy();
		}
	}
	DepthCameras.Empty();
}


// Called every frame
void ACameraManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACameraManager::RenderImages() {
    // Create a new JSON object.
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);


	for (auto camera : DepthCameras) {
		Cast<ADepthCameraActor>(camera)->RenderImages();

		  // Get the camera's world position and rotation.
        FVector Position = camera->GetActorLocation();
        FRotator Rotation = camera->GetActorRotation();

		
		float Tolerance = 0.00001f;
		Rotation.Pitch = FMath::IsNearlyZero(Rotation.Pitch, Tolerance) ? 0.0f : Rotation.Pitch;
		Rotation.Yaw = FMath::IsNearlyZero(Rotation.Yaw, Tolerance) ? 0.0f : Rotation.Yaw;
		Rotation.Roll = FMath::IsNearlyZero(Rotation.Roll, Tolerance) ? 0.0f : Rotation.Roll;
		

		// Create a new JSON object for this camera.
        TSharedPtr<FJsonObject> CameraObject = MakeShareable(new FJsonObject);
        CameraObject->SetStringField("World Position", Position.ToString());
        CameraObject->SetStringField("World Rotation", Rotation.ToString());

		// Add the camera object to the main JSON object.
        JsonObject->SetObjectField(camera->GetName(), CameraObject);
	}
	// Convert the JSON object to a string.
	FString JsonString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

		// Save the JSON string to a file.
	FFileHelper::SaveStringToFile(JsonString, *(FPaths::ProjectDir() + FString("/Images/CameraData.json")));
}

