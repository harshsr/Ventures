// Fill out your copyright notice in the Description page of Project Settings.


#include "Fabrik/FabrikChainComp.h"


UFabrikChainComp::UFabrikChainComp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.
	PrimaryComponentTick.bCanEverTick = true;
}


void UFabrikChainComp::BeginPlay()
{
	Super::BeginPlay();

	InitBoneChains();
}


void UFabrikChainComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!BoneChains.IsEmpty())
	{
		for (int i = 0; i < BoneChains.Num(); i++)
		{
			FabrikSolve(BoneChains[i],i);
		}
	}
}

void UFabrikChainComp::InitBoneChains()
{
	FString ChainStartIdentifier = "Start";
	FString ChainEndIdentifier = "IK";

	int NumTotalBones = Chain->GetNumBones();
	int Iteraor = 0;

	while (Iteraor < NumTotalBones)
	{
		FName BoneName = Chain->GetBoneName(Iteraor);
		FString BoneNameString = BoneName.ToString();
		if (BoneNameString.Contains(ChainStartIdentifier))
		{
			FChainBones BoneChain;
			BoneChain.BoneIndices.Add(Iteraor);
			Iteraor++;
			while (Iteraor < NumTotalBones)
			{
				FName NextBoneName = Chain->GetBoneName(Iteraor);
				FString NextBoneNameString = NextBoneName.ToString();
				if (NextBoneNameString.Contains(ChainEndIdentifier))
				{
					BoneChain.BoneIndices.Add(Iteraor);
					break;
				}
				else
				{
					BoneChain.BoneIndices.Add(Iteraor);
					Iteraor++;
				}
			}
			BoneChains.Add(BoneChain);
		}
		else
		{
			Iteraor++;
		}
	}

	ExtensionRatios.SetNum(BoneChains.Num());
}


void UFabrikChainComp::InitChain(FChainBones BoneChain, int ChainIndex)
{
	BoneTransforms.Empty();
	BoneLocations.Empty();
	BoneLengths.Empty();
	
	if (TargetTransforms.Num() > ChainIndex)
	{
		TargetTransform = TargetTransforms[ChainIndex];
	}
	else
	{
		return;
	}
	
	NumBones = BoneChain.BoneIndices.Num();
	
	BoneTransforms.SetNum(NumBones);
	BoneLocations.SetNum(NumBones);
	BoneLengths.SetNum(NumBones);
	

	TotalChainLength = 0.0f;

	for (int i = 0; i < NumBones; i++)
	{
		BoneTransforms[i] = Chain->GetBoneTransform(BoneChain.BoneIndices[i]);
		BoneLocations[i] = BoneTransforms[i].GetLocation();

		if (i > 0)
		{
			float BoneLength = 0.0f;
			BoneLength = (BoneLocations[i] - BoneLocations[i - 1]).Size();
			BoneLengths[i] = BoneLength;
			TotalChainLength += BoneLength;
		}
	}
}

void UFabrikChainComp::FabrikSolve(FChainBones BoneChain, int ChainIndex)
{
	InitChain(BoneChain, ChainIndex);
	
	// joint positions
	if (BoneLocations.IsEmpty())
	{
		return;
	}

	// distance from root to target
	DistanceToTarget = (BoneLocations[0] - TargetTransform.GetLocation()).Size();

	// target is further than the length of the chain
	if (DistanceToTarget > TotalChainLength)
	{
		// move the target to the end of the chain
		FVector Direction = (TargetTransform.GetLocation() - BoneLocations[0]).GetSafeNormal();

		// arrange the bones in a line
		for (int i = 1; i < NumBones; i++)
		{
			BoneLocations[i] = BoneLocations[i - 1] + Direction * BoneLengths[i];
		}
	}
	else
	{
		// distance from end effector to target
		DistanceToTarget = (BoneLocations[NumBones - 1] - TargetTransform.GetLocation()).Size();
		int Iterations = 0;
		while (DistanceToTarget > Tolerance && Iterations < NumIterations)
		{
			RootStartLocation = BoneLocations[0];
			FabrikBackward();
			FabrikForward();

			// distance from end effector to target
			DistanceToTarget = (BoneLocations[NumBones - 1] - TargetTransform.GetLocation()).Size();
			Iterations++;
		}
	}

	PoleConstraint(ChainIndex);
	
	// update the bone transforms
	for (int i = 0; i < NumBones; i++)
	{
		FTransform BoneTransform = BoneTransforms[i];
		BoneTransform.SetLocation(BoneLocations[i]);
		FVector Direction =  FVector(0.0f, 0.0f, 0.0f);
		if (i < NumBones - 2)
		{
			Direction = (BoneLocations[i + 1] - BoneLocations[i]).GetSafeNormal();
			
		}
		else
		{
			Direction = (TargetTransform.GetLocation() - BoneLocations[i]).GetSafeNormal();
		}
		FRotator Rotation = Direction.Rotation();
		FQuat TargetRotation = FQuat::MakeFromRotator( Rotation );
		BoneTransform.SetRotation(TargetRotation);
		
		
		Chain->SetBoneTransformByName(Chain->GetBoneName(BoneChain.BoneIndices[i]), BoneTransform, EBoneSpaces::WorldSpace);
		
	}

	// update the extension ratio
	float TotalExtension = (BoneLocations[0] - BoneLocations[NumBones - 1]).Size();
	ExtensionRatios[ChainIndex] = TotalExtension / TotalChainLength;
}

void UFabrikChainComp::FabrikBackward()
{
	for (int i = NumBones - 1; i > 0; i--)
	{
		if (i == NumBones - 1)
		{
			BoneLocations[i] = TargetTransform.GetLocation();
		}
		else
		{
			FVector Direction = (BoneLocations[i + 1] - BoneLocations[i]).GetSafeNormal();
			BoneLocations[i] = BoneLocations[i + 1] - Direction * BoneLengths[i + 1];
		}
	}
}

void UFabrikChainComp::FabrikForward()
{
	for (int i = 0; i < NumBones; i++)
	{
		if (i == 0)
		{
			BoneLocations[i] = RootStartLocation;
		}
		else
		{
			FVector Direction = (BoneLocations[i - 1] - BoneLocations[i]).GetSafeNormal();
			BoneLocations[i] = BoneLocations[i - 1] - Direction * BoneLengths[i];
		}
	}
}

void UFabrikChainComp::PoleConstraint(int ChainIndex)
{
	if (NumBones == 4 && PoleTargets.Num()>ChainIndex)
	{
		FVector LimbAxis = (BoneLocations[3] - BoneLocations[1]).GetSafeNormal();

		FVector PoleDirection = 1 * (PoleTargets[ChainIndex].GetLocation() - BoneLocations[1]).GetSafeNormal();
		FVector BoneDirection = (BoneLocations[2] - BoneLocations[1]).GetSafeNormal();

		// debug draw
		/*
		DrawDebugDirectionalArrow(GetWorld(), BoneLocations[1], BoneLocations[1] + PoleDirection * 1000.0f, 10.0f, FColor::Blue, false, 0.01f, 0, 1.0f);
		DrawDebugDirectionalArrow(GetWorld(), BoneLocations[1], BoneLocations[1] + BoneDirection * 1000.0f, 10.0f, FColor::Green, false, 0.01f, 0, 1.0f);
		DrawDebugDirectionalArrow(GetWorld(), BoneLocations[1], BoneLocations[1] + LimbAxis * 1000.0f, 10.0f, FColor::Red, false, 0.01f, 0, 1.0f);
		*/

		// make pole direction orthogonal to limb axis
		FVector Orthogonal = FVector::CrossProduct(PoleDirection, LimbAxis);
		PoleDirection = FVector::CrossProduct(LimbAxis, Orthogonal);
		PoleDirection.Normalize();

		// make bone direction orthogonal to limb axis
		Orthogonal = FVector::CrossProduct(BoneDirection, LimbAxis);
		BoneDirection = FVector::CrossProduct(LimbAxis, Orthogonal);
		BoneDirection.Normalize();

		// debug draw orthogonal vectors
		/*
		DrawDebugDirectionalArrow(GetWorld(), BoneLocations[1], BoneLocations[1] + PoleDirection * 2000.0f, 10.0f, FColor::Purple, false, 0.01f, 0, 1.0f);
		DrawDebugDirectionalArrow(GetWorld(), BoneLocations[1], BoneLocations[1] + BoneDirection * 1000.0f, 10.0f, FColor::Yellow, false, 0.01f, 0, 1.0f);
		*/

		// get the angle between the pole direction and the bone direction
		FQuat Rotation = FQuat::FindBetweenNormals(BoneDirection, PoleDirection);

		BoneLocations[2] = BoneLocations[1] + Rotation.RotateVector(BoneLocations[2] - BoneLocations[1]);
	}
}

float UFabrikChainComp::GetExtensionRatio(int ChainIndex)
{
	return ExtensionRatios[ChainIndex];
}

void UFabrikChainComp::SetTargetTransformByChainIndex(int ChainIndex, FTransform Transform)
{
	TargetTransforms[ChainIndex] = Transform;
}

FTransform UFabrikChainComp::GetTargetTransformByChainIndex(int ChainIndex)
{
	return TargetTransforms[ChainIndex];
}

FVector UFabrikChainComp::ComputeBestFitPlaneNormal(const TArray<FVector>& Points)
{
	if (Points.Num() == 4)
	{
		FVector PlaneNormal = FVector::CrossProduct(Points[1] - Points[0], Points[2] - Points[0]);
		PlaneNormal.Normalize();
		return PlaneNormal;
	}
	else
	{
		return FVector(0.0f, 0.0f, 0.0f);
	}
}





