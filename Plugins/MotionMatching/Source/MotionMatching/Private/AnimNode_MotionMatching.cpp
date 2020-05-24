#include "AnimNode_MotionMatching.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"


namespace
{
	FTransform GetAnimBoneToRootTransform(const UAnimSequence * InSequence, const float AnimTime, int32 BoneIndex)
	{
		if (!InSequence || (BoneIndex == INDEX_NONE))
		{
			ensureMsgf(InSequence, TEXT("animation sequence is nullptr"));
			ensureMsgf((BoneIndex == INDEX_NONE), TEXT("BoneIndex is none"));

			return FTransform::Identity;
		}

		FTransform animBoneToRootTransform = FTransform::Identity;;
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
}


void FAnimNode_MotionMatching::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	if (!InAnimInstance)
	{
		ensureAlwaysMsgf(false, TEXT("Animation instance is nullptr"));

		return;
	}
	
	SkeletalMeshComponent = InAnimInstance->GetSkelMeshComponent();
	OwnerPawn = InAnimInstance->TryGetPawnOwner();
	World = InAnimInstance->GetWorld();	
	BoneNames.Remove(NAME_None);
	LoadBoneToRootTransforms();
}

void FAnimNode_MotionMatching::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	const float deltaTime = Context.GetDeltaTime();

	GlobalDeltaTime = deltaTime;
	DebugTimer += deltaTime;

	if (DebugTimer > DebugRate)
	{
		UpdateTimer += deltaTime;
	}
}
void FAnimNode_MotionMatching::Evaluate_AnyThread(FPoseContext& Output)
{
	if (!AnimationSequence || !SkeletalMeshComponent)
	{
		ensureMsgf(false, TEXT("Animation sequence is nullptr"));

		return;
	}

	if (DebugTimer > DebugRate)
	{
		DebugTimer = 0.0f;
		if (UpdateTimer > UpdateRate)
		{
			LowestCostAnimkey = FindLowestCostAnimKey();
			UpdateTimer = 0.0f;

			if(IsDebugMode)
			{
				const FVector& currentTrajectory = CalculateCurrentTrajectory();
				DrawDebugTrajectory(currentTrajectory, FColor::Yellow);
	
				const float currentAnimationTime = LowestCostAnimkey.AnimationStartTime;
				const FTransform& animTransform = AnimationSequence->ExtractRootMotion(currentAnimationTime, UpdateRate, true);
				const FTransform& worldAnimTransform = SkeletalMeshComponent->ConvertLocalRootMotionToWorld(animTransform);
				DrawDebugTrajectory(worldAnimTransform.GetTranslation(), FColor::Red);
			}
		}

		const float animationTime = LowestCostAnimkey.AnimationStartTime + UpdateTimer;
		AnimationSequence->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext(animationTime, true));

		MoveOwnerPawn();
				
		PreviousAnimTime = LowestCostAnimkey.AnimationStartTime + UpdateTimer;
	}
	else
	{
		AnimationSequence->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext(LowestCostAnimkey.AnimationStartTime + UpdateTimer, true));
	}
}

FAnimKey FAnimNode_MotionMatching::FindLowestCostAnimKey()
{
	if (!AnimationSequence || !OwnerPawn)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("OwnerPawn is nullptr"));
		ensureMsgf(AnimationSequence, TEXT("Animation sequence is nullptr"));
#endif //WITH_EDITOR

		return FAnimKey{};
	}

	const float animLength = AnimationSequence->SequenceLength;
	float lowestAnimCost = BIG_NUMBER;
	float lowestCostAnimStartTime = -1.0f;
	CurrentTrajectory = CalculateCurrentTrajectory();

	for (float animTime = 0.0f; animTime < (animLength - (AnimationSampling * NumberOfStepsToMatch)); animTime += AnimationSampling)
	{
		float currentAnimCost = 0.0f;
		const FTransform& rootMotion = AnimationSequence->ExtractRootMotion(animTime, UpdateRate, true);

		if (TrajectoryWeight > 0)
		{
			currentAnimCost += TrajectoryWeight * ComputeTrajectoryCost(animTime, rootMotion);
		}

		if (PoseWeight > 0)
		{
			currentAnimCost += PoseWeight * ComputePoseCost(animTime);
		}

		if (OrientationWeight > 0)
		{
			currentAnimCost += OrientationWeight * ComputeOrientationCost(animTime, rootMotion);
		}

		if (lowestAnimCost > currentAnimCost)
		{
			lowestAnimCost = currentAnimCost;
			lowestCostAnimStartTime = animTime;

			if(IsDebugMode)
			{
				UE_LOG(LogTemp, Warning, TEXT("New lowestAnimCost: %f"), lowestAnimCost);
				UE_LOG(LogTemp, Warning, TEXT("New lowestCostAnimStartTime: %f"), lowestCostAnimStartTime);
				UE_LOG(LogTemp, Warning, TEXT("PreviousAnimTime: %f"), PreviousAnimTime);
				UE_LOG(LogTemp, Warning, TEXT("LowestCostAnimkey.AnimationStartTime + UpdateTimer: %f"), LowestCostAnimkey.AnimationStartTime + UpdateTimer);
				UE_LOG(LogTemp, Warning, TEXT("LowestCostAnimkey.AnimationStartTime: %f"), LowestCostAnimkey.AnimationStartTime);
				UE_LOG(LogTemp, Warning, TEXT("UpdateTimer: %f"), UpdateTimer);
				UE_LOG(LogTemp, Warning, TEXT("----------------------------------------"));
			}
		}
	}
	
	if (IsDebugMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("The lowestAnimCost: %f"), lowestAnimCost);
		UE_LOG(LogTemp, Warning, TEXT("The lowestCostAnimStartTime: %f"), lowestCostAnimStartTime);
	}

	return FAnimKey{0, lowestCostAnimStartTime};
}


float FAnimNode_MotionMatching::ComputeTrajectoryCost(float AnimTime, const FTransform& RootMotion) const 
{
	if (!AnimationSequence)
	{
#if !WITH_EDITOR
		ensureMsgf(AnimationSequence, TEXT("Animation sequence is nullptr"));
#endif //WITH_EDITOR

		return 0.0f;
	}

	const FTransform& worldAnimTransform = SkeletalMeshComponent->ConvertLocalRootMotionToWorld(RootMotion);
	FVector animTranslation = worldAnimTransform.GetTranslation();
	
	return FVector::Dist(CurrentTrajectory, animTranslation);
}

float FAnimNode_MotionMatching::ComputePoseCost(float AnimTime) const
{
	if (!AnimationSequence || !SkeletalMeshComponent || BoneNames.Num() == 0)
	{
#if !WITH_EDITOR
		ensureMsgf(SkeletalMeshComponent, TEXT("SkeletalMeshComponent is nullptr"));
		ensureMsgf(AnimationSequence, TEXT("Animation sequence is nullptr"));
		ensureMsgf(BoneNames.Num() == 0, TEXT("Bone Names array is empty"));
#endif //WITH_EDITOR

		return 0.0f;
	}

	float Cost = 0.0f;
		
	for (const FName& boneName : BoneNames)
	{
		const FTransform& newBoneTransform = GetLoadedBoneToRootTransform(boneName, AnimTime);
		const FTransform& previousBoneTransform = GetLoadedBoneToRootTransform(boneName, PreviousAnimTime);
	
		Cost += FTransform::SubtractTranslations(newBoneTransform, previousBoneTransform).Size();
	}

	return Cost;
}

float FAnimNode_MotionMatching::ComputeOrientationCost(float AnimTime, const FTransform& RootMotion) const
{
	if (AnimTime + UpdateRate > AnimationSequence->SequenceLength)
	{
		return BIG_NUMBER;
	}

	const FTransform& rootMotion = AnimationSequence->ExtractRootMotion(AnimTime, UpdateRate, true);
	const FTransform& rootSkeletalTransform = SkeletalMeshComponent->GetBoneTransform(0) * RootMotion;
	const FVector& orientation = rootSkeletalTransform.GetRotation().Vector().RotateAngleAxis(90.0f, FVector{ 0.0f, 0.0f, 1.0f });

	return FVector::Dist(orientation, CurrentTrajectory);
}

FVector FAnimNode_MotionMatching::CalculateCurrentTrajectory() const
{
	if (!OwnerPawn || !SkeletalMeshComponent)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("OwnerPawn is nullptr"));
		ensureMsgf(SkeletalMeshComponent, TEXT("SkeletalMesh is nullptr"));
#endif //WITH_EDITOR
		
		return FVector{1, 1, 1};
	}

	const FRotator rotation = OwnerPawn->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);
	const FVector forwardDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);
	const FVector rightDirection = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);
	const float forwardValue = OwnerPawn->GetInputAxisValue(FName{ "MoveForward" }); //TODO: find a better way of providing input axes names
	const float rightValue = OwnerPawn->GetInputAxisValue(FName{ "MoveRight" });

	return (forwardDirection * forwardValue + rightDirection * rightValue) * TrajectoryLength * NumberOfStepsToMatch;
}

void FAnimNode_MotionMatching::MoveOwnerPawn() const
{
	if (!OwnerPawn || !AnimationSequence || !SkeletalMeshComponent)
	{
#if !WITH_EDITOR
		ensureMsgf(OwnerPawn, TEXT("OwnerPawn is nullptr"));
		ensureMsgf(AnimationSequence, TEXT("AnimationSequence is nullptr"));
		ensureMsgf(SkeletalMeshComponent, TEXT("SkeletalMeshComponent is nullptr"));
#endif //WITH_EDITOR

		return;
	}

	const float animLength = AnimationSequence->SequenceLength;
	const float currentAnimationTime = LowestCostAnimkey.AnimationStartTime + UpdateTimer;
	
	if (currentAnimationTime > animLength)
	{
		return;
	}

	const FTransform& rootMotion = AnimationSequence->ExtractRootMotion(currentAnimationTime, GlobalDeltaTime, false);

	UCharacterMovementComponent* ownerPawnMovementComponent = Cast<UCharacterMovementComponent>(OwnerPawn->GetMovementComponent());
		
	if (ownerPawnMovementComponent)
	{
		// character movement component should convert the root motion to the world coordinates:
		ownerPawnMovementComponent->RootMotionParams.Set(rootMotion); 
	}
}

void FAnimNode_MotionMatching::DrawDebugAnimationPose()
{
	if (OwnerPawn && SkeletalMeshComponent)
	{
		FTransform skeletalMeshTransform = OwnerPawn->GetActorTransform();
		FTransform rootSkeletalTransform = SkeletalMeshComponent->GetBoneTransform(0);
		FTransform rootToPelvisSkeletalTransform = rootSkeletalTransform.GetRelativeTransform(skeletalMeshTransform);

		for (int i = 0; i < 68; ++i)
		{
			FTransform indexBoneTransform = GetAnimBoneToRootTransform(AnimationSequence, LowestCostAnimkey.AnimationStartTime, i);
			indexBoneTransform *= rootToPelvisSkeletalTransform;
			FTransform transformedBoneOnMeshTransform = indexBoneTransform * skeletalMeshTransform;
			FVector transformedBoneOnMeshTranslation = transformedBoneOnMeshTransform.GetTranslation();
			DrawDebugPoint(World, transformedBoneOnMeshTranslation, 10, FColor::Red, false, DebugLinesLifetime, 0);
		}
	}
}

void FAnimNode_MotionMatching::DrawDebugSkeletalMeshPose()
{
	if (SkeletalMeshComponent)
	{
		for (int i = 0; i < 68; i+=5)
		{
			const FTransform& skeletalMeshBoneTransform = SkeletalMeshComponent->GetBoneTransform(i);
			DrawDebugPoint(World, skeletalMeshBoneTransform.GetTranslation() + FVector{ 0, 200, 0 }, 3, FColor::Green, false, DebugLinesLifetime, 0);
		}
	}
}

void FAnimNode_MotionMatching::DrawDebugSkeletalMeshBoneToRootPosition()
{
	if (!SkeletalMeshComponent)
	{
		return;
	}

	const FVector offset = FVector::ZeroVector;

	const int32 rootBoneIndex = SkeletalMeshComponent->GetBoneIndex(RootBoneName);
	const FTransform& skeletalMeshRootBoneTransform = SkeletalMeshComponent->GetBoneTransform(rootBoneIndex);

	for (const FName& boneName : BoneNames)
	{
		const int32 boneIndex = SkeletalMeshComponent->GetBoneIndex(boneName);
		const FTransform& skeletalMeshBoneTransform = SkeletalMeshComponent->GetBoneTransform(boneIndex);

		UKismetSystemLibrary::DrawDebugLine(World, skeletalMeshRootBoneTransform.GetTranslation() + offset, skeletalMeshBoneTransform.GetTranslation() + offset, FColor::Green, DebugLinesLifetime, 2.0f);
	}

}

void FAnimNode_MotionMatching::DrawDebugTrajectory(const FVector& Trajectory, const FColor& Color) const
{
	if (!World || !SkeletalMeshComponent)
	{
#if !WITH_EDITOR
		ensureMsgf(World, TEXT("World instance is nullptr"));
		ensureMsgf(SkeletalMesh, TEXT("SkeletalMesh is nullptr"));
#endif //WITH_EDITOR

		return;
	}

	const FVector& skeletalMeshLocation = SkeletalMeshComponent->GetComponentLocation();
	UKismetSystemLibrary::DrawDebugLine(World, skeletalMeshLocation, skeletalMeshLocation + (Trajectory * 10), Color, DebugLinesLifetime, 2.0f);
}

void FAnimNode_MotionMatching::LoadBoneToRootTransforms()
{
	if (!AnimationSequence || !SkeletalMeshComponent)
	{
		return;
	}

	FootLeftIndex = SkeletalMeshComponent->GetBoneIndex(FName(TEXT("foot_l")));
	FootRightIndex = SkeletalMeshComponent->GetBoneIndex(FName(TEXT("foot_r")));
	HeadIndex = SkeletalMeshComponent->GetBoneIndex(FName(TEXT("head")));
	HandLeftIndex = SkeletalMeshComponent->GetBoneIndex(FName(TEXT("hand_l")));
	HandRightIndex = SkeletalMeshComponent->GetBoneIndex(FName(TEXT("hand_r")));
	PelvisIndex = SkeletalMeshComponent->GetBoneIndex(FName(TEXT("pelvis")));

	const float animLength = AnimationSequence->SequenceLength;

	for (float animTime = 0.0f; animTime < (animLength - (AnimationSampling * NumberOfStepsToMatch)); animTime += AnimationSampling)
	{
		FootLeftToRootTransforms.Emplace(GetAnimBoneToRootTransform(AnimationSequence, animTime, FootLeftIndex));
		FootRightToRootTransforms.Emplace(GetAnimBoneToRootTransform(AnimationSequence, animTime, FootRightIndex));
		HeadToRootTransforms.Emplace(GetAnimBoneToRootTransform(AnimationSequence, animTime, HeadIndex));
		HandLeftToRootTransforms.Emplace(GetAnimBoneToRootTransform(AnimationSequence, animTime, HandLeftIndex));
		HandRightToRootTransforms.Emplace(GetAnimBoneToRootTransform(AnimationSequence, animTime, HandRightIndex));
		PelvisToRootTransforms.Emplace(GetAnimBoneToRootTransform(AnimationSequence, animTime, PelvisIndex));
	}
}

FTransform FAnimNode_MotionMatching::GetLoadedBoneToRootTransform(const FName& BoneName, float AnimTime) const
{
	const int32 keyIndex = static_cast<int32>(AnimTime / AnimationSampling);

	const bool isKeyIndexValid = FootLeftToRootTransforms.IsValidIndex(keyIndex);
	const bool isIndexAssigned = (FootLeftIndex != INDEX_NONE);

	if (!isKeyIndexValid || !isIndexAssigned)
	{
		ensureMsgf(isKeyIndexValid, TEXT("Current key index does not match any preloaded bone to root transform"));
		ensureMsgf(isIndexAssigned, TEXT("Bone index has not been assigned"));

		return FTransform::Identity;
	}

	const int32 boneIndex = SkeletalMeshComponent->GetBoneIndex(BoneName);
	
	if (boneIndex == FootLeftIndex)
	{
		return FootLeftToRootTransforms[keyIndex];
	}
	else if (boneIndex == FootRightIndex)
	{
		return FootRightToRootTransforms[keyIndex];
	}
	else if (boneIndex == HeadIndex)
	{
		return HeadToRootTransforms[keyIndex];
	}
	else if (boneIndex == HandLeftIndex)
	{
		return HandLeftToRootTransforms[keyIndex];
	}
	else if (boneIndex == HandRightIndex)
	{
		return HandRightToRootTransforms[keyIndex];
	}
	else if (boneIndex == PelvisIndex)
	{
		return PelvisToRootTransforms[keyIndex];
	}

	return FTransform::Identity;
}
