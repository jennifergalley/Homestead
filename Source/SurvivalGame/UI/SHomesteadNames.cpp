#include "SHomesteadNames.h"
#include "HomesteadUITheme.h"
#include "HomesteadPalette.h"

#include "SHomesteadArrival.h"
#include "../Simulation/HomesteadManor.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
namespace
{
const FLinearColor& NameInk = HomesteadPalette::Cream;
const FLinearColor& NameMuted = HomesteadPalette::Sage;
const FLinearColor& NameGold = HomesteadPalette::Brass;
HomesteadUITheme::FThemeColor NameWarning(0.95f, 0.55f, 0.42f);
HomesteadUITheme::FThemeColor NamePanel(0.025f, 0.05f, 0.038f, 0.9f);
HomesteadUITheme::FThemeColor NameField(0.06f, 0.1f, 0.075f, 0.95f);
HomesteadUITheme::FThemeColor NameFieldSelected(0.13f, 0.19f, 0.14f, 1.0f);

std::string ToUtf8(const FString& Text) { return std::string(TCHAR_TO_UTF8(*Text)); }
}

const TCHAR* const SHomesteadNames::GridRows[] = {TEXT("ABCDEFGHIJ"), TEXT("KLMNOPQRST"), TEXT("UVWXYZ'-.")};

void SHomesteadNames::Construct(const FArguments& Args)
{
    Values[0] = Args._Heroine;
    Values[1] = Args._Family;
    Values[2] = Args._Estate;
    OnBegin = Args._OnBegin;
    OnBack = Args._OnBack;
    const FText Labels[] = {FText::FromString(TEXT("First name")), FText::FromString(TEXT("Family surname")),
        FText::FromString(TEXT("Estate"))};
    TSharedRef<SVerticalBox> Fields = SNew(SVerticalBox);
    for (int32 Field = 0; Field < FieldCount; ++Field)
        Fields->AddSlot().AutoHeight().Padding(0, 0, 0, 14)[FieldWidget(Field, Labels[Field])];
    const auto ButtonColor = [this](int32 Target)
    {
        return TAttribute<FSlateColor>::CreateLambda([this, Target]()
        { return FSlateColor(Row == Target && !bGrid ? NameGold : NameInk); });
    };
    ChildSlot
    .HAlign(HAlign_Center)
    .VAlign(VAlign_Center)
    [
        SNew(SBox)
        .WidthOverride(720)
        [
            SNew(SBorder)
            .BorderImage_Lambda([]() -> const FSlateBrush* { static const FSlateRoundedBoxBrush Panel(NamePanel, 10.0f); return &Panel; })
            .Padding(FMargin(44, 32))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("Who comes home?")))
                    .Font(DisplayFont(40)).ColorAndOpacity(NameInk)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 22)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Name her, her family and the estate. You can keep these or type your own.")))
                    .Font(HomesteadUITheme::Font("Regular", 15)).ColorAndOpacity(NameMuted).AutoWrapText(true)
                ]
                + SVerticalBox::Slot().AutoHeight()[Fields]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SBox)
                    .Visibility_Lambda([this]() { return bGrid ? EVisibility::Visible : EVisibility::Collapsed; })
                    [GridWidget()]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]() { return FText::FromString(Warning); })
                    .Visibility_Lambda([this]() { return Warning.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
                    .Font(HomesteadUITheme::Font("Regular", 15)).ColorAndOpacity(NameWarning)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 0)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        SNew(SButton).IsFocusable(false)
                        .ButtonColorAndOpacity(FLinearColor(0, 0, 0, 0))
                        .OnClicked_Lambda([this]() { Row = BackRow; OnBack.ExecuteIfBound(); return FReply::Handled(); })
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("\u2039 Appearance")))
                            .Font(HomesteadUITheme::Font("Bold", 17)).ColorAndOpacity(ButtonColor(BackRow))
                        ]
                    ]
                    + SHorizontalBox::Slot().FillWidth(1)
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        SNew(SButton).IsFocusable(false)
                        .ButtonColorAndOpacity(FLinearColor(0, 0, 0, 0))
                        .OnClicked_Lambda([this]() { Row = BeginRow; TryBegin(); return FReply::Handled(); })
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("Begin \u203A")))
                            .Font(HomesteadUITheme::Font("Bold", 20)).ColorAndOpacity(ButtonColor(BeginRow))
                        ]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Controller: A edit with letters, Start begin, B back\nKeyboard: type, Tab or Enter for the next field, Esc back")))
                    .Font(HomesteadUITheme::Font("Regular", 14)).ColorAndOpacity(NameMuted).AutoWrapText(true)
                ]
            ]
        ]
    ];
}

TSharedRef<SWidget> SHomesteadNames::FieldWidget(int32 Field, const FText& Label)
{
    return SNew(SButton).IsFocusable(false)
        .ButtonColorAndOpacity(FLinearColor(0, 0, 0, 0))
        .ContentPadding(0)
        .OnClicked_Lambda([this, Field]() { Row = Field; bGrid = false; Warning.Reset(); return FReply::Handled(); })
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(2, 0, 0, 4)
            [
                SNew(STextBlock).Text(Label).Font(HomesteadUITheme::Font("Bold", 14))
                .ColorAndOpacity_Lambda([this, Field]() { return FSlateColor(Row == Field ? NameGold : NameMuted); })
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SBorder)
                .BorderImage_Lambda([this, Field]() -> const FSlateBrush*
                {
                    static const FSlateRoundedBoxBrush Plain(NameField, 6.0f, FLinearColor(0.2f, 0.26f, 0.21f), 1.0f);
                    static const FSlateRoundedBoxBrush Chosen(NameFieldSelected, 6.0f, NameGold, 2.0f);
                    return Row == Field ? &Chosen : &Plain;
                })
                .Padding(FMargin(14, 8))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this, Field]()
                        {
                            const bool bCaret = Row == Field && FMath::Fmod(FSlateApplication::Get().GetCurrentTime(), 1.0) < 0.6;
                            return FText::FromString(Values[Field] + (bCaret ? TEXT("|") : TEXT(" ")));
                        })
                        .Font(DisplayFont(26)).ColorAndOpacity(NameInk)
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this, Field]()
                        { return FText::FromString(FString::Printf(TEXT("%d/%d"), Values[Field].Len(), Homestead::Manor::MaxNameLength)); })
                        .Font(HomesteadUITheme::Font("Regular", 12)).ColorAndOpacity(NameMuted)
                    ]
                ]
            ]
        ];
}

int32 SHomesteadNames::GridWidth(int32 Y) const
{
    return Y < GridHeight - 1 ? FCString::Strlen(GridRows[Y]) : 4;
}

FString SHomesteadNames::GridCell(int32 X, int32 Y) const
{
    if (Y < GridHeight - 1)
    {
        const TCHAR Letter = GridRows[Y][X];
        return FString::Chr(bShift ? Letter : FChar::ToLower(Letter));
    }
    static const TCHAR* Specials[] = {TEXT("Shift"), TEXT("Space"), TEXT("Delete"), TEXT("Done")};
    return Specials[X];
}

TSharedRef<SWidget> SHomesteadNames::GridWidget()
{
    TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);
        for (int32 X = 0; X < GridWidth(Y); ++X)
        {
            Line->AddSlot().AutoWidth().Padding(3)
            [
                SNew(SButton).IsFocusable(false)
                .ButtonColorAndOpacity_Lambda([this, X, Y]()
                { return FLinearColor(GridX == X && GridY == Y ? NameGold * 0.55f : NameField); })
                .OnClicked_Lambda([this, X, Y]() { GridX = X; GridY = Y; PressGridCell(); return FReply::Handled(); })
                [
                    SNew(SBox).WidthOverride(Y < GridHeight - 1 ? 44.0f : 116.0f).HeightOverride(40).HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this, X, Y]() { return FText::FromString(GridCell(X, Y)); })
                        .Font(HomesteadUITheme::Font("Bold", 16))
                        .ColorAndOpacity_Lambda([this, X, Y]() { return FSlateColor(GridX == X && GridY == Y ? FLinearColor::White : NameInk); })
                    ]
                ]
            ];
        }
        Grid->AddSlot().AutoHeight().HAlign(HAlign_Center)[Line];
    }
    return Grid;
}

void SHomesteadNames::SetValue(int32 Field, const FString& Text)
{
    if (Field >= 0 && Field < FieldCount) Values[Field] = Text.Left(Homestead::Manor::MaxNameLength);
}

FString SHomesteadNames::Problem() const
{
    const char* Names[] = {"first name", "surname", "estate name"};
    for (int32 Field = 0; Field < FieldCount; ++Field)
    {
        const std::string Problem = Homestead::Manor::NameProblem(Homestead::Manor::TrimName(ToUtf8(Values[Field])), Names[Field]);
        if (!Problem.empty()) return UTF8_TO_TCHAR(Problem.c_str());
    }
    return FString();
}

bool SHomesteadNames::TypeCharacter(TCHAR Character)
{
    if (Row >= FieldCount || Character < 32 || Character == 127) return false;
    if (Values[Row].Len() >= Homestead::Manor::MaxNameLength) { Warning = TEXT("Names are at most 24 characters."); return true; }
    Values[Row].AppendChar(Character);
    Warning.Reset();
    return true;
}

void SHomesteadNames::Delete()
{
    if (Row < FieldCount && !Values[Row].IsEmpty()) Values[Row].LeftChopInline(1);
    Warning.Reset();
}

void SHomesteadNames::Move(int32 Delta)
{
    Row = (Row + Delta + BackRow + 1) % (BackRow + 1);
    Warning.Reset();
}

void SHomesteadNames::PressGridCell()
{
    if (GridY < GridHeight - 1)
    {
        TypeCharacter(GridCell(GridX, GridY)[0]);
        // Capitals start a word; lowercase follows, the way a name is written.
        bShift = false;
        return;
    }
    switch (GridX)
    {
    case 0: bShift = !bShift; break;
    case 1: TypeCharacter(TEXT(' ')); bShift = true; break;
    case 2: Delete(); break;
    default: bGrid = false; if (Row < FieldCount - 1) ++Row; else Row = BeginRow; break;
    }
}

void SHomesteadNames::TryBegin()
{
    Warning = Problem();
    if (!Warning.IsEmpty())
    {
        // Put her straight back on the field that needs attention.
        for (int32 Field = 0; Field < FieldCount; ++Field)
            if (!Homestead::Manor::NameProblem(Homestead::Manor::TrimName(ToUtf8(Values[Field])), "name").empty())
            {
                Row = Field;
                break;
            }
        return;
    }
    OnBegin.ExecuteIfBound(Values[0], Values[1], Values[2]);
}

bool SHomesteadNames::HandleKey(const FKey& Key, bool bShiftDown)
{
    const bool Up = Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Up;
    const bool Down = Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down || Key == EKeys::Down;
    const bool Left = Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Left;
    const bool Right = Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right || Key == EKeys::Right;
    const bool Accept = Key == EKeys::Gamepad_FaceButton_Bottom;
    const bool Cancel = Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Escape;
    if (Key == EKeys::Gamepad_Special_Right) { bGrid = false; Row = BeginRow; TryBegin(); return true; }
    if (bGrid)
    {
        if (Up || Down)
        {
            GridY = (GridY + (Down ? 1 : -1) + GridHeight) % GridHeight;
            GridX = FMath::Min(GridX, GridWidth(GridY) - 1);
        }
        else if (Left || Right) GridX = (GridX + (Right ? 1 : -1) + GridWidth(GridY)) % GridWidth(GridY);
        else if (Accept || Key == EKeys::Enter) PressGridCell();
        else if (Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::BackSpace) Delete();
        else if (Key == EKeys::Gamepad_FaceButton_Top) TypeCharacter(TEXT(' '));
        else if (Cancel) bGrid = false;
        else return false;
        return true;
    }
    if (Up || (Key == EKeys::Tab && (bShiftDown || FSlateApplication::Get().GetModifierKeys().IsShiftDown()))) Move(-1);
    else if (Down || Key == EKeys::Tab) Move(1);
    else if ((Left || Right) && Row >= BeginRow) Row = Row == BeginRow ? BackRow : BeginRow;
    else if (Key == EKeys::BackSpace) Delete();
    else if (Accept || Key == EKeys::Enter)
    {
        if (Row == BeginRow) TryBegin();
        else if (Row == BackRow) OnBack.ExecuteIfBound();
        else if (Accept) { bGrid = true; bShift = Values[Row].IsEmpty() || Values[Row].EndsWith(TEXT(" ")); GridX = GridY = 0; }
        else Move(1);
    }
    else if (Cancel) OnBack.ExecuteIfBound();
    else return false;
    return true;
}

FReply SHomesteadNames::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    return HandleKey(Event.GetKey(), Event.IsShiftDown()) ? FReply::Handled() : FReply::Unhandled();
}

FReply SHomesteadNames::OnKeyChar(const FGeometry& Geometry, const FCharacterEvent& Event)
{
    const TCHAR Character = Event.GetCharacter();
    if (Character == TEXT('\t') || Character == TEXT('\r') || Character == TEXT('\n') || Character == TEXT('\b'))
        return FReply::Handled();
    return TypeCharacter(Character) ? FReply::Handled() : FReply::Unhandled();
}
}
