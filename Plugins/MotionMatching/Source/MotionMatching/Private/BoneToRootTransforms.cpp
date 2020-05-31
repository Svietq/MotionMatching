#include "BoneToRootTransforms.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"

FBoneToRootTransforms::FBoneToRootTransforms(class USkeletalMeshComponent* InSkeletalMeshComponent, UAnimSequence* InAnimSequence, float InAnimationSampling)
{
	if (InSkeletalMeshComponent)
	{
		FootLeftIndex = InSkeletalMeshComponent->GetBoneIndex(FName(TEXT("foot_l")));
		FootRightIndex = InSkeletalMeshComponent->GetBoneIndex(FName(TEXT("foot_r")));
		HeadIndex = InSkeletalMeshComponent->GetBoneIndex(FName(TEXT("head")));
		HandLeftIndex = InSkeletalMeshComponent->GetBoneIndex(FName(TEXT("hand_l")));
		HandRightIndex = InSkeletalMeshComponent->GetBoneIndex(FName(TEXT("hand_r")));
		PelvisIndex = InSkeletalMeshComponent->GetBoneIndex(FName(TEXT("pelvis")));
	}

	if (InAnimSequence)
	{
		LoadBoneToRootTransforms(InAnimSequence, InAnimationSampling);
	}
}

FTransform FBoneToRootTransforms::GetTransform(int32 BoneIndex, int32 KeyIndex) const
{
	if (!FootLeft.IsValidIndex(KeyIndex))
	{
		ensureMsgf(false, TEXT("Current key index does not match any preloaded bone to root transform"));

		return FTransform::Identity;
	}

	if (BoneIndex == FootLeftIndex)
	{
		return FootLeft[KeyIndex];
	}
	else if (BoneIndex == FootRightIndex)
	{
		return FootRight[KeyIndex];
	}
	else if (BoneIndex == HeadIndex)
	{
		return Head[KeyIndex];
	}
	else if (BoneIndex == HandLeftIndex)
	{
		return HandLeft[KeyIndex];
	}
	else if (BoneIndex == HandRightIndex)
	{
		return HandRight[KeyIndex];
	}
	else if (BoneIndex == PelvisIndex)
	{
		return Pelvis[KeyIndex];
	}

	return FTransform::Identity;
}

void FBoneToRootTransforms::LoadBoneToRootTransforms(UAnimSequence* InAnimSequence, float InAnimationSampling)
{
	const float animLength = InAnimSequence->SequenceLength;

	for (float animTime = 0.0f; animTime < animLength; animTime += InAnimationSampling)
	{
		FootLeft.Emplace(CalculateBoneToRootTransform(InAnimSequence, animTime, FootLeftIndex));
		FootRight.Emplace(CalculateBoneToRootTransform(InAnimSequence, animTime, FootRightIndex));
		Head.Emplace(CalculateBoneToRootTransform(InAnimSequence, animTime, HeadIndex));
		HandLeft.Emplace(CalculateBoneToRootTransform(InAnimSequence, animTime, HandLeftIndex));
		HandRight.Emplace(CalculateBoneToRootTransform(InAnimSequence, animTime, HandRightIndex));
		Pelvis.Emplace(CalculateBoneToRootTransform(InAnimSequence, animTime, PelvisIndex));
	}
}

FTransform FBoneToRootTransforms::CalculateBoneToRootTransform(const UAnimSequence * InSequence, const float AnimTime, int32 BoneIndex)
{
	FTransform animBoneToRootTransform = FTransform::Identity;
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