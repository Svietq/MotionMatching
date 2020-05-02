#pragma once

struct FAnimKey
{
	int32 AnimationIndex = 0;
	float AnimationStartTime = 0.0f;
	uint32 KeyIndex = 0;
};

inline bool operator==(const FAnimKey& Lhs, const FAnimKey& Rhs) 
{
	return (Lhs.AnimationIndex == Rhs.AnimationIndex) && (Lhs.AnimationStartTime == Rhs.AnimationStartTime);
}

inline bool operator!=(const FAnimKey& Lhs, const FAnimKey& Rhs) 
{ 
	return !operator==(Lhs, Rhs); 
}
