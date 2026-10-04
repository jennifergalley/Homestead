#pragma once

#include "CoreMinimal.h"
#include "HomesteadPalette.h"

// The shared ornate frame's geometry (the double rule, the corner flourish and the paper grain) as plain
// rectangles in the surface's own logical units, so the Slate SHomesteadFrame and the Canvas HUD's
// plates draw the same frame. Nothing here reads the engine, and the cost is fixed: the grain is a small
// deterministic speck count, never a function of what the panel holds.
namespace HomesteadFrameStyle
{
// Outer rule, the inner rule's inset and width, and the corner flourish (at most 8 units at 1080p).
constexpr float OuterWidth = 2.0f;
constexpr float InnerInset = 5.0f, InnerWidth = 1.0f;
constexpr float FlourishArm = 8.0f, FlourishWidth = 2.0f, FlourishDot = 3.0f;
// Paper grain: at most this many 1-unit specks, at most this opaque (6% at 1080p).
constexpr int32 MaxGrainSpecks = 56;
constexpr float GrainAreaPerSpeck = 2600.0f;
constexpr float GrainOpacity = 0.06f;

enum class EPart : uint8 { Outer, Inner, Flourish, Grain };

// Calls Fn(Part, X, Y, Width, Height) for every rectangle of a frame over (0,0)-(W,H).
template <typename FnType>
void ForEachRect(float W, float H, FnType&& Fn)
{
    if (W < 4 * InnerInset || H < 4 * InnerInset) return;
    Fn(EPart::Outer, 0.0f, 0.0f, W, OuterWidth);
    Fn(EPart::Outer, 0.0f, H - OuterWidth, W, OuterWidth);
    Fn(EPart::Outer, 0.0f, OuterWidth, OuterWidth, H - 2 * OuterWidth);
    Fn(EPart::Outer, W - OuterWidth, OuterWidth, OuterWidth, H - 2 * OuterWidth);

    const float L = InnerInset, T = InnerInset, R = W - InnerInset, B = H - InnerInset;
    Fn(EPart::Inner, L, T, R - L, InnerWidth);
    Fn(EPart::Inner, L, B - InnerWidth, R - L, InnerWidth);
    Fn(EPart::Inner, L, T, InnerWidth, B - T);
    Fn(EPart::Inner, R - InnerWidth, T, InnerWidth, B - T);

    // Each corner: two arms along the inner rule and a dot just inside the bracket.
    const float Gap = 2.0f;
    for (int32 Corner = 0; Corner < 4; ++Corner)
    {
        const bool bRight = Corner & 1, bBottom = Corner & 2;
        const float CX = bRight ? R : L, CY = bBottom ? B : T;
        const float DirX = bRight ? -1.0f : 1.0f, DirY = bBottom ? -1.0f : 1.0f;
        const auto Span = [](float Origin, float Dir, float Length) { return Dir > 0 ? Origin : Origin - Length; };
        Fn(EPart::Flourish, Span(CX, DirX, FlourishArm), Span(CY, DirY, FlourishWidth), FlourishArm, FlourishWidth);
        Fn(EPart::Flourish, Span(CX, DirX, FlourishWidth), Span(CY, DirY, FlourishArm), FlourishWidth, FlourishArm);
        Fn(EPart::Flourish, Span(CX + DirX * (FlourishWidth + Gap), DirX, FlourishDot),
            Span(CY + DirY * (FlourishWidth + Gap), DirY, FlourishDot), FlourishDot, FlourishDot);
    }

    // The grain: a fixed scatter inside the inner rule, the same for a given size every time.
    const int32 Specks = FMath::Clamp(static_cast<int32>(W * H / GrainAreaPerSpeck), 4, MaxGrainSpecks);
    uint32 Seed = 2166136261u ^ static_cast<uint32>(W * 7.0f) ^ (static_cast<uint32>(H * 13.0f) << 11);
    const auto Next = [&Seed]() { Seed = Seed * 1664525u + 1013904223u; return static_cast<float>(Seed >> 8) / 16777216.0f; };
    const float InnerW = R - L - 4.0f, InnerH = B - T - 4.0f;
    for (int32 Index = 0; Index < Specks; ++Index)
    {
        const float X = L + 2.0f + Next() * InnerW, Y = T + 2.0f + Next() * InnerH;
        Fn(EPart::Grain, X, Y, 1.0f, 1.0f);
    }
}

// The colour of a part now (the theme may change while she plays).
inline FLinearColor ColorOf(EPart Part)
{
    switch (Part)
    {
    case EPart::Outer: return HomesteadPalette::FrameOuter;
    case EPart::Inner: return HomesteadPalette::FrameInner;
    case EPart::Flourish: return HomesteadPalette::FrameFlourish;
    default: return HomesteadPalette::FrameGrain.CopyWithNewOpacity(GrainOpacity);
    }
}
}
