#pragma once

struct FAnimKey
{
	int32 Index = 0;
	float StartTime = 0.0f;
};

inline bool operator==(const FAnimKey& Lhs, const FAnimKey& Rhs) 
{
	return (Lhs.Index == Rhs.Index) && FMath::IsNearlyEqual(Lhs.StartTime, Rhs.StartTime);
}

inline bool operator!=(const FAnimKey& Lhs, const FAnimKey& Rhs) 
{ 
	return !operator==(Lhs, Rhs); 
}

inline bool operator<(const FAnimKey& Lhs, const FAnimKey& Rhs) 
{ 
	return (Lhs.Index < Rhs.Index) ? true : ((Lhs.Index == Rhs.Index) ? Lhs.StartTime < Rhs.StartTime : false);
}

inline bool operator> (const FAnimKey& Lhs, const FAnimKey& Rhs) 
{ 
	return  operator<(Rhs, Lhs); 
}

inline bool operator<=(const FAnimKey& Lhs, const FAnimKey& Rhs) 
{ 
	return !operator>(Lhs, Rhs); 
}

inline bool operator>=(const FAnimKey& Lhs, const FAnimKey& Rhs) 
{ 
	return !operator<(Lhs, Rhs); 
}