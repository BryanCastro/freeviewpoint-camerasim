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

	RenderRGBDTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("RenderRGBDTarget "));
	RenderRGBTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("RenderRGBTarget "));

    // Initialize DepthMaterialInstance
static ConstructorHelpers::FObjectFinder<UMaterialInstance> MaterialInstance(TEXT("MaterialInstanceConstant'/Game/Materials/MI_PostProcessDepth.MI_PostProcessDepth'"));
    if (MaterialInstance.Succeeded()) {
        DepthMaterialInstance = MaterialInstance.Object;
    } else {
        if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::Printf(TEXT("DepthCameraActor.cpp: Failed to Load MI_PostProcessDepth in Constructor!")));
    }

}

// Called when the game starts or when spawned
void ADepthCameraActor::BeginPlay()
{
	Super::BeginPlay();
	
	RenderRGBDTarget->InitAutoFormat(ResolutionX, ResolutionY);
	RenderRGBDTarget->ClearColor = FLinearColor::Black;
	RenderRGBDTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
	RenderRGBDTarget->UpdateResourceImmediate(true);


	RenderRGBTarget->InitAutoFormat(ResolutionX, ResolutionY);
	RenderRGBTarget->ClearColor = FLinearColor::Black;
	RenderRGBTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
	RenderRGBTarget->UpdateResourceImmediate(true); 


	SceneRGBDCapture->TextureTarget = RenderRGBDTarget;
	SceneRGBDCapture->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	SceneRGBDCapture->PostProcessSettings.AddBlendable(DepthMaterialInstance, 1);

	SceneRGBCapture->TextureTarget = RenderRGBTarget;
	SceneRGBCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	if (DepthMaterialInstance == nullptr) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::Printf(TEXT("DepthCameraActor.cpp: DepthMaterialInstance is Empty!")));
	}
	else {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Green, FString::Printf(TEXT("DepthCameraActor.cpp: DepthMaterialInstance Valid!")));
	}

}

void ADepthCameraActor::RenderImages(){
	// Update the time accumulator
	//TimeAccumulator += DeltaTime;

	// Check if 1/24th of a second has passed
	//if (TimeAccumulator >= (1.0f / 24.0f) && CurrentFramesCaptured <= FramesToCapture)
	//{

		// Capture the scene to update the render target
		SceneRGBDCapture->CaptureScene();
		SceneRGBCapture->CaptureScene();
		FString CameraName = GetName();
		// Call the function to save the render targets
		SaveRenderTargetToDisk(RenderRGBDTarget, CameraName + FString("RGBD"), true);
		SaveRenderTargetToDisk(RenderRGBTarget, CameraName + FString("RGB"));

		// Reset the time accumulator
		TimeAccumulator -= (1.0f / 24.0f);

		CurrentFramesCaptured++;
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



	// Save the pixel data to a bitmap file
void ADepthCameraActor::SaveRenderTargetToDisk(UTextureRenderTarget2D* RenderTarget, FString FileName, bool bIsDepth) {

	if (!RenderTarget)
		return;



	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	//FReadSurfaceDataFlags ReadSurfaceDataFlags;
	//ReadSurfaceDataFlags.SetLinearToGamma(false);


	int32 Width = RenderTarget->SizeX;
    int32 Height = RenderTarget->SizeY;

    TArray<FColor> Bitmap;
    Bitmap.AddUninitialized(Width * Height);

    // Read the render target surface data into an array
    RenderTargetResource->ReadPixels(Bitmap);

	if(bIsDepth){
		// Convert to grayscale
		for (auto& Color : Bitmap)
		{
			uint8 GrayValue = Color.R;
			Color = FColor(GrayValue, GrayValue, GrayValue, 255); // Set RGB to the same gray value and Alpha to 255
		}
	}

	/*
	int32 Width = RenderTarget->SizeX;
	int32 Height = RenderTarget->SizeY;

	TArray<FFloat16Color> Float16ColorData;
	Float16ColorData.AddUninitialized(Width * Height);

	// Read the render target surface data into an array
	RenderTargetResource->ReadFloat16Pixels(Float16ColorData);

	TArray<FColor> Bitmap;
	Bitmap.AddUninitialized(Width * Height);

	// Find min and max depth values
	float MinDepth = FLT_MAX;
	float MaxDepth = -FLT_MAX;
	for (const auto& Color : Float16ColorData)
	{
		float Depth = Color.R.GetFloat();
		if (Depth < MinDepth) MinDepth = Depth;
		if (Depth > MaxDepth) MaxDepth = Depth;
	}

	// Normalize depth values and populate bitmap
	for (int32 i = 0; i < Float16ColorData.Num(); ++i)
	{
		float Depth = Float16ColorData[i].R.GetFloat();
		uint8 DepthByte = static_cast<uint8>(255 * (Depth - MinDepth) / (MaxDepth - MinDepth));
		Bitmap[i] = FColor(DepthByte, DepthByte, DepthByte, 255); // Grayscale depth
	}
	*/

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.GetAllocatedSize(), RenderTarget->SizeX, RenderTarget->SizeY, ERGBFormat::BGRA, 8);

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

	// Save the raw file
	/*
	TArray<uint8> RawData;
	RawData.Reserve(Bitmap.Num() * sizeof(FColor));
	for (const FColor& Color : Bitmap)
	{
		RawData.Append(reinterpret_cast<const uint8*>(&Color), sizeof(FColor));
	}
	// Define the raw file path
	FString RawFilePath = DirectoryPath / (FileName + TEXT(".raw"));

	// Save the raw file
	bool bRawSuccess = FFileHelper::SaveArrayToFile(RawData, *RawFilePath);
	*/
}
	

