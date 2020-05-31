#include "AnimContainer.h"

void FAnimContainer::Init(const TArray<UAnimSequence*>& InAnimationsArray, float InAnimationSampling)
{
	AnimationsArray = InAnimationsArray;
	AnimationSampling = InAnimationSampling;
}

void FAnimContainer::ResetKey()
{
	CurrentKey.StartTime = 0.0f;
	CurrentKey.Index = 0;
}

void FAnimContainer::IncrementKey()
{
	CurrentKey.StartTime += AnimationSampling;

	if (CurrentKey.StartTime > GetAnimation(CurrentKey).SequenceLength &&
		CurrentKey.Index < AnimationsArray.Num() - 1)
	{
		CurrentKey.Index += 1;
		CurrentKey.StartTime = 0.0f;
	}
}

FAnimKey FAnimContainer::MaxKey() const
{
	return FAnimKey{AnimationsArray.Num() - 1, AnimationsArray.Last()->SequenceLength};
}

UAnimSequence& FAnimContainer::GetAnimation(const FAnimKey& AnimKey) const
{
	return *(AnimationsArray[AnimKey.Index]);
}

FTransform FAnimContainer::ExtractRootMotion(const FAnimKey& AnimKey, float DeltaTime) const
{
	const UAnimSequence& animSequence = GetAnimation(AnimKey);

	if (AnimKey.StartTime > animSequence.SequenceLength)
	{
		return FTransform::Identity;
	}

	return animSequence.ExtractRootMotion(AnimKey.StartTime, DeltaTime, true);
}

void FAnimContainer::GetAnimationPose(FPoseContext& PoseContext, const FAnimKey& AnimKey) const
{
	const FAnimExtractContext& animExtractContext = FAnimExtractContext(AnimKey.StartTime, true);

	GetAnimation(AnimKey).GetAnimationPose(PoseContext.Pose, PoseContext.Curve, animExtractContext);
}