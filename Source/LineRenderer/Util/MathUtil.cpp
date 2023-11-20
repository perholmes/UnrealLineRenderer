// Copyright Hollywood Camera Work

#include "MathUtil.h"

float FMathUtil::SnapToWholeNumber(const float Input, const float Range)
{
    const float LowerBound = FMath::FloorToFloat(Input);
    const float UpperBound = FMath::CeilToFloat(Input);

    if (FMath::Abs(Input - LowerBound) <= Range) {
        return LowerBound;
    }

    else if (FMath::Abs(UpperBound - Input) <= Range) {
        return UpperBound;
    }

    return Input;
}
