#pragma once

#include <cstdint>
#include "Vector/Vector4.hpp"
#include "Quaternion.hpp"
#include <EspLines/Memory/Memory.hpp>

struct TMatrix {
    Vector4 position;
    Quaternion rotation;
    Vector4 scale;
};

class TransformUtils {
public:
    static bool GetPosition(uint32_t transform, Vector3& pos) {
        pos = Vector3::Zero();

        uint32_t transformObjValue;
        if (!Mem.Read(transform + 0x8, transformObjValue)) {
            return false;
        }

        uint32_t indexValue;
        if (!Mem.Read(transformObjValue + 0x24, indexValue)) {
            return false;
        }

        uint32_t matrixValue;
        if (!Mem.Read(transformObjValue + 0x20, matrixValue)) {
            return false;
        }

        uint32_t matrixListValue;
        if (!Mem.Read(matrixValue + 0x10, matrixListValue)) {
            return false;
        }

        uint32_t matrixIndicesValue;
        if (!Mem.Read(matrixValue + 0x14, matrixIndicesValue)) {
            return false;
        }

        Vector3 resultValue;
        if (!Mem.Read(indexValue * 0x30 + matrixListValue, resultValue)) {
            return false;
        }

        int maxTries = 50;
        int tries = 0;
        int transformIndexValue;

        if (!Mem.Read((uint32_t)((indexValue * 0x4) + matrixIndicesValue), transformIndexValue)) {
            return false;
        }

        while (transformIndexValue >= 0) {
            if (++tries == maxTries) break;

            TMatrix tMatrixValue;
            if (!Mem.Read((uint32_t)(0x30 * transformIndexValue + matrixListValue), tMatrixValue)) {
                return false;
            }

            float rotX = tMatrixValue.rotation.X;
            float rotY = tMatrixValue.rotation.Y;
            float rotZ = tMatrixValue.rotation.Z;
            float rotW = tMatrixValue.rotation.W;

            float scaleX = resultValue.X * tMatrixValue.scale.x;
            float scaleY = resultValue.Y * tMatrixValue.scale.x;
            float scaleZ = resultValue.Z * tMatrixValue.scale.z;

            resultValue.X = tMatrixValue.position.x + scaleX +
                (scaleX * ((rotY * rotY * -2.0f) - (rotZ * rotZ * 2.0f))) +
                (scaleY * ((rotW * rotZ * -2.0f) - (rotY * rotX * -2.0f))) +
                (scaleZ * ((rotZ * rotX * 2.0f) - (rotW * rotY * -2.0f)));
            resultValue.Y = tMatrixValue.position.y + scaleY +
                (scaleX * ((rotX * rotY * 2.0f) - (rotW * rotZ * -2.0f))) +
                (scaleY * ((rotZ * rotZ * -2.0f) - (rotX * rotX * 2.0f))) +
                (scaleZ * ((rotW * rotX * -2.0f) - (rotZ * rotY * -2.0f)));
            resultValue.Z = tMatrixValue.position.z + scaleZ +
                (scaleX * ((rotW * rotY * -2.0f) - (rotX * rotZ * -2.0f))) +
                (scaleY * ((rotY * rotZ * 2.0f) - (rotW * rotX * -2.0f))) +
                (scaleZ * ((rotX * rotX * -2.0f) - (rotY * rotY * 2.0f)));

            if (!Mem.Read((uint32_t)(transformIndexValue * 0x4 + matrixIndicesValue), transformIndexValue)) {
                return false;
            }
        }

        pos = resultValue;
        return tries != maxTries;
    }

    static bool GetNodePosition(uint32_t nodeTransform, Vector3& result) {
        uint32_t transformValue;
        if (!Mem.Read(nodeTransform + 0x8, transformValue)) {
            result = Vector3::Zero();
            return false;
        }
        return GetPosition(transformValue, result);
    }
};