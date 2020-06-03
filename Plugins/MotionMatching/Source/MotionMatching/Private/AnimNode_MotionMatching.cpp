#include "AnimNode_MotionMatching.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"


void FAnimNode_MotionMatching::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	if (!InAnimInstance)
	{
		ensureAlwaysMsgf(false, TEXT("Animation instance is nullptr"));

		return;
	}
	
	SkeletalMeshComponent = InAnimInstance->GetSkelMeshComponent();
	OwnerPawn = InAnimInstance->TryGetPawnOwner();
	World = InAnimInstance->GetWorld();	
	BoneNames.Remove(NAME_None);
	AnimationContainer.Init(AnimationsArray, AnimationSampling);
	LoadBoneToRootTransforms();
}

void FAnimNode_MotionMatching::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	const float deltaTime = Context.GetDeltaTime(); 

	GlobalDeltaTime = deltaTime;
	DebugTimer += deltaTime;

	if (DebugTimer > DebugRate)
	{
		UpdateTimer += deltaTime;
	}
}

void FAnimNode_MotionMatching::Evaluate_AnyThread(FPoseContext& Output)
{
	if (!SkeletalMeshComponent)
	{
		ensureMsgf(false, TEXT("SkeletalMeshComponent is nullptr"));

		return;
	}

	if (DebugTimer > DebugRate)
	{
		DebugTimer = 0.0f;
		if (UpdateTimer > UpdateRate)
		{
			PreviousAnimKey = NewAnimKey;
			LowestCostAnimkey = FindLowestCostAnimKey();
			UpdateTimer = 0.0f;
			BlendWeight = 1.0f;

			if(IsDebugMode)
			{
				const FVector& currentTrajectory = CalculateCurrentTrajectory();
				DrawDebugTrajectory(currentTrajectory, FColor::Yellow);
	
				const FTransform& animTransform = AnimationContainer.ExtractRootMotion(LowestCostAnimkey, UpdateRate);
				const FTransform& worldAnimTransform = SkeletalMeshComponent->ConvertLocalRootMotionToWorld(animTransform);
				DrawDebugTrajectory(worldAnimTransform.GetTranslation(), FColor::Red);
			}
		}

		NewAnimKey = FAnimKey{ LowestCostAnimkey.Index, LowestCostAnimkey.StartTime + UpdateTimer };
		MoveOwnerPawn();
	}

	BlendWeight -= BlendWeightDecrement;
	AnimationContainer.GetBlendedPose(Output, PreviousAnimKey, NewAnimKey, BlendWeight);
}

FAnimKey FAnimNode_MotionMatching::FindLowestCostAnimKey()
{
	float lowestAnimCost = BIG_NUMBER;
	FAnimKey lowestCostAnimKey;
	CurrentTrajectory = CalculateCurrentTrajectory();

	for (AnimationContainer.ResetKey(); AnimationContainer.CurrentKey < AnimationContainer.MaxKey(); AnimationContainer.IncrementKey())
	{
		float currentAnimCost = 0.0f;
		const FTransform& rootMotion = AnimationContainer.ExtractRootMotion(AnimationContainer.CurrentKey, UpdateRate);

		if (TrajectoryWeight > 0)
		{
			currentAnimCost += TrajectoryWeight * ComputeTrajectoryCost(rootMotion);
		}

		if (PoseWeight > 0)
		{
			currentAnimCost += PoseWeight * ComputePoseCost();
		}

		if (lowestAnimCost > currentAnimCost)
		{
			lowestAnimCost = currentAnimCost;
			lowestCostAnimKey = AnimationContainer.CurrentKey;
		}
	}
	
	return lowestCostAnimKey;
}

float FAnimNode_MotionMatching::ComputeTrajectoryCost(const FTransform& RootMotion) const 
{
	const FTransform& worldAnimTransform = SkeletalMeshComponent->ConvertLocalRootMotionToWorld(RootMotion);
	FVector animTranslation = worldAnimTransform.GetTranslation();
	
	return FVector::Dist(CurrentTrajectory, animTranslation);
}

float FAnimNode_MotionMatching::ComputePoseCost() const
{
	float Cost = 0.0f;
		
	for (const FName& boneName : BoneNames)
	{
		const FTransform& newBoneTransform = GetLoadedBoneToRootTransform(boneName, AnimationContainer.CurrentKey);
		const FTransform& previousBoneTransform = GetLoadedBoneToRootTransform(boneName, PreviousAnimKey);
	
		Cost += FTransform::SubtractTranslations(newBoneTransform, previousBoneTransform).Size();
	}

	return Cost;
}

FVector FAnimNode_MotionMatching::CalculateCurrentTrajectory() const
{
	if (!OwnerPawn)
	{
#if !WITH_EDITOR
		ensureMsgf(false, TEXT("OwnerPawn is nullptr"));
#endif //WITH_EDITOR
		
		return FVector{1, 1, 1};
	}

	const FRotator& rotation = OwnerPawn->GetControlRotation();
	const FRotator& yawRotation = FRotator{0, rotation.Yaw, 0};
	const FVector& forwardDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);
	const FVector& rightDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);
	const float forwardValue = OwnerPawn->GetInputAxisValue(FName{ "MoveForward" }); //TODO: find a better way of providing input axes names
	const float rightValue = OwnerPawn->GetInputAxisValue(FName{ "MoveRight" });

	return (forwardDirection * forwardValue + rightDirection * rightValue) * TrajectoryLength;
}

void FAnimNode_MotionMatching::MoveOwnerPawn() const
{
	if (!OwnerPawn)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("OwnerPawn is nullptr"));
#endif //WITH_EDITOR

		return;
	}
	
	const FTransform& blendedRootMotion = AnimationContainer.ExtractBlendedRootMotion(PreviousAnimKey, NewAnimKey, BlendWeight, GlobalDeltaTime);

	UCharacterMovementComponent* ownerPawnMovementComponent = Cast<UCharacterMovementComponent>(OwnerPawn->GetMovementComponent());
		
	if (ownerPawnMovementComponent)
	{
		// character movement component should convert the root motion to the world coordinates:
		ownerPawnMovementComponent->RootMotionParams.Set(blendedRootMotion);
	}
}

void FAnimNode_MotionMatching::LoadBoneToRootTransforms()
{
	for (UAnimSequence* animationSequence : AnimationsArray)
	{
		BoneToRootTransformsArray.Emplace(FBoneToRootTransforms{SkeletalMeshComponent, animationSequence, AnimationSampling});
	}
}

FTransform FAnimNode_MotionMatching::GetLoadedBoneToRootTransform(const FName& BoneName, const FAnimKey& AnimKey) const
{
	const FBoneToRootTransforms& boneToRootTransforms = BoneToRootTransformsArray[AnimKey.Index];
	const int32 boneIndex = SkeletalMeshComponent->GetBoneIndex(BoneName);
	const int32 keyIndex = static_cast<int32>(AnimKey.StartTime / AnimationSampling);

	return boneToRootTransforms.GetTransform(boneIndex, keyIndex);
}

void FAnimNode_MotionMatching::DrawDebugTrajectory(const FVector& Trajectory, const FColor& Color) const
{
	if (!World || !SkeletalMeshComponent)
	{
#if !WITH_EDITOR
		ensureMsgf(World, TEXT("World instance is nullptr"));
		ensureMsgf(SkeletalMesh, TEXT("SkeletalMesh is nullptr"));
#endif //WITH_EDITOR

		return;
	}

	const FVector& skeletalMeshLocation = SkeletalMeshComponent->GetComponentLocation();
	UKismetSystemLibrary::DrawDebugLine(World, skeletalMeshLocation, skeletalMeshLocation + (Trajectory * 10), Color, DebugLinesLifetime, 2.0f);
}
