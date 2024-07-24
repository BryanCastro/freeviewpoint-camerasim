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
	
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(Camera);

	RenderTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("RenderTarget"));

}

void ADepthCameraActor::RenderImage(USceneCaptureComponent2D* SceneCapture2D, ETextureRenderTargetFormat TargetFormat, ESceneCaptureSource SceneCaptureSource, bool AlwaysPersisting) {
	RenderTarget->RenderTargetFormat = TargetFormat;
	RenderTarget->UpdateResourceImmediate(true);

	SceneCapture2D->CaptureSource = SceneCaptureSource;
	SceneCapture2D->bAlwaysPersistRenderingState = AlwaysPersisting;
	SceneCapture2D->CaptureScene();

}

// Called when the game starts or when spawned
void ADepthCameraActor::BeginPlay()
{
	Super::BeginPlay();
	
	RenderTarget->InitAutoFormat(ResolutionX, ResolutionY);
	RenderTarget->UpdateResourceImmediate(true);

	SceneCapture->TextureTarget = RenderTarget;
	SceneCapture->TextureTarget->ClearColor = FLinearColor::Black;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->bAlwaysPersistRenderingState = false;

	// Enable important flags for capturing lighting
	SceneCapture->ShowFlags.SetLighting(true);
	SceneCapture->ShowFlags.SetDynamicShadows(true);
	SceneCapture->ShowFlags.SetGlobalIllumination(true);

	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	//SceneCapture->bAlwaysPersistRenderingState = true;

	ApplyPostProcessSettingsToSceneCapture(GetWorld(), SceneCapture);

}

void ADepthCameraActor::SetFarClipDistance(float FarClipDistance) {
	FarClip = FarClipDistance;
}

void ADepthCameraActor::SetCameraName(int index) {
	CameraName = FString::Printf(TEXT("Camera_%d"), index);
}

void ADepthCameraActor::SetFarClipPlane(USceneCaptureComponent2D* SceneCapture2D) {
	int32 RandomID = FMath::RandRange(1, 100000);

	if (SceneCapture2D) {
		SceneCapture2D->FOVAngle = Camera->FieldOfView;
		// Set custom projection matrix
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Green, FString::Printf(TEXT("DepthCameraActor.cpp: FOVAngle %f"), SceneCapture->FOVAngle));

		const float FOV = SceneCapture2D->FOVAngle * (float)PI / 360.0f;
		const float AspectRatio = (float)SceneCapture2D->TextureTarget->GetSurfaceWidth() / (float)SceneCapture2D->TextureTarget->GetSurfaceHeight();

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

		SceneCapture2D->bUseCustomProjectionMatrix = true;
		SceneCapture2D->CustomProjectionMatrix = ProjectionMatrix;
		
	}

}

FString ADepthCameraActor::GetCameraName() {
	return CameraName;
}

void ADepthCameraActor::RenderImages(bool bCaptureMask, UMaterialInstance* MetaHumanMaskMaterialInstance){

		FString Name = GetCameraName();

		RenderImage(SceneCapture, ETextureRenderTargetFormat::RTF_RGBA16f, ESceneCaptureSource::SCS_SceneDepth);
		SaveRenderTargetToDisk(RenderTarget,Name+FString("_RGBD"), true);
		RenderImage(SceneCapture, ETextureRenderTargetFormat::RTF_RGBA16f, ESceneCaptureSource::SCS_FinalColorLDR);
		SaveRenderTargetToDisk(RenderTarget,Name + FString("_RGB"), false);

		if (bCaptureMask) {
			SceneCapture->AddOrUpdateBlendable(MetaHumanMaskMaterialInstance, 1.0f);
			RenderImage(SceneCapture, ETextureRenderTargetFormat::RTF_R32f, ESceneCaptureSource::SCS_FinalColorLDR, true);
			SaveRenderTargetToDisk(RenderTarget, Name + FString("_Mask"), true);
		}
		
		if (RenderTarget)
		{
			SceneCapture->ConditionalBeginDestroy();
			SceneCapture = nullptr;
			RenderTarget->ConditionalBeginDestroy();
			RenderTarget = nullptr;
		}

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

void ADepthCameraActor::SaveRenderTargetToDisk(UTextureRenderTarget2D* RenderTarget2D, FString FileName, bool bIsDepth)
{
	if (!RenderTarget2D)
		return;

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget2D->GameThread_GetRenderTargetResource();
	int32 Width = RenderTarget2D->SizeX;
	int32 Height = RenderTarget2D->SizeY;

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