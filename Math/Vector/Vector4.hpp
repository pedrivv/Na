#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <iostream>

struct Vector4
{
    float x, y, z, w;

    Vector4() : x(0), y(0), z(0), w(0) {}

    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    void print() const {
        std::cout << "Vector4(" << x << ", " << y << ", " << z << ", " << w << ")\n";
    }
};