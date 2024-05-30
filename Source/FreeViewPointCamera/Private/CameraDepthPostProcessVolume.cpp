// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraDepthPostProcessVolume.h"
#include "Materials/MaterialExpressionSceneDepth.h"
#include "Materials/Material.h"

ACameraDepthPostProcessVolume::ACameraDepthPostProcessVolume() {

}

void ACameraDepthPostProcessVolume::BeginPlay() {
	/*
	DepthMapMaterial = NewObject<UMaterial>();
	DepthMapMaterial->MaterialDomain = MD_PostProcess;
	DepthMapMaterial->BlendableLocation = BL_BeforeTonemapping;

	SceneDepthNode = NewObject<UMaterialExpressionSceneDepth>(DepthMapMaterial);
	SceneDepthNode->Material = DepthMapMaterial;

	FExpressionInput Input;
	Input.Expression = SceneDepthNode;
	DepthMapMaterial->EmissiveColor = Input;

	DepthMapMaterial->

	bUnbound = true;
	FPostProcessSettings& PostProcessSettings = Settings;
	PostProcessSettings.AddBlendable(DepthMapMaterial, 1.0f);
	*/
}