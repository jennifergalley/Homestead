#pragma once

// Pure navigation rules shared by the native widget and non-engine tests.
namespace HomesteadMenuNavigation
{
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
}
