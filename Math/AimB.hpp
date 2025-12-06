#pragma once

#include <Windows.h>
#include <imgui.h>
#include "Vector/Vector2.hpp"
#include "Quaternion.hpp"

struct Math {
    static float Abs(float value) {
        return value < 0.0f ? -value : value;
    }

    static Vector2 Abs(const Vector2& v) {
        return Vector2(Abs(v.X), Abs(v.Y));
    }
};

class AimB {
private:
    static constexpr float SMALL_FLOAT = 0.0000000001f;

public:
    static Quaternion GetRotationToLocation(const Vector3& targetLocation,
        float y_bias, const Vector3& myLoc) {
        Vector3 forwards = targetLocation + Vector3(0, y_bias, 0) - myLoc;
        Vector3 upwards(0, 1, 0);

        return LookRotation(forwards, upwards);
    }

private:
    static Quaternion LookRotation(Vector3 forwards, Vector3 upwards) {
        forwards = Normalized(forwards);
        upwards = Normalized(upwards);

        if (SqrMagnitude(forwards) < SMALL_FLOAT || SqrMagnitude(upwards) < SMALL_FLOAT)
            return Quaternion::Identity();

        if (1 - std::abs(Vector3::Dot(forwards, upwards)) < SMALL_FLOAT)
            return FromToRotation(forwards, upwards);

        Vector3 right = Normalized(Vector3::Cross(upwards, forwards));
        upwards = Vector3::Cross(forwards, right);

        Quaternion quaternion;
        float radicand = right.X + upwards.Y + forwards.Z;

        if (radicand > 0) {
            quaternion.W = std::sqrt(1.0f + radicand) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.W);
            quaternion.X = (upwards.Z - forwards.Y) * recip;
            quaternion.Y = (forwards.X - right.Z) * recip;
            quaternion.Z = (right.Y - upwards.X) * recip;
        }
        else if (right.X >= upwards.Y && right.X >= forwards.Z) {
            quaternion.X = std::sqrt(1.0f + right.X - upwards.Y - forwards.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.X);
            quaternion.W = (upwards.Z - forwards.Y) * recip;
            quaternion.Z = (forwards.X + right.Z) * recip;
            quaternion.Y = (right.Y + upwards.X) * recip;
        }
        else if (upwards.Y > forwards.Z) {
            quaternion.Y = std::sqrt(1.0f - right.X + upwards.Y - forwards.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.Y);
            quaternion.Z = (upwards.Z + forwards.Y) * recip;
            quaternion.W = (forwards.X - right.Z) * recip;
            quaternion.X = (right.Y + upwards.X) * recip;
        }
        else {
            quaternion.Z = std::sqrt(1.0f - right.X - upwards.Y + forwards.Z) * 0.5f;
            float recip = 1.0f / (4.0f * quaternion.Z);
            quaternion.Y = (upwards.Z + forwards.Y) * recip;
            quaternion.X = (forwards.X + right.Z) * recip;
            quaternion.W = (right.Y - upwards.X) * recip;
        }
        return quaternion;
    }

    static Quaternion FromToRotation(const Vector3& forwards, const Vector3& upwards) {
        float dot = Vector3::Dot(forwards, upwards);
        float k = std::sqrt(SqrMagnitude(forwards) * SqrMagnitude(upwards));

        if (std::abs(dot / k + 1) < 0.00001) {
            Vector3 ortho = Orthogonal(forwards);
            return Quaternion(Normalized(ortho).X, Normalized(ortho).Y, Normalized(ortho).Z, 0);
        }
        Vector3 cross = Vector3::Cross(forwards, upwards);
        return Normalized(Quaternion(cross.X, cross.Y, cross.Z, dot + k));
    }

    static Quaternion Normalized(const Quaternion& rotation) {
        float norm = Norm(rotation);
        return Quaternion(rotation.X / norm, rotation.Y / norm, rotation.Z / norm, rotation.W / norm);
    }

    static float Norm(const Quaternion& rotation) {
        return std::sqrt(rotation.X * rotation.X +
            rotation.Y * rotation.Y +
            rotation.Z * rotation.Z +
            rotation.W * rotation.W);
    }

    static Vector3 Orthogonal(const Vector3& v) {
        return v.Z < v.X ? Vector3(v.Y, -v.X, 0) : Vector3(0, -v.Z, v.Y);
    }

    static Vector3 Normalized(const Vector3& vector) {
        float mag = Magnitude(vector);

        if (mag == 0)
            return Vector3::Zero();
        return vector / mag;
    }

    static float Magnitude(const Vector3& vector) {
        return std::sqrt(SqrMagnitude(vector));
    }

    static float SqrMagnitude(const Vector3& v) {
        return v.X * v.X + v.Y * v.Y + v.Z * v.Z;
    }
};