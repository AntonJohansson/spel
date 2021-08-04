#pragma once

#include "types.h"

struct v2 {
    f32 x, y;
};

#define V2(x,y)   (struct v2){x,y}
