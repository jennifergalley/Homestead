#pragma once

#include "Math/Color.h"

// The parchment notice shared by the field book's NoticeCard (UI/SHomesteadMenu) and the HUD's
// top-centre world notices (AHomesteadHUD: the focus actions and the toast): aged paper in a
// double-ruled frame, iron-gall brown ink, and rust ink for things that went wrong. Sizes are in the
// surface's own logical units.
namespace HomesteadNoticeStyle
{
inline constexpr FLinearColor Paper(0.62f, 0.54f, 0.38f, 0.97f);
inline constexpr FLinearColor InkBrown(0.03f, 0.022f, 0.014f, 1.0f);
inline constexpr FLinearColor RustInk(0.22f, 0.03f, 0.015f, 1.0f);
// The outer frame, and the inner rule (ink at RuleOpacity) that makes it a double frame.
inline constexpr FLinearColor Frame(0.2f, 0.12f, 0.05f, 1.0f);
inline constexpr float RuleOpacity = 0.45f;
inline constexpr float FrameWidth = 2.0f, RuleInset = 5.0f, RuleWidth = 1.0f;
inline constexpr FLinearColor Shadow(0.0f, 0.0f, 0.0f, 0.4f);
}
