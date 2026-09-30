#include "../Source/SurvivalGame/HomesteadMusicPlaylist.h"
#include <cstdlib>
#include <iostream>
#include <set>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
}

int main()
{
    using Homestead::MusicShuffleBag;
    MusicShuffleBag bag;

    bag.Reset(0, -1, 1);
    Check(bag.Next() == -1 && bag.Next() == -1, "Empty catalog must report no track");

    bag.Reset(1, 0, 2);
    for (int i = 0; i < 5; ++i) Check(bag.Next() == 0, "One-track catalog must keep playing its only track");

    for (int count : {2, 3, 5, 8})
        for (std::uint64_t seed = 1; seed <= 400; ++seed)
        {
            const int avoid = static_cast<int>(seed % static_cast<std::uint64_t>(count));
            bag.Reset(count, avoid, seed);
            int previous = avoid;
            for (int round = 0; round < 6; ++round)
            {
                std::set<int> played;
                for (int i = 0; i < count; ++i)
                {
                    const int track = bag.Next();
                    Check(track >= 0 && track < count, "Track index out of range");
                    Check(played.insert(track).second, "A track repeated before the bag was finished");
                    if (i == 0) Check(track != previous, "A bag started with the track that just played");
                    previous = track;
                }
                Check(static_cast<int>(played.size()) == count, "A bag did not play every track");
            }
        }

    // Different seeds must not force the same order.
    std::set<int> firsts;
    for (std::uint64_t seed = 1; seed <= 64; ++seed)
    {
        bag.Reset(5, -1, seed);
        firsts.insert(bag.Next());
    }
    Check(firsts.size() >= 4, "Seeds do not vary the first track");

    // The same seed reproduces the same order (tests can inject entropy).
    bag.Reset(5, 2, 77);
    int a[10];
    for (int& track : a) track = bag.Next();
    bag.Reset(5, 2, 77);
    for (int track : a) Check(bag.Next() == track, "A fixed seed must reproduce the order");

    // Pacing: the first piece 45-120 s in, then 3-8 minutes of ambience alone after each; draws outside
    // [0, 1] are clamped, and the whole range is reachable.
    namespace Pacing = Homestead::MusicPacing;
    Check(Pacing::FirstDelay(0.0f) == 45.0f && Pacing::FirstDelay(1.0f) == 120.0f, "First delay must span 45-120 s");
    Check(Pacing::Gap(0.0f) == 180.0f && Pacing::Gap(1.0f) == 480.0f, "Gap must span 3-8 minutes");
    Check(Pacing::FirstDelay(-1.0f) == 45.0f && Pacing::Gap(7.0f) == 480.0f, "Out-of-range draws must clamp");
    for (int step = 0; step <= 100; ++step)
    {
        const float unit = static_cast<float>(step) / 100.0f;
        const float first = Pacing::FirstDelay(unit), gap = Pacing::Gap(unit);
        Check(first >= 45.0f && first <= 120.0f && gap >= 180.0f && gap <= 480.0f, "Pacing out of range");
        Check(step == 0 || gap > Pacing::Gap(static_cast<float>(step - 1) / 100.0f), "Gap must grow with the draw");
    }

    std::cout << "HomesteadMusicPlaylistTests passed\n";
    return 0;
}
