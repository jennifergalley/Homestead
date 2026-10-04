#pragma once

#include "Math/Color.h"
#include "HomesteadUITheme.h"

// The parchment notice shared by the field book's NoticeCard (UI/SHomesteadMenu) and the HUD's
// top-centre world notices (AHomesteadHUD: the focus actions and the toast): aged paper in a
// double-ruled frame, iron-gall brown ink, and rust ink for things that went wrong; in the dark book,
// umber paper, cream ink and terracotta (Card*() below). Sizes are in the surface's own logical units.
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

// The same card in the dark (candlelit) book: umber paper, a muted gilt frame, cream ink and terracotta
// for things that went wrong (apply-dark-theme-everywhere; contrast in pitch-dark-parchment-theme/design.md).
// Values are linear.
namespace Dark
{
inline constexpr FLinearColor Paper(0.0423f, 0.0252f, 0.0152f, 0.97f);   // #3A2C21
inline constexpr FLinearColor Ink(0.8632f, 0.7605f, 0.5647f, 1.0f);      // #EFE2C6 cream
inline constexpr FLinearColor MutedInk(0.5776f, 0.4678f, 0.3050f, 1.0f); // #C8B696
inline constexpr FLinearColor Rust(0.8070f, 0.2789f, 0.1590f, 1.0f);     // #E8906F terracotta
inline constexpr FLinearColor Frame(0.3325f, 0.1946f, 0.0595f, 1.0f);    // #9C7A45 dim gilt
inline constexpr FLinearColor Gilt(0.6584f, 0.4020f, 0.1221f, 1.0f);     // #D4AA62
inline constexpr FLinearColor OnGilt(0.0176f, 0.0097f, 0.0048f, 1.0f);   // #24190F
}
// The light card's secondary ink (the focus card's title): iron-gall at 4.5:1 or better on its paper.
inline constexpr FLinearColor MutedInkBrown(0.10f, 0.07f, 0.04f, 1.0f);

// The colours a notice is drawn in now (the palette may change while she plays).
inline FLinearColor CardPaper() { return HomesteadUITheme::IsDark() ? Dark::Paper : Paper; }
inline FLinearColor CardInk() { return HomesteadUITheme::IsDark() ? Dark::Ink : InkBrown; }
inline FLinearColor CardMutedInk() { return HomesteadUITheme::IsDark() ? Dark::MutedInk : MutedInkBrown; }
inline FLinearColor CardRust() { return HomesteadUITheme::IsDark() ? Dark::Rust : RustInk; }
inline FLinearColor CardFrame() { return HomesteadUITheme::IsDark() ? Dark::Frame : Frame; }
inline FLinearColor CardRule() { return (HomesteadUITheme::IsDark() ? Dark::Gilt : InkBrown).CopyWithNewOpacity(RuleOpacity); }
// A key or pad glyph's stamp and its letter: pine and brass on the light card, gilt and umber on the dark.
inline FLinearColor KeyStamp() { return HomesteadUITheme::IsDark() ? Dark::Gilt : FLinearColor(0.055f, 0.09f, 0.075f, 0.96f); }
inline FLinearColor KeyLetter() { return HomesteadUITheme::IsDark() ? Dark::OnGilt : FLinearColor(0.92f, 0.74f, 0.43f, 1.0f); }

// Every notice surface the game draws. A new one is added here (append before Count) and drawn
// through it; the UI gallery (HomesteadUIGallery.h) fails its capture run until it has an entry.
enum class ESurface : uint8
{
    WorldNotice,      // the HUD's top-centre toast
    WorldNoticeError, // the same, rust-framed
    BookNotice,       // the field book's notice card
    BookNoticeError,
    FocusCard,        // the focus title and action hints
    PickupLine,       // "+3 Berries" in the common HUD notice stack
    ControlsStrip,    // the first-minute controls reminder
    ShopStatus,       // the shop's status line (a refusal or a confirm)
    Count
};
inline constexpr bool IsError(ESurface Surface)
{ return Surface == ESurface::WorldNoticeError || Surface == ESurface::BookNoticeError; }
}
