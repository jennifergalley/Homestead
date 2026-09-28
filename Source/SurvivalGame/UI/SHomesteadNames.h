#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

namespace HomesteadMenus
{
// The new-game Names step (after Appearance): first name, family surname and estate name.
// It owns all input while shown. Keyboard: type into the highlighted field; Tab, arrows and Enter
// move between fields; Enter on Begin starts. Controller: the D-pad or stick picks a field, A opens
// an on-screen letter grid (X deletes, Y adds a space, B closes it), Start begins and B goes back.
class SHomesteadNames : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_ThreeParams(FOnBegin, const FString&, const FString&, const FString&);
    SLATE_BEGIN_ARGS(SHomesteadNames) {}
        SLATE_ARGUMENT(FString, Heroine)
        SLATE_ARGUMENT(FString, Family)
        SLATE_ARGUMENT(FString, Estate)
        SLATE_EVENT(FOnBegin, OnBegin)
        SLATE_EVENT(FSimpleDelegate, OnBack)
    SLATE_END_ARGS()

    static constexpr int32 FieldCount = 3;
    static constexpr int32 BeginRow = 3;
    static constexpr int32 BackRow = 4;

    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply OnKeyChar(const FGeometry& Geometry, const FCharacterEvent& Event) override;
    // For the controller's own input routing and automation: returns true when handled.
    bool HandleKey(const FKey& Key);
    bool TypeCharacter(TCHAR Character);

    FString Value(int32 Field) const { return Values[Field]; }
    void SetValue(int32 Field, const FString& Text);
    int32 SelectedRow() const { return Row; }
    bool IsGridOpen() const { return bGrid; }
    // Why the entries can't be used yet, or empty when Begin will accept them.
    FString Problem() const;

private:
    FString Values[FieldCount];
    int32 Row = 0;
    bool bGrid = false;
    bool bShift = true;
    int32 GridX = 0, GridY = 0;
    FString Warning;
    FOnBegin OnBegin;
    FSimpleDelegate OnBack;
    static const TCHAR* const GridRows[];
    static constexpr int32 GridHeight = 4;
    TSharedRef<SWidget> FieldWidget(int32 Field, const FText& Label);
    TSharedRef<SWidget> GridWidget();
    FString GridCell(int32 X, int32 Y) const;
    int32 GridWidth(int32 Y) const;
    void PressGridCell();
    void Delete();
    void Move(int32 Delta);
    void TryBegin();
};
}
