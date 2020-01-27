#pragma once

#include "Animation/AnimNodeBase.h"
#include "AnimNode_MotionMatching.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_MotionMatching : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	FAnimNode_MotionMatching();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MotionData, meta = (PinShownByDefault))
	float Responsiveness;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MotionData, meta = (PinShownByDefault))
	float VelocityStrength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MotionData, meta = (PinShownByDefault))
	float PoseStrength;

};