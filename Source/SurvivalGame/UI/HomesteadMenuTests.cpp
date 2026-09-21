#include "HomesteadMenuNavigation.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHomesteadMenuNavigationTest, "Homestead.UI.GridNavigation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FHomesteadMenuNavigationTest::RunTest(const FString& Parameters)
{
    using namespace HomesteadMenuNavigation;
    TestEqual(TEXT("Empty grid has no fake selection"), Step(0, 0, 6, 1, 0), -1);
    TestEqual(TEXT("Right edge cannot wrap into another row"), Step(5, 14, 6, 1, 0), 5);
    TestEqual(TEXT("Left edge cannot wrap into prior row"), Step(6, 14, 6, -1, 0), 6);
    TestEqual(TEXT("Partial final row selects nearest existing column"), Step(10, 14, 6, 0, 1), 13);
    TestEqual(TEXT("Bottom edge stays selected"), Step(13, 14, 6, 0, 1), 13);
    TestEqual(TEXT("Top edge stays selected"), Step(2, 14, 6, 0, -1), 2);
    TestEqual(TEXT("Removed tail selection clamps"), Step(20, 3, 6, 0, 1), 2);
    TestEqual(TEXT("Previous tab wraps"), Cycle(0, 7, -1), 6);
    TestEqual(TEXT("Next tab wraps"), Cycle(6, 7, 1), 0);
    for (int Count = 1; Count <= 120; ++Count)
        for (int Index = 0; Index < Count; ++Index)
            for (int Direction : {-1, 1})
            {
                const int Horizontal = Step(Index, Count, 6, Direction, 0);
                const int Vertical = Step(Index, Count, 6, 0, Direction);
                TestTrue(TEXT("Horizontal result exists"), Horizontal >= 0 && Horizontal < Count);
                TestTrue(TEXT("Vertical result exists"), Vertical >= 0 && Vertical < Count);
            }
    return true;
}
#endif
