#pragma once
#include "FlecsComponents.hpp"
#include <cstdint>

namespace game {
struct Spin { float x, y, z; };

// Gallery motion contracts (formerly declared in WorldGalleryTree.flecs). Layouts are mirrored by
// Script initializers and C# typed reads; keep member order stable.
struct GalleryTreeActivity { float rate; bool draft; std::int32_t sequence; float elapsed, started; };
struct GalleryTreeMotion { float time; };
struct GalleryLayerIntent { float angle, speed, started, glowPhase, glowRate; };
struct GalleryLayerState { float turn; };

// Ring assignment contracts (formerly Script structs in GalleryCatalog.flecs and WorldGalleryTree.flecs). GalleryEntry is the
// catalog record each gallery world carries; GalleryPlacement is derived from it by the app module (see Application.cpp).
struct GalleryEntry { std::int32_t order, width, depth, kind; bool active; };
struct GalleryPlacement { std::int32_t layer, slot; };
}
