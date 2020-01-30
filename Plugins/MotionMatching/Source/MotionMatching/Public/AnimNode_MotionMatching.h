#pragma once

#include "Animation/AnimNodeBase.h"
#include "AnimNode_MotionMatching.generated.h"


struct FAnimKey
{
	int32 AnimationIndex = 0;
	float AnimationStartTime = 0.0f;
};


USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_MotionMatching : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	FAnimNode_MotionMatching();

	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MotionData, meta = (PinShownByDefault))
	float Responsiveness = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MotionData, meta = (PinShownByDefault))
	float VelocityStrength = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MotionData, meta = (PinShownByDefault))
	float PoseStrength = 1.0f;

private:
	FAnimKey* FindLowestCostAnimKey();
	FVector CalculateCurrentTrajectory();

	USkeletalMeshComponent* SkeletalMesh = nullptr;
	APawn* OwnerPawn = nullptr;
	UWorld* World = nullptr;
};