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

// The grain: a fixed scatter inside the inner rule (L,T)-(R,B), the same for a given size every time.
template <typename FnType>
void ForEachGrain(float W, float H, float L, float T, float R, float B, FnType&& Fn)
{
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

    ForEachGrain(W, H, L, T, R, B, Fn);
}

// The three showpiece panels (her pack, her portrait, the calendar) wear a heavier frame: a thicker outer
// rule, a gilt stud on each outer corner, long corner brackets with a dot tucked inside, and a short
// bar at the middle of each long side. Same part kinds and colours as the standard frame.
constexpr float GrandOuterWidth = 3.0f;
constexpr float GrandInnerInset = 6.0f, GrandInnerWidth = 1.5f;
constexpr float GrandStud = 7.0f, GrandArm = 16.0f, GrandArmWidth = 3.0f, GrandDot = 4.0f, GrandDotInset = 12.0f;
constexpr float GrandTick = 22.0f, GrandTickMinEdge = 90.0f;

template <typename FnType>
void ForEachGrandRect(float W, float H, FnType&& Fn)
{
    if (W < 4 * GrandArm || H < 4 * GrandArm) return;
    Fn(EPart::Outer, 0.0f, 0.0f, W, GrandOuterWidth);
    Fn(EPart::Outer, 0.0f, H - GrandOuterWidth, W, GrandOuterWidth);
    Fn(EPart::Outer, 0.0f, GrandOuterWidth, GrandOuterWidth, H - 2 * GrandOuterWidth);
    Fn(EPart::Outer, W - GrandOuterWidth, GrandOuterWidth, GrandOuterWidth, H - 2 * GrandOuterWidth);

    const float L = GrandInnerInset, T = GrandInnerInset, R = W - GrandInnerInset, B = H - GrandInnerInset;
    Fn(EPart::Inner, L, T, R - L, GrandInnerWidth);
    Fn(EPart::Inner, L, B - GrandInnerWidth, R - L, GrandInnerWidth);
    Fn(EPart::Inner, L, T, GrandInnerWidth, B - T);
    Fn(EPart::Inner, R - GrandInnerWidth, T, GrandInnerWidth, B - T);

    const auto Span = [](bool bFar, float Extent, float Length, float Inset) { return bFar ? Extent - Inset - Length : Inset; };
    for (int32 Corner = 0; Corner < 4; ++Corner)
    {
        const bool bRight = Corner & 1, bBottom = Corner & 2;
        Fn(EPart::Flourish, Span(bRight, W, GrandStud, 0.0f), Span(bBottom, H, GrandStud, 0.0f), GrandStud, GrandStud);
        Fn(EPart::Flourish, Span(bRight, W, GrandArm, GrandInnerInset), Span(bBottom, H, GrandArmWidth, GrandInnerInset), GrandArm, GrandArmWidth);
        Fn(EPart::Flourish, Span(bRight, W, GrandArmWidth, GrandInnerInset), Span(bBottom, H, GrandArm, GrandInnerInset), GrandArmWidth, GrandArm);
        Fn(EPart::Flourish, Span(bRight, W, GrandDot, GrandDotInset), Span(bBottom, H, GrandDot, GrandDotInset), GrandDot, GrandDot);
    }
    if (W >= GrandTickMinEdge)
    {
        Fn(EPart::Flourish, (W - GrandTick) * 0.5f, GrandInnerInset, GrandTick, GrandArmWidth);
        Fn(EPart::Flourish, (W - GrandTick) * 0.5f, H - GrandInnerInset - GrandArmWidth, GrandTick, GrandArmWidth);
    }
    if (H >= GrandTickMinEdge)
    {
        Fn(EPart::Flourish, GrandInnerInset, (H - GrandTick) * 0.5f, GrandArmWidth, GrandTick);
        Fn(EPart::Flourish, W - GrandInnerInset - GrandArmWidth, (H - GrandTick) * 0.5f, GrandArmWidth, GrandTick);
    }
    ForEachGrain(W, H, L, T, R, B, Fn);
}

// The field book's own rectangles (tabs, pack and hotbar cells, equipment slots, craft rows) carry a small
// border of their own instead of one frame around the whole book: a hairline rule, a fainter inner rule
// and a bracket at each corner with a dot tucked inside it. Fixed rectangle count per button.
constexpr float CellRuleWidth = 1.5f;
constexpr float CellInnerInset = 3.5f, CellInnerWidth = 1.0f;
constexpr float CellBracketArm = 7.0f, CellBracketWidth = 2.5f, CellDot = 2.0f, CellDotInset = 5.0f;
// Below these sizes a button only gets the hairline rule (a swatch, a round handle, a tiny icon button).
constexpr float CellMinSize = 14.0f, CellInnerMinSize = 30.0f;

enum class ECellPart : uint8 { Rule, Inner, Bracket };

template <typename FnType>
void ForEachCellRect(float W, float H, FnType&& Fn)
{
    if (W < CellMinSize || H < CellMinSize) return;
    Fn(ECellPart::Rule, 0.0f, 0.0f, W, CellRuleWidth);
    Fn(ECellPart::Rule, 0.0f, H - CellRuleWidth, W, CellRuleWidth);
    Fn(ECellPart::Rule, 0.0f, CellRuleWidth, CellRuleWidth, H - 2 * CellRuleWidth);
    Fn(ECellPart::Rule, W - CellRuleWidth, CellRuleWidth, CellRuleWidth, H - 2 * CellRuleWidth);
    if (W < CellInnerMinSize || H < CellInnerMinSize) return;

    const float L = CellInnerInset, T = CellInnerInset, R = W - CellInnerInset, B = H - CellInnerInset;
    Fn(ECellPart::Inner, L, T, R - L, CellInnerWidth);
    Fn(ECellPart::Inner, L, B - CellInnerWidth, R - L, CellInnerWidth);
    Fn(ECellPart::Inner, L, T + CellInnerWidth, CellInnerWidth, B - T - 2 * CellInnerWidth);
    Fn(ECellPart::Inner, R - CellInnerWidth, T + CellInnerWidth, CellInnerWidth, B - T - 2 * CellInnerWidth);
    for (int32 Corner = 0; Corner < 4; ++Corner)
    {
        const bool bRight = Corner & 1, bBottom = Corner & 2;
        const auto Span = [](bool bFar, float Extent, float Length) { return bFar ? Extent - Length : 0.0f; };
        Fn(ECellPart::Bracket, Span(bRight, W, CellBracketArm), Span(bBottom, H, CellBracketWidth), CellBracketArm, CellBracketWidth);
        Fn(ECellPart::Bracket, Span(bRight, W, CellBracketWidth), Span(bBottom, H, CellBracketArm), CellBracketWidth, CellBracketArm);
        Fn(ECellPart::Bracket, Span(bRight, W, CellDotInset + CellDot) + (bRight ? 0.0f : CellDotInset),
            Span(bBottom, H, CellDotInset + CellDot) + (bBottom ? 0.0f : CellDotInset), CellDot, CellDot);
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

inline FLinearColor CellColorOf(ECellPart Part, bool bHovered)
{
    switch (Part)
    {
    case ECellPart::Rule: return bHovered ? FLinearColor(HomesteadPalette::CellBracket) : FLinearColor(HomesteadPalette::CellRule);
    case ECellPart::Inner: return HomesteadPalette::CellInner;
    default: return HomesteadPalette::CellBracket;
    }
}
}
