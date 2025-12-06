#pragma once

#include <imgui.h>
#include "Vector/Vector2.hpp"
#include "Vector/Vector3.hpp"
#include "Matrix4v4.hpp"

class W2S {
public:
    static Vector2 WorldToScreen(const Matrix4x4& viewMatrix, const Vector3& pos, int width, int height) {
        Vector2 result(-1, -1);

        if (width <= 0 || height <= 0) {
            return result;
        }

        float v9 = (pos.X * viewMatrix.m00) + (pos.Y * viewMatrix.m10) + (pos.Z * viewMatrix.m20) + viewMatrix.m30;
        float v10 = (pos.X * viewMatrix.m01) + (pos.Y * viewMatrix.m11) + (pos.Z * viewMatrix.m21) + viewMatrix.m31;
        float v12 = (pos.X * viewMatrix.m03) + (pos.Y * viewMatrix.m13) + (pos.Z * viewMatrix.m23) + viewMatrix.m33;

        if (v12 >= 0.01f) {
            float v13 = static_cast<float>(width) / 2.0f;
            float v14 = static_cast<float>(height) / 2.0f;

            result.X = v13 + (v13 * v9) / v12;
            result.Y = v14 - (v14 * v10) / v12;
        }

        return result;
    }

    static ImVec2 WorldToScreenImVec2(const Matrix4x4& viewMatrix, const Vector3& pos, int width, int height) {
        Vector2 result = WorldToScreen(viewMatrix, pos, width, height);
        return ImVec2(result.X, result.Y);
    }
};

//float v9 = (pos.X * viewMatrix.m00) + (pos.Y * viewMatrix.m10) + (pos.Z * viewMatrix.m20) + viewMatrix.m30;
//float v10 = (pos.X * viewMatrix.m01) + (pos.Y * viewMatrix.m11) + (pos.Z * viewMatrix.m21) + viewMatrix.m31;
//float v12 = (pos.X * viewMatrix.m03) + (pos.Y * viewMatrix.m13) + (pos.Z * viewMatrix.m23) + viewMatrix.m33;
