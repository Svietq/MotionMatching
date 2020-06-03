#pragma once

#include "Animation/AnimNodeBase.h"
#include "AnimKey.h"
#include "AnimContainer.h"
#include "BoneToRootTransforms.h"

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
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;

	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float AnimationSampling = 0.05f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float TrajectoryWeight = 1.0f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float PoseWeight = 1.0f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float OrientationWeight = 1.0f;

	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float UpdateRate = 0.2f;
	float UpdateTimer = 0.0f;

	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float TrajectoryLength = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float BlendWeightDecrement = 0.01f;

	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	bool IsDebugMode = false;
	float DebugTimer = 0.0f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float DebugRate = 0.2;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float DebugLinesLifetime = 3.0f;

	UPROPERTY(EditAnywhere, Category = MotionData)
	TArray<UAnimSequence*> AnimationsArray;

	UPROPERTY(EditAnywhere, Category = MotionData)
	TArray<FName> BoneNames;

private:
	FAnimKey FindLowestCostAnimKey();
	float ComputeTrajectoryCost(const FTransform& RootMotion) const;
	float ComputePoseCost() const;
	float ComputeOrientationCost(float AnimTime, const FTransform& RootMotion) const;
	FVector CalculateCurrentTrajectory() const;
	void MoveOwnerPawn() const;
	void LoadBoneToRootTransforms();
	FTransform GetLoadedBoneToRootTransform(const FName& BoneName, const FAnimKey& AnimKey) const;
	void DrawDebugTrajectory(const FVector& Trajectory, const FColor& Color = FColor::Green) const;

	FAnimContainer AnimationContainer;
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	APawn* OwnerPawn = nullptr;
	UWorld* World = nullptr;
	FAnimKey LowestCostAnimkey = FAnimKey{0, 0.0f};
	FAnimKey PreviousAnimKey = FAnimKey{ 0, 0.0f };
	FAnimKey NewAnimKey = FAnimKey{ 0, 0.0f };
	float GlobalDeltaTime = 0.0f;
	FVector CurrentTrajectory;
	TArray<FBoneToRootTransforms> BoneToRootTransformsArray;
	float BlendWeight = 1.0f;

};