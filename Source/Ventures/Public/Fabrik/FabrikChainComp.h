// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "FabrikChainComp.generated.h"

USTRUCT()
struct FChainBones
{
	GENERATED_BODY()

	TArray<int32> BoneIndices;
	
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VENTURES_API UFabrikChainComp : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFabrikChainComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:

	int NumBones;
	TArray<FTransform> BoneTransforms;
	FVector RootStartLocation;
	TArray<FVector> BoneLocations;
	TArray<float> BoneLengths;
	float TotalChainLength;
	float DistanceToTarget;
	//TArray<FQuat> BoneStartRotations;
	//TArray<FVector> BoneForwardVectors;
	//FQuat TargetStartRotation;
	//FQuat EndEffectorStartRotation;

	FTransform TargetTransform;
	TArray<FChainBones> BoneChains;
	TArray<float> ExtensionRatios;
	
	

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fabrik")
	UPoseableMeshComponent* Chain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fabrik")
	int NumIterations = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fabrik")
	TArray<FTransform> TargetTransforms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fabrik")
	float Tolerance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fabrik")
	TArray<FTransform> PoleTargets;

private:

	UFUNCTION()
	void InitBoneChains();
	
	UFUNCTION()
	void InitChain(FChainBones BoneChain, int ChainIndex);
	
	UFUNCTION()
	void FabrikSolve(FChainBones BoneChain, int ChainIndex);

	UFUNCTION()
	void FabrikForward();

	UFUNCTION()
	void FabrikBackward();

	// only works for 3 bone chains
	UFUNCTION()
	void PoleConstraint(int ChainIndex);

	UFUNCTION(BlueprintCallable, Category = "Fabrik")
	float GetExtensionRatio(int ChainIndex);

	UFUNCTION(BlueprintCallable, Category = "Fabrik")
	void SetTargetTransformByChainIndex(int ChainIndex, FTransform Transform);

	UFUNCTION(BlueprintCallable, Category = "Fabrik")
	FTransform GetTargetTransformByChainIndex(int ChainIndex);

	UFUNCTION(BlueprintCallable, Category="Math|Plane Fitting")
	static FVector ComputeBestFitPlaneNormal(const TArray<FVector>& Points);
};
