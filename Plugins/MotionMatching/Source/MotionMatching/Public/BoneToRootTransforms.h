#pragma once

struct FBoneToRootTransforms
{
	FBoneToRootTransforms(class USkeletalMeshComponent* InSkeletalMeshComponent, class UAnimSequence* InAnimSequence, float InAnimationSampling);

	FTransform GetTransform(int32 BoneIndex, int32 KeyIndex) const;

private:
	void LoadBoneToRootTransforms(class UAnimSequence* InAnimSequence, float InAnimationSampling);
	FTransform CalculateBoneToRootTransform(const class UAnimSequence * InSequence, const float AnimTime, int32 BoneIndex);

	//TODO: Make this more generic
	UPROPERTY()
	TArray<FTransform> FootLeft;
	UPROPERTY()
	TArray<FTransform> FootRight;
	UPROPERTY()
	TArray<FTransform> Head;
	UPROPERTY()
	TArray<FTransform> HandLeft;
	UPROPERTY()
	TArray<FTransform> HandRight;
	UPROPERTY()
	TArray<FTransform> Pelvis;

	int32 FootLeftIndex = 0;
	int32 FootRightIndex = 0;
	int32 HeadIndex = 0;
	int32 HandLeftIndex = 0;
	int32 HandRightIndex = 0;
	int32 PelvisIndex = 0;

};