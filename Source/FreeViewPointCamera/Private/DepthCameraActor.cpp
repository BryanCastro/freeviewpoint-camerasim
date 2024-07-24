// Fill out your copyright notice in the Description page of Project Settings.
#include "DepthCameraActor.h"
#include "CineCameraComponent.h"
#include "Engine/SceneCapture.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Light.h"
#include "Engine/PostProcessVolume.h"
#include "Misc/Guid.h"
#include "Engine/World.h"
#include "EngineUtils.h"



// Sets default values
ADepthCameraActor::ADepthCameraActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	
	SceneRGBDCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneRGBDCapture"));
	SceneRGBDCapture->SetupAttachment(Camera);
	SceneRGBCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneRGBCapture"));
	SceneRGBCapture->SetupAttachment(Camera);
	SceneMaskCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneMaskCapture"));
	SceneMaskCapture->SetupAttachment(Camera);

	// Generate a new GUID
	//FGuid NewGuid = FGuid::NewGuid();
	//FString GuidString = NewGuid.ToString();
	//FName UniqueName = FName(*FString::Printf(TEXT("RenderRGBDTarget_%s"), *GuidString));

	RenderRGBDTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("RenderRGBDTarget"));
	RenderRGBTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("RenderRGBTarget"));
	RenderMaskTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("RenderMaskTarget"));
}

// Called when the game starts or when spawned
void ADepthCameraActor::BeginPlay()
{
	Super::BeginPlay();
	
	RenderRGBDTarget->InitAutoFormat(ResolutionX, ResolutionY);
	RenderRGBDTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
	RenderRGBDTarget->UpdateResourceImmediate(true);


	RenderRGBTarget->InitAutoFormat(ResolutionX, ResolutionY);
	RenderRGBTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
	RenderRGBTarget->UpdateResourceImmediate(true); 

	RenderMaskTarget->InitAutoFormat(ResolutionX, ResolutionY);
	RenderMaskTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_R32f;
	RenderMaskTarget->UpdateResourceImmediate(true);

	SceneRGBDCapture->TextureTarget = RenderRGBDTarget;
	SceneRGBDCapture->TextureTarget->ClearColor = FLinearColor::Black;
	SceneRGBDCapture->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	SceneRGBDCapture->bCaptureEveryFrame = false;
	SceneRGBDCapture->bCaptureOnMovement = false;
	SceneRGBDCapture->bAlwaysPersistRenderingState = false;

	// Enable important flags for capturing lighting
	SceneRGBDCapture->ShowFlags.SetLighting(true);
	SceneRGBDCapture->ShowFlags.SetDynamicShadows(true);
	SceneRGBDCapture->ShowFlags.SetGlobalIllumination(true);

	SceneRGBCapture->TextureTarget = RenderRGBTarget;
	SceneRGBCapture->TextureTarget->ClearColor = FLinearColor::White;
	SceneRGBCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneRGBCapture->bCaptureEveryFrame = false;
	SceneRGBCapture->bCaptureOnMovement = false;
	SceneRGBCapture->bAlwaysPersistRenderingState = false;
	ApplyPostProcessSettingsToSceneCapture(GetWorld(), SceneRGBCapture);


	// Enable important flags for capturing lighting
	SceneRGBCapture->ShowFlags.SetLighting(true);
	SceneRGBCapture->ShowFlags.SetDynamicShadows(true);
	SceneRGBCapture->ShowFlags.SetGlobalIllumination(true);

	SceneMaskCapture->TextureTarget = RenderMaskTarget;
	SceneMaskCapture->TextureTarget->ClearColor = FLinearColor::White;
	SceneMaskCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneMaskCapture->bCaptureEveryFrame = false;
	SceneMaskCapture->bCaptureOnMovement = false;
	SceneMaskCapture->bAlwaysPersistRenderingState = true;

	// Enable important flags for capturing lighting
	SceneMaskCapture->ShowFlags.SetLighting(true);
	SceneMaskCapture->ShowFlags.SetDynamicShadows(true);
	SceneMaskCapture->ShowFlags.SetGlobalIllumination(true);

}

void ADepthCameraActor::SetFarClipDistance(float FarClipDistance) {
	FarClip = FarClipDistance;
}

void ADepthCameraActor::SetCameraName(int index) {
	CameraName = FString::Printf(TEXT("Camera_%d"), index);
}

void ADepthCameraActor::SetFarClipPlane(USceneCaptureComponent2D* SceneCapture) {
	int32 RandomID = FMath::RandRange(1, 100000);

	if (SceneCapture) {
		SceneCapture->FOVAngle = Camera->FieldOfView;
		// Set custom projection matrix
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Green, FString::Printf(TEXT("DepthCameraActor.cpp: FOVAngle %f"), SceneCapture->FOVAngle));

		const float FOV = SceneCapture->FOVAngle * (float)PI / 360.0f;
		const float AspectRatio = (float)SceneCapture->TextureTarget->GetSurfaceWidth() / (float)SceneCapture->TextureTarget->GetSurfaceHeight();

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Green, FString::Printf(TEXT("DepthCameraActor.cpp: FOV %f"), FOV));

		const float Near = 0.1f;
		const float Far = FarClip;

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(RandomID, 20.0f, FColor::Green, FString::Printf(TEXT("FarClipValue: %f"), FarClip));

		const float Q = Far / (Far - Near);
		const float Qn = -Q * Near;

		FMatrix ProjectionMatrix = FPerspectiveMatrix(
			FOV,
			AspectRatio,
			1.0f,
			Far,
			Near
		);

		SceneCapture->bUseCustomProjectionMatrix = true;
		SceneCapture->CustomProjectionMatrix = ProjectionMatrix;
		
	}

}

FString ADepthCameraActor::GetCameraName() {
	return CameraName;
}

void ADepthCameraActor::RenderImages(bool bCaptureMask, UMaterialInstance* MetaHumanMaskMaterialInstance){
	// Update the time accumulator
	//TimeAccumulator += DeltaTime;

	// Check if 1/24th of a second has passed
	//if (TimeAccumulator >= (1.0f / 24.0f) && CurrentFramesCaptured <= FramesToCapture)
	//{

		// Capture the scene to update the render target
		SceneRGBDCapture->CaptureScene();
		SceneRGBCapture->CaptureScene();

		if (bCaptureMask) {

			SceneMaskCapture->AddOrUpdateBlendable(MetaHumanMaskMaterialInstance, 1.0f);
			SceneMaskCapture->CaptureScene();
		}
		

		FString Name = GetCameraName();
		// Call the function to save the render targets
		SaveRenderTargetToDisk(RenderRGBDTarget, Name + FString("_RGBD"), true);
		SaveRenderTargetToDisk(RenderRGBTarget, Name + FString("_RGB"));
		SaveRenderTargetToDisk(RenderMaskTarget, Name + FString("_Mask"), true);
		// Reset the time accumulator
		//TimeAccumulator -= (1.0f / 24.0f);

		//CurrentFramesCaptured++;
  //	}
}


// Called every frame
void ADepthCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

const FVector ADepthCameraActor::GetPosition() {
	return Camera->GetComponentLocation();
}

const FRotator ADepthCameraActor::GetRotation(){
	return Camera->GetComponentRotation();
}

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

void ADepthCameraActor::SaveRenderTargetToDisk(UTextureRenderTarget2D* RenderTarget, FString FileName, bool bIsDepth)
{
	if (!RenderTarget)
		return;

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	int32 Width = RenderTarget->SizeX;
	int32 Height = RenderTarget->SizeY;

	TArray<uint8> RawData;

	if (bIsDepth)
	{
		// Read depth data from the render target
		TArray<FFloat16Color> Float16Data;
		RenderTargetResource->ReadFloat16Pixels(Float16Data);

		// Initialize the raw data array with 16-bit depth values
		RawData.AddUninitialized(Width * Height * sizeof(uint16));

		// Find min and max depth values for normalization
		float MinDepth = FLT_MAX;
		float MaxDepth = FLT_MIN;
		for (const FFloat16Color& DepthColor : Float16Data)
		{
			float Depth = DepthColor.R;
			if (Depth < MinDepth) MinDepth = Depth;
			if (Depth > MaxDepth) MaxDepth = Depth;
		}

		// Debug: Log the min and max depth values
		UE_LOG(LogTemp, Warning, TEXT("MinDepth: %f, MaxDepth: %f"), MinDepth, MaxDepth);

		// Normalize and store depth values in 16-bit format
		for (int32 i = 0; i < Float16Data.Num(); ++i)
		{
			float NewDepth = Float16Data[i].R; // Assuming depth is stored in the red channel
			float NormalizedDepth = (NewDepth - MinDepth) / (MaxDepth - MinDepth);
			uint16 DepthValue16 = static_cast<uint16>(NormalizedDepth * 65535.0f); // Convert to 16-bit depth
			RawData[i * 2] = DepthValue16 & 0xFF; // Lower byte
			RawData[i * 2 + 1] = (DepthValue16 >> 8) & 0xFF; // Upper byte
		}
	}
	else
	{
		TArray<FColor> Bitmap;
		Bitmap.AddUninitialized(Width * Height);

		// Read the render target surface data into an array
		FReadSurfaceDataFlags ReadSurfaceDataFlags(RCM_UNorm);
		ReadSurfaceDataFlags.SetLinearToGamma(true); // Ensure gamma correction is applied
		RenderTargetResource->ReadPixels(Bitmap, ReadSurfaceDataFlags);

		// Store the color data as raw bytes
		RawData.SetNum(Width * Height * 4); // 4 bytes per pixel (RGBA)
		FMemory::Memcpy(RawData.GetData(), Bitmap.GetData(), RawData.Num());
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	// Set raw data and save as 16-bit grayscale PNG if depth, otherwise save as normal image
	if (bIsDepth)
	{
		ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), Width, Height, ERGBFormat::Gray, 16);
	}
	else
	{
		ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), Width, Height, ERGBFormat::BGRA, 8);
	}

	// Get compressed data as TArray64<uint8>
	const TArray64<uint8>& PNGData64 = ImageWrapper->GetCompressed(100);
	TArray<uint8> PNGData(PNGData64);

	// Ensure the Images directory exists
	FString DirectoryPath = FPaths::ProjectDir() + TEXT("Images");
	IFileManager::Get().MakeDirectory(*DirectoryPath, true);

	// Define the file path
	FString FilePath = DirectoryPath / (FileName + TEXT(".png"));
	// Save the PNG file
	bool bSuccess = FFileHelper::SaveArrayToFile(PNGData, *FilePath);
}
	
void ADepthCameraActor::SetDistanceFromLookTarget(float Distance){
	DistanceFromLookTarget = Distance;
}

float ADepthCameraActor::GetDistanceFromLookTarget(){
	return DistanceFromLookTarget;
}

void ADepthCameraActor::SetupSceneCaptureComponent(UWorld* World, USceneCaptureComponent2D* SceneCaptureComponent)
{
	SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR; // or SCS_FinalColorLDR
	ApplyPostProcessSettingsToSceneCapture(World, SceneCaptureComponent);
}

void ADepthCameraActor::ApplyPostProcessSettingsToSceneCapture(UWorld* World, USceneCaptureComponent2D* SceneCaptureComponent)
{
	FPostProcessSettings CombinedPostProcessSettings;

	// Retrieve settings from the global post-process volume
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		APostProcessVolume* PPVolume = *It;
		if (PPVolume && PPVolume->bUnbound)
		{
			CombinedPostProcessSettings = PPVolume->Settings;
			break;
		}
	}

	// Apply the post-process settings to the scene capture component
	SceneCaptureComponent->PostProcessSettings = CombinedPostProcessSettings;
}

void ADepthCameraActor::DisableCamera() {
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	TArray<USceneComponent*> Components;
	GetComponents(Components);

	for (USceneComponent* Component : Components) {
		USceneCaptureComponent2D* SceneCamptureComponent = Cast<USceneCaptureComponent2D>(Component);
		if (SceneCamptureComponent) {
			SceneCamptureComponent->SetComponentTickEnabled(false);
			SceneCamptureComponent->Deactivate();
			//SceneCamptureComponent->TextureTarget=nullptr;
		}

		// Disable collision for other components
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
		if (PrimitiveComponent)
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

}
void ADepthCameraActor::EnableCamera() {

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// Enable scene capture components
	TArray<USceneComponent*> Components;
	GetComponents(Components);

	for (USceneComponent* Component : Components)
	{
		USceneCaptureComponent2D* SceneCaptureComponent = Cast<USceneCaptureComponent2D>(Component);
		if (SceneCaptureComponent)
		{
			SceneCaptureComponent->SetComponentTickEnabled(true);
			SceneCaptureComponent->Activate();
			//SceneCaptureComponent->TextureTarget = RenderTarget;
		}

		// Enable collision for other components if needed
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
		if (PrimitiveComponent)
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}