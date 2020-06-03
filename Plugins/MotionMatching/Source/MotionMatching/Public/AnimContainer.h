#pragma once

#include "AnimKey.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNodeBase.h"


struct FAnimContainer
{
public:
	void Init(const TArray<UAnimSequence*>& InAnimationsArray, float InAnimationSampling);
	void ResetKey();
	void IncrementKey();
	FAnimKey MaxKey() const;
	const UAnimSequence& GetAnimation(const FAnimKey& AnimKey) const;
	FTransform ExtractBlendedRootMotion(const FAnimKey& PreviousAnimKey, const FAnimKey& NewAnimKey, float BlendWeight, float DeltaTime) const;
	FTransform ExtractRootMotion(const FAnimKey& AnimKey, float DeltaTime) const;
	void GetBlendedPose(FPoseContext& PoseContext, const FAnimKey& PreviousAnimKey, const FAnimKey& NewAnimKey, float BlendWeight) const;
	void GetPose(FPoseContext& PoseContext, const FAnimKey& AnimKey) const;

	FAnimKey CurrentKey;

private:
	TArray<UAnimSequence*> AnimationsArray;
	float AnimationSampling = 0.0f;

};