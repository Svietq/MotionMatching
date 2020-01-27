#include "AnimGraphNode_MotionMatching.h"

FLinearColor UAnimGraphNode_MotionMatching::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

FString UAnimGraphNode_MotionMatching::GetNodeCategory() const
{
	return TEXT("Animation tools");
}

FString UAnimGraphNode_MotionMatching::GetControllerDescription() const
{
	return TEXT("Motion Matching");
}

FText UAnimGraphNode_MotionMatching::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Motion Matching"));
}
