#include "AnimNode_MotionMatching.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimSequence.h"

#define DEBUG_DRAW 0

namespace
{
	void DrawDebugTrajectory(UWorld* World, USkeletalMeshComponent* SkeletalMesh, const FVector& CurrentTrajectory, const FColor& Color = FColor::Green)
	{
		if (!World || !SkeletalMesh)
		{
#if !WITH_EDITOR
			ensureMsgf(World, TEXT("World instance is nullptr"));
			ensureMsgf(SkeletalMesh, TEXT("SkeletalMesh is nullptr"));
#endif //WITH_EDITOR

			return;
		}

		const FVector& skeletalMeshLocation = SkeletalMesh->GetComponentLocation();
		DrawDebugLine(World, skeletalMeshLocation, skeletalMeshLocation + CurrentTrajectory, Color, false, 0.05, 0, 5.f);
	}

	float CountTo(float MaxTime, float DeltaTime = 0.1)
	{
		static float timer = 0.0;
		timer += DeltaTime;
		timer = (timer < MaxTime) ? timer : 0.0f;

		return timer;
	}

	void ResetCounter()
	{
		CountTo(0.0f);
	}

	FTransform GetAnimBoneToRootTransform(const UAnimSequence * InSequence, const float AnimTime, int BoneIndex)
	{
		if (!InSequence || (BoneIndex == INDEX_NONE))
		{
			ensureMsgf(InSequence, TEXT("animation sequence is nullptr"));
			ensureMsgf((BoneIndex == INDEX_NONE), TEXT("BoneIndex is none"));

			return FTransform::Identity;
		}

		FTransform animBoneToRootTransform = FTransform::Identity;;
		const USkeleton* SourceSkeleton = InSequence->GetSkeleton();
		FReferenceSkeleton RefSkel = SourceSkeleton->GetReferenceSkeleton();

		if (RefSkel.IsValidIndex(BoneIndex))
		{
			InSequence->GetBoneTransform(animBoneToRootTransform, BoneIndex, AnimTime, false);

			while (RefSkel.GetParentIndex(BoneIndex) != INDEX_NONE)
			{
				const int parentIndex = RefSkel.GetParentIndex(BoneIndex);
				FTransform parentBoneTransform;
				InSequence->GetBoneTransform(parentBoneTransform, parentIndex, AnimTime, false);
	
				animBoneToRootTransform = animBoneToRootTransform * parentBoneTransform;
				BoneIndex = parentIndex;
			}
		}

		return animBoneToRootTransform;
	}
}


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
}

void FAnimNode_MotionMatching::Evaluate_AnyThread(FPoseContext& Output)
{
	if (!AnimationSequence)
	{
		ensureMsgf(false, TEXT("Animation sequence is nullptr"));

		return;
	}

	CurrentAnimTime = CountTo(MaxSequenceLength, AnimationSampling);

	FAnimKey lowestCostAnimKey = FindLowestCostAnimKey();
	if (LowestCostAnimkey != lowestCostAnimKey)
	{
		ResetCounter();
		CurrentAnimTime = 0.0f;
		LowestCostAnimkey = lowestCostAnimKey;
		MoveOwnerPawn();
	}

	AnimationSequence->GetAnimationPose(Output.Pose, Output.Curve,
		FAnimExtractContext(LowestCostAnimkey.AnimationStartTime + CurrentAnimTime, true));

#if DEBUG_DRAW
	DrawDebugAnimationPose();
	DrawDebugSkeletalMeshPose();
#endif //DEBUG_DRAW
}

FAnimKey FAnimNode_MotionMatching::FindLowestCostAnimKey()
{
	if (!AnimationSequence || !OwnerPawn)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("OwnerPawn is nullptr"));
		ensureMsgf(AnimationSequence, TEXT("Animation sequence is nullptr"));
#endif //WITH_EDITOR

		return FAnimKey{};
	}

	const float animLength = AnimationSequence->SequenceLength;
	float lowestAnimCost = BIG_NUMBER;
	float lowestCostAnimStartTime = -1.0f;

	for (float animTime = 0.0f; animTime < animLength; animTime += AnimationSampling)
	{
		float currentAnimCost = 0.0f;
		currentAnimCost += Trajectory * ComputeTrajectoryCost(animTime); 
		currentAnimCost += Pose * ComputePoseCost(animTime); 

		if (lowestAnimCost > currentAnimCost && animTime != LowestCostAnimkey.AnimationStartTime)
		{
			lowestAnimCost = currentAnimCost;
			lowestCostAnimStartTime = animTime;
		}
	}

#if DEBUG_DRAW
	const FTransform rootMotion = AnimationSequence->ExtractRootMotion(lowestCostAnimStartTime, AnimationSampling, false);
	DrawDebugTrajectory(World, SkeletalMeshComponent, rootMotion.GetTranslation() * 50, FColor::Red);
#endif //DEBUG_DRAW

	return FAnimKey{0, lowestCostAnimStartTime};
}


float FAnimNode_MotionMatching::ComputeTrajectoryCost(float AnimTime) const 
{
	if (!AnimationSequence)
	{
#if !WITH_EDITOR
		ensureMsgf(AnimationSequence, TEXT("Animation sequence is nullptr"));
#endif //WITH_EDITOR

		return 0.0f;
	}

	const FVector currentTrajectory = CalculateCurrentTrajectory();
	const FTransform& animTransform = AnimationSequence->ExtractRootMotion(AnimTime, AnimationSampling, false);

	return FVector::Dist(currentTrajectory, animTransform.GetTranslation());
}

float FAnimNode_MotionMatching::ComputePoseCost(float AnimTime) const
{
	if (!AnimationSequence || !SkeletalMeshComponent || BoneNames.Num() == 0)
	{
#if !WITH_EDITOR
		ensureMsgf(SkeletalMeshComponent, TEXT("SkeletalMeshComponent is nullptr"));
		ensureMsgf(AnimationSequence, TEXT("Animation sequence is nullptr"));
		ensureMsgf(BoneNames.Num() == 0, TEXT("Bone Names array is empty"));
#endif //WITH_EDITOR

		return 0.0f;
	}

	float Cost = 0.0f;

	for (const FName& boneName : BoneNames)
	{
		const int32 boneIndex = SkeletalMeshComponent->GetBoneIndex(boneName);

		FTransform newBoneTransform = GetBoneToRootTransform(AnimTime, boneIndex);

		const float currentTime = LowestCostAnimkey.AnimationStartTime + CurrentAnimTime;
		FTransform currentBoneTransform = GetBoneToRootTransform(currentTime, boneIndex);

		Cost += FVector::Dist(newBoneTransform.GetTranslation(), currentBoneTransform.GetTranslation());
	}

	return Cost;
}

FVector FAnimNode_MotionMatching::CalculateCurrentTrajectory() const
{
	if (!OwnerPawn || !SkeletalMeshComponent)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("OwnerPawn is nullptr"));
		ensureMsgf(SkeletalMeshComponent, TEXT("SkeletalMesh is nullptr"));
#endif //WITH_EDITOR
		
		return FVector{1, 1, 1};
	}

	const FRotator rotation = OwnerPawn->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);
	const FVector forwardDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);
	const FVector rightDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);
	const float forwardValue = OwnerPawn->GetInputAxisValue(FName{ "MoveForward" }); //TODO: find a better way of providing input axes names
	const float rightValue = OwnerPawn->GetInputAxisValue(FName{ "MoveRight" });
	const float trajectoryLength = 10.0f;

	return (forwardDirection * forwardValue + rightDirection * rightValue) * trajectoryLength;
}

FTransform FAnimNode_MotionMatching::GetBoneToRootTransform(float AnimTime, int32 BoneIndex) const
{
	FTransform rootTransform;
	const int32 rootBoneIndex = 0;
	AnimationSequence->GetBoneTransform(rootTransform, rootBoneIndex, AnimTime, false);

	FTransform boneTransform;
	AnimationSequence->GetBoneTransform(boneTransform, BoneIndex, AnimTime, false);

	return rootTransform.GetRelativeTransform(boneTransform);
}

void FAnimNode_MotionMatching::MoveOwnerPawn()
{
	if (!OwnerPawn || !AnimationSequence)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("OwnerPawn is nullptr"));
		ensureMsgf(AnimationSequence, TEXT("AnimationSequence is nullptr"));
#endif //WITH_EDITOR

		return;
	}

	const FTransform rootMotion = AnimationSequence->ExtractRootMotion(LowestCostAnimkey.AnimationStartTime, AnimationSampling, false);
	OwnerPawn->AddMovementInput(rootMotion.GetTranslation());

#if DEBUG_DRAW
	DrawDebugTrajectory(World, SkeletalMeshComponent, rootMotion.GetTranslation() * 50, FColor::Green);
#endif //DEBUG_DRAW
}

void FAnimNode_MotionMatching::DrawDebugAnimationPose()
{
	if (OwnerPawn && SkeletalMeshComponent)
	{
		FTransform skeletalMeshTransform = OwnerPawn->GetActorTransform();
		FTransform rootSkeletalTransform = SkeletalMeshComponent->GetBoneTransform(0);
		FTransform rootToPelvisSkeletalTransform = rootSkeletalTransform.GetRelativeTransform(skeletalMeshTransform);

		for (int i = 0; i < 68; ++i)
		{
			FTransform indexBoneTransform = GetAnimBoneToRootTransform(AnimationSequence, LowestCostAnimkey.AnimationStartTime, i);
			indexBoneTransform *= rootToPelvisSkeletalTransform;
			FTransform transformedBoneOnMeshTransform = indexBoneTransform * skeletalMeshTransform;
			FVector transformedBoneOnMeshTranslation = transformedBoneOnMeshTransform.GetTranslation();
			DrawDebugPoint(World, transformedBoneOnMeshTranslation, 3, FColor::Blue, false, 0.05, 0);
		}
	}
}

void FAnimNode_MotionMatching::DrawDebugSkeletalMeshPose()
{
	if (SkeletalMeshComponent)
	{
		for (int i = 0; i < 68; i+=5)
		{
			const FTransform& skeletalMeshBoneTransform = SkeletalMeshComponent->GetBoneTransform(i);
			DrawDebugPoint(World, skeletalMeshBoneTransform.GetTranslation() + FVector{ 0, 200, 0 }, 3, FColor::Green, false, 0.05, 0);
		}
	}
}

void FAnimNode_MotionMatching::DrawDebugBoneToRootPosition(float AnimTime, FColor Color, const FVector& Offset)
{
	FTransform newPoseRootTransform;
	AnimationSequence->GetBoneTransform(newPoseRootTransform, 0, AnimTime, false);

	for (const FName& boneName : BoneNames)
	{
		const int32 boneIndex = SkeletalMeshComponent->GetBoneIndex(boneName);
		FTransform newPoseBoneTransform;
		AnimationSequence->GetBoneTransform(newPoseBoneTransform, boneIndex, AnimTime, false);
		DrawDebugLine(World, newPoseRootTransform.GetTranslation() + Offset,
			newPoseBoneTransform.GetTranslation() + Offset, Color, false, 0.05, 0, 5.f);
	}
}
