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
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;

	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float AnimationSampling = 0.05f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float TrajectoryWeight = 1.0f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float PoseWeight = 1.0f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float OrientationWeight = 1.0f;

	//TODO: NumberOfStepsToMatch and TrajectoryLength might not be needed anymore after adding velocity matching,
	//      and changing new trajectory computation to compare vector's rotations instead of distances
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	int32 NumberOfStepsToMatch = 5;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float TrajectoryLength = 2.0f;
	
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	bool IsDebugMode = false;
	float DebugTimer = 0.0f;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float DebugRate = 0.2;
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float DebugLinesLifetime = 3.0f;
	
	UPROPERTY(EditAnywhere, Category = Parameters, meta = (PinShownByDefault))
	float UpdateRate = 0.2f;
	float UpdateTimer = 0.0f;

	UPROPERTY(EditAnywhere, Category = MotionData)
	UAnimSequence* AnimationSequence = nullptr;
	UPROPERTY(EditAnywhere, Category = MotionData)
	TArray<FName> BoneNames;
	UPROPERTY(EditAnywhere, Category = MotionData)
	FName RootBoneName; //TODO: check if this is needed

private:
	FAnimKey FindLowestCostAnimKey();
	float ComputeTrajectoryCost(float AnimTime, const FTransform& RootMotion) const;
	float ComputePoseCost(float AnimTime) const;
	float ComputeOrientationCost(float AnimTime, const FTransform& RootMotion) const;
	FVector CalculateCurrentTrajectory() const;
	void MoveOwnerPawn() const;
	void DrawDebugAnimationPose();
	void DrawDebugSkeletalMeshPose();
	void DrawDebugSkeletalMeshBoneToRootPosition();
	void DrawDebugTrajectory(const FVector& Trajectory, const FColor& Color = FColor::Green) const;

	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	APawn* OwnerPawn = nullptr;
	UWorld* World = nullptr;
	FAnimKey LowestCostAnimkey = FAnimKey{0, 0.0f};
	float PreviousAnimTime = 0.0f;
	float GlobalDeltaTime = 0.0f;
	FVector CurrentTrajectory;

	//TODO: Make LoadBoneToRootTransforms and GetLoadedBoneToRootTransform more generic
	void LoadBoneToRootTransforms();
	FTransform GetLoadedBoneToRootTransform(const FName& BoneName, float AnimTime) const;

	TArray<FTransform> FootLeftToRootTransforms;
	TArray<FTransform> FootRightToRootTransforms;
	TArray<FTransform> HeadToRootTransforms;
	TArray<FTransform> HandLeftToRootTransforms;
	TArray<FTransform> HandRightToRootTransforms;
	TArray<FTransform> PelvisToRootTransforms;

	int32 FootLeftIndex = INDEX_NONE;
	int32 FootRightIndex = INDEX_NONE;
	int32 HeadIndex = INDEX_NONE; 
	int32 HandLeftIndex = INDEX_NONE; 
	int32 HandRightIndex = INDEX_NONE; 
	int32 PelvisIndex = INDEX_NONE; 

};