#pragma once

#include "CoreMinimal.h"
#include "AnimNode_MotionMatching.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_MotionMatching.generated.h"


UCLASS()
class MOTIONMATCHING_API UAnimGraphNode_MotionMatching : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetNodeCategory() const override;

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_MotionMatching Node;

protected:
	virtual FString GetControllerDescription() const;
};
