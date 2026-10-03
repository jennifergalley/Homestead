#pragma once

// Pure navigation rules shared by the native widget and non-engine tests.
namespace HomesteadMenuNavigation
{
struct Direction
{
    int x = 0;
    int y = 0;
    constexpr bool Any() const { return x != 0 || y != 0; }
};
struct MoveResult
{
    int index = -1;
    bool boundary = false;
};
constexpr bool CanConfirmInventoryDrag(bool dragging, bool movableSubject, bool emptySlot)
{
    return movableSubject || (dragging && emptySlot);
}
constexpr int Step(int index, int count, int columns, int dx, int dy)
{
    if (count <= 0 || columns <= 0) return -1;
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;
    const int row = index / columns;
    const int column = index % columns;
    if (dx)
    {
        const int next = index + dx;
        return next >= 0 && next < count && next / columns == row ? next : index;
    }
    const int targetRow = row + dy;
    if (targetRow < 0 || targetRow > (count - 1) / columns) return index;
    const int next = targetRow * columns + column;
    return next < count ? next : count - 1;
}
constexpr int Cycle(int index, int count, int direction)
{
    return count > 0 ? (index + count + direction) % count : 0;
}

constexpr MoveResult Move(int index, int count, int columns, Direction direction, int desiredColumn)
{
    if (count <= 0 || columns <= 0) return {-1, direction.Any()};
    const int current = index < 0 ? 0 : index >= count ? count - 1 : index;
    int next = Step(current, count, columns, direction.x, direction.y);
    const bool boundary = direction.Any() && next == current;
    if (!boundary && direction.y != 0)
    {
        const int column = desiredColumn < 0 ? 0 : desiredColumn >= columns ? columns - 1 : desiredColumn;
        next = (next / columns) * columns + column;
        if (next >= count) next = count - 1;
    }
    return {next, boundary};
}

class StickNavigation
{
public:
    void Sample(bool horizontal, float value, double now)
    {
        if (!(value >= -1.0f && value <= 1.0f)) { Reset(); return; }
        if (horizontal) x_ = value; else y_ = value;
        sampledAt_ = now;
    }
    void Reset() { x_ = y_ = 0; last_ = {}; nextAt_ = 0; sampledAt_ = -1; }
    Direction Poll(double now)
    {
        if (sampledAt_ < 0 || now - sampledAt_ > 0.25) { Reset(); return {}; }
        const float ax = x_ < 0 ? -x_ : x_, ay = y_ < 0 ? -y_ : y_;
        if (!(ax >= 0.55f || ay >= 0.55f)) { last_ = {}; nextAt_ = 0; return {}; }
        const bool horizontal = ax > ay || (ax == ay && last_.x != 0);
        const Direction current = horizontal ? Direction{x_ > 0 ? 1 : -1, 0} : Direction{0, y_ > 0 ? 1 : -1};
        if (current.x != last_.x || current.y != last_.y)
        {
            last_ = current; nextAt_ = now + 0.28; return current;
        }
        // Absolute-time addition can differ from the same sampled deadline by one ULP.
        if (now < nextAt_ && nextAt_ - now > 0.000001) return {};
        nextAt_ = now + 0.18;
        return current;
    }
private:
    float x_ = 0, y_ = 0;
    double sampledAt_ = -1, nextAt_ = 0;
    Direction last_;
};
}
