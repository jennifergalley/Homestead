#pragma once

#include "HomesteadSimulation.h"

#include <iosfwd>
#include <vector>

// Estate parcels: pieces of land made from the layout's landmark polygons at a new game. The home
// estate (Anchor::EstateBoundary) is owned; every ForSale.* polygon is for sale and unowned. Saves
// store only ownership by parcel id, so the polygons can move whenever the level is re-authored.
namespace Homestead
{
struct EstateLayout;

constexpr const char* OutsideEstateMessage = "Outside your estate";
constexpr int MaxParcels = 256;

// Parcels for a new estate game; polygons with fewer than three points are skipped.
std::vector<Parcel> ParcelsFromLayout(const EstateLayout& layout);
// Replaces `state.parcels` with the layout's, keeping the ownership of parcels it already had.
void SeedEstateParcels(State& state, const EstateLayout& layout);

// The first parcel containing the point, or null.
const Parcel* FindParcelAt(const std::vector<Parcel>& parcels, Point point);
// Whether some owned parcel contains the point.
bool InOwnedParcel(const std::vector<Parcel>& parcels, Point point);
// The four corners of a turned rectangle.
void FootprintCorners(const Footprint& area, Point (&corners)[4]);
// Whether every corner lies in an owned parcel (each corner may use a different one).
bool FootprintOwned(const std::vector<Parcel>& parcels, const Footprint& area);

// The optional trailing "parcels" save section: `parcels N` then N lines of `id owned`.
void WriteParcelOwnership(std::ostream& out, const State& state);
// Reads the section when it comes next (leaving the stream untouched otherwise) and applies the
// saved ownership to `state.parcels` by id; ids the layout no longer has are ignored.
bool ReadParcelOwnership(std::istream& in, State& state);
}
