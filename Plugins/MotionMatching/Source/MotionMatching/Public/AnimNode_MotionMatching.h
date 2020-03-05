#pragma once

#include "Animation/AnimNodeBase.h"
#include "AnimKey.h"
#include "AnimNode_MotionMatching.generated.h"


USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_MotionMatching : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	FAnimNode_MotionMatching() : FAnimNode_Base() {	}

	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;

	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float AnimationSampling = 0.05f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float Trajectory = 1.0f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float Pose = 1.0f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float MaxSequenceLength = 1.0f;

	UPROPERTY(EditAnywhere, Category = MotionData)
	UAnimSequence* AnimationSequence = nullptr;

	UPROPERTY(EditAnywhere, Category = MotionData)
	TArray<FName> BoneNames;

private:
	FAnimKey FindLowestCostAnimKey();
	float ComputeTrajectoryCost(float AnimTime) const;
	float ComputePoseCost(float AnimTime) const;
	FVector CalculateCurrentTrajectory() const;
	FTransform GetBoneToRootTransform(float AnimTime, int32 BoneIndex) const;

	void DrawDebugAnimationPose();
	void DrawDebugSkeletalMeshPose();
	void DrawDebugBoneToRootPosition(float AnimTime, FColor Color, const FVector& Offset);

	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	APawn* OwnerPawn = nullptr;
	UWorld* World = nullptr;
	FAnimKey LowestCostAnimkey = FAnimKey{0, 0.0f};
	float CurrentAnimTime = 0.0f;

};