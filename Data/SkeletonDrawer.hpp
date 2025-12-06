#pragma once
#include <imgui.h>

struct SkeletonDrawer {
    ImDrawList* drawList;
    ImU32 color;
    float thickness;

    inline void Bone(const ImVec2& from, const ImVec2& to) const {
        drawList->AddLine(from, to, color, thickness);
    }

    inline void Head(const ImVec2& center, float radius) const {
        drawList->AddCircle(center, radius, color, 0, thickness);
    }
};
