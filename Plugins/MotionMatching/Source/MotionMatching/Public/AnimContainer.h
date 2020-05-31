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
	UAnimSequence& GetAnimation(const FAnimKey& AnimKey) const;
	FTransform ExtractRootMotion(const FAnimKey& AnimKey, float DeltaTime) const;
	void GetAnimationPose(FPoseContext& PoseContext, const FAnimKey& AnimKey) const;

	FAnimKey CurrentKey;

private:
	TArray<UAnimSequence*> AnimationsArray;
	float AnimationSampling = 0.0f;
};