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
	SceneRGBDCapture->bCaptureEveryFrame = true;
	SceneRGBDCapture->bCaptureOnMovement = true;

	// Enable important flags for capturing lighting
	SceneRGBDCapture->ShowFlags.SetLighting(true);
	SceneRGBDCapture->ShowFlags.SetDynamicShadows(true);
	SceneRGBDCapture->ShowFlags.SetGlobalIllumination(true);
	//SceneRGBDCapture->PostProcessSettings.AddBlendable(DepthMaterialInstance, 1);

	SceneRGBCapture->TextureTarget = RenderRGBTarget;
	SceneRGBCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneRGBCapture->bCaptureEveryFrame = true;
	SceneRGBCapture->bCaptureOnMovement = true;

	// Enable important flags for capturing lighting
	SceneRGBCapture->ShowFlags.SetLighting(true);
	SceneRGBCapture->ShowFlags.SetDynamicShadows(true);
	SceneRGBCapture->ShowFlags.SetGlobalIllumination(true);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(0, 20.0f, FColor::Green, FString::Printf(TEXT("In Constructor!")));


	if (DepthMaterialInstance == nullptr) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::Printf(TEXT("DepthCameraActor.cpp: DepthMaterialInstance is Empty!")));
	}
	else {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Green, FString::Printf(TEXT("DepthCameraActor.cpp: DepthMaterialInstance Valid!")));
	}

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
		const float FOV = SceneCapture->FOVAngle * (float)PI / 360.0f;
		const float AspectRatio = (float)SceneCapture->TextureTarget->GetSurfaceWidth() / (float)SceneCapture->TextureTarget->GetSurfaceHeight();

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

void ADepthCameraActor::RenderImages(){
	// Update the time accumulator
	//TimeAccumulator += DeltaTime;

	// Check if 1/24th of a second has passed
	//if (TimeAccumulator >= (1.0f / 24.0f) && CurrentFramesCaptured <= FramesToCapture)
	//{

		// Capture the scene to update the render target
		SceneRGBDCapture->CaptureScene();
		SceneRGBCapture->CaptureScene();
		FString Name = GetCameraName();
		// Call the function to save the render targets
		SaveRenderTargetToDisk(RenderRGBDTarget, Name + FString("_RGBD"), true);
		SaveRenderTargetToDisk(RenderRGBTarget, Name + FString("_RGB"));

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
	FReadSurfaceDataFlags ReadSurfaceDataFlags(RCM_UNorm);
	ReadSurfaceDataFlags.SetLinearToGamma(false);


	int32 Width = RenderTarget->SizeX;
    int32 Height = RenderTarget->SizeY;

    TArray<FColor> Bitmap;
    Bitmap.AddUninitialized(Width * Height);

   // Initialize a depth buffer with a large initial depth for each pixel
    TArray<float> DepthBuffer;
    DepthBuffer.AddUninitialized(Width * Height);
    for (auto& Depth : DepthBuffer)
    {
        Depth = FLT_MAX;
    }

    // Read the render target surface data into an array
    RenderTargetResource->ReadPixels(Bitmap, ReadSurfaceDataFlags);

    if(bIsDepth){
        // Convert to grayscale and implement depth testing
        for (int32 i = 0; i < Bitmap.Num(); ++i)
        {
            auto& Color = Bitmap[i];
            float NewDepth = Color.R / 255.0f; // Assuming depth is stored in the red channel and normalized to [0, 1]
            // Only overwrite the pixel if the new pixel is closer to the camera
            if (NewDepth < DepthBuffer[i])
            {
                uint8 GrayValue = Color.R;
                Color = FColor(GrayValue, GrayValue, GrayValue, 255); // Set RGB to the same gray value and Alpha to 255
                DepthBuffer[i] = NewDepth;
            }
        }
    }


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
	
}
	

