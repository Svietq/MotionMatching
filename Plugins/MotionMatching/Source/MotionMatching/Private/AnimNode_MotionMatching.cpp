#include "AnimNode_MotionMatching.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"

namespace
{
	void DrawDebugTrajectory(UWorld* World, USkeletalMeshComponent* SkeletalMesh, const FVector& currentTrajectory)
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
		DrawDebugLine(World, skeletalMeshLocation, skeletalMeshLocation + currentTrajectory, FColor::Green, false, 0.05, 0, 5.f);
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
	FAnimKey* lowestCostAnimkey = FindLowestCostAnimKey();
	//TODO: PlayAnimKey(lowestCostAnimkey);
}

FAnimKey* FAnimNode_MotionMatching::FindLowestCostAnimKey()
{
	FVector currentTrajectory = CalculateCurrentTrajectory();
	DrawDebugTrajectory(World, SkeletalMesh, currentTrajectory);

	//TODO:
	//go through all poses
		//calculate trajectory per pose (extract root motion)
		//compare to current trajectory
		//save if is the lowest

	//return the lowest

	return nullptr;
}

FVector FAnimNode_MotionMatching::CalculateCurrentTrajectory()
{
	if (!OwnerPawn || !SkeletalMesh)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("PawnOwner is nullptr"));
		ensureMsgf(SkeletalMesh, TEXT("SkeletalMesh is nullptr"));
#endif //WITH_EDITOR
		
		return FVector{};
	}

	const FRotator rotation = OwnerPawn->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);
	const FVector forwardDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);
	const FVector rightDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);
	const float forwardValue = OwnerPawn->GetInputAxisValue(FName{ "MoveForward" }); //TODO: find a better way of providing input axes names
	const float rightValue = OwnerPawn->GetInputAxisValue(FName{ "MoveRight" });
	const float trajectoryLength = 100.0f;

	return (forwardDirection * forwardValue + rightDirection * rightValue) * trajectoryLength;
}
