#include "AnimNode_MotionMatching.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"

#define DEBUG_DRAW 1

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
}

FAnimNode_MotionMatching::FAnimNode_MotionMatching() : FAnimNode_Base()
{

}

void FAnimNode_MotionMatching::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	if (!InAnimInstance)
	{
		ensureAlwaysMsgf(false, TEXT("Animation instance is nullptr"));

		return;
	}
	
	SkeletalMesh = InAnimInstance->GetSkelMeshComponent();
	OwnerPawn = InAnimInstance->TryGetPawnOwner();
	World = InAnimInstance->GetWorld();	
}

void FAnimNode_MotionMatching::Evaluate_AnyThread(FPoseContext& Output)
{
	if (!AnimationSequence)
	{
		ensureMsgf(false, TEXT("Animation sequence is nullptr"));

		return;
	}

	const FAnimKey newLowestCostAnimkey = FindLowestCostAnimKey();
	if (LowestCostAnimkey != newLowestCostAnimkey)
	{
		ResetCounter();
		LowestCostAnimkey = newLowestCostAnimkey;
	}

	AnimationSequence->GetAnimationPose(Output.Pose, Output.Curve, 
		FAnimExtractContext(LowestCostAnimkey.AnimationStartTime + CountTo(MaxSequenceLength, AnimationSampling), true));
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

	FVector currentTrajectory = CalculateCurrentTrajectory();
#if DEBUG_DRAW
	DrawDebugTrajectory(World, SkeletalMesh, currentTrajectory);
#endif //DEBUG_DRAW

	const float animLength = AnimationSequence->SequenceLength;
	float lowestAnimCost = BIG_NUMBER;
	float lowestCostAnimStartTime = 0.0f;

	for (float animTime = 0.0f; animTime < animLength; animTime += AnimationSampling)
	{
		const FTransform& animTransform = AnimationSequence->ExtractRootMotion(animTime, AnimationSampling, false);
		const float currentAnimCost = FVector::Dist(currentTrajectory, animTransform.GetTranslation());
		if (lowestAnimCost > currentAnimCost)
		{
			lowestAnimCost = currentAnimCost;
			lowestCostAnimStartTime = animTime;
		}
	}

#if DEBUG_DRAW
	const FTransform& animTransform = AnimationSequence->ExtractRootMotion(lowestCostAnimStartTime, AnimationSampling, false);
	DrawDebugTrajectory(World, SkeletalMesh, animTransform.GetTranslation(), FColor::Red);
#endif //DEBUG_DRAW

	return FAnimKey{0, lowestCostAnimStartTime};
}

FVector FAnimNode_MotionMatching::CalculateCurrentTrajectory()
{
	if (!OwnerPawn || !SkeletalMesh)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("OwnerPawn is nullptr"));
		ensureMsgf(SkeletalMesh, TEXT("SkeletalMesh is nullptr"));
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
