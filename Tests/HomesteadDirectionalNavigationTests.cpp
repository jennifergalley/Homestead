#include "../Source/SurvivalGame/UI/HomesteadMenuNavigation.h"
#include <iostream>
#include <limits>

int main()
{
    using namespace HomesteadMenuNavigation;
    int checks = 0;
    auto Check = [&](bool value, const char* name)
    {
        ++checks;
        if (!value) { std::cerr << "FAIL " << name << '\n'; return false; }
        return true;
    };
    if (!Check(Move(0, 0, 4, {0, 1}, 0).boundary, "empty grid exposes boundary")
        || !Check(Move(3, 6, 4, {0, 1}, 3).index == 5, "short row clamps")
        || !Check(Move(5, 6, 4, {0, -1}, 3).index == 3, "desired column returns")
        || !Check(Move(5, 6, 4, {0, 1}, 3).boundary, "final row exits")
        || !Check(Move(0, 6, 4, {-1, 0}, 0).boundary, "left edge exits")
        || !Check(Move(0, 24, 4, {0, 1}, 0).index == 4, "logical offscreen row stays internal")) return 1;
    for (int count = 1; count <= 120; ++count)
        for (int index = 0; index < count; ++index)
            for (const Direction direction : {Direction{-1, 0}, {1, 0}, {0, -1}, {0, 1}})
            {
                const auto result = Move(index, count, 4, direction, index % 4);
                if (!Check(result.index >= 0 && result.index < count, "bounded logical index")
                    || !Check(result.boundary == (Step(index, count, 4, direction.x, direction.y) == index), "real logical boundary")) return 1;
            }
    StickNavigation stick;
    stick.Sample(false, 0.1f, 1);
    if (!Check(!stick.Poll(1).Any(), "stick noise ignored")) return 1;
    stick.Sample(false, 0.9f, 2);
    if (!Check(stick.Poll(2).y == 1, "initial downward stick gesture")
        || !Check(!stick.Poll(2.1).Any(), "no premature repeat")) return 1;
    stick.Sample(false, 0.9f, 2.28);
    if (!Check(stick.Poll(2.28).y == 1, "held stick repeat")) return 1;
    stick.Sample(false, -0.9f, 2.3);
    if (!Check(stick.Poll(2.3).y == -1, "reversal immediate")) return 1;
    stick.Sample(false, 0, 2.31);
    if (!Check(!stick.Poll(2.31).Any(), "neutral stops")) return 1;
    stick.Sample(true, 0.9f, 3);
    if (!Check(stick.Poll(3).x == 1, "horizontal stick")
        || !Check(!stick.Poll(3.4).Any(), "stale/disconnected stick stops")) return 1;
    stick.Sample(true, 0.8f, 4); stick.Sample(false, 0.9f, 4);
    if (!Check(stick.Poll(4).y == 1, "one dominant diagonal direction")) return 1;
    stick.Sample(true, std::numeric_limits<float>::quiet_NaN(), 5);
    if (!Check(!stick.Poll(5).Any(), "invalid analog sample rejected")) return 1;
    std::cout << "PASS " << checks << " directional navigation checks\n";
    return 0;
}
