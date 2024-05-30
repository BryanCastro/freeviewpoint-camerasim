// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/PostProcessVolume.h"
#include "CameraDepthPostProcessVolume.generated.h"


class UMaterial;
class UMaterialExpressionSceneDepth;

/**
 * 
 */
UCLASS()
class FREEVIEWPOINTCAMERA_API ACameraDepthPostProcessVolume : public APostProcessVolume
{
	GENERATED_BODY()

public:
	ACameraDepthPostProcessVolume();
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta = (AllowPrivateAccess = "true"))
	UMaterial* DepthMapMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta=(AllowPrivateAccess="true"))
	UMaterialExpressionSceneDepth* SceneDepthNode;


};
