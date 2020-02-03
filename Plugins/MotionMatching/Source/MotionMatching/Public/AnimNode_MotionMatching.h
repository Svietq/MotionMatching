#pragma once

#include "Animation/AnimNodeBase.h"
#include "AnimKey.h"
#include "AnimNode_MotionMatching.generated.h"


USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_MotionMatching : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	FAnimNode_MotionMatching();

	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Parameters, meta = (PinShownByDefault))
	float AnimationSampling = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Parameters, meta = (PinShownByDefault))
	float MaxSequenceLength = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MotionData)
	UAnimSequence* AnimationSequence = nullptr;

private:
	FAnimKey FindLowestCostAnimKey();
	FVector CalculateCurrentTrajectory();

	USkeletalMeshComponent* SkeletalMesh = nullptr;
	APawn* OwnerPawn = nullptr;
	UWorld* World = nullptr;
	FAnimKey LowestCostAnimkey = FAnimKey{0, 0.0f};

};