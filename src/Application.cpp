#include "Application.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace flecs_unity {

namespace {

// Public component names stay at the root scope (Script, JSON and saves use them); systems and observers live in the
// app module. Phases follow the map in Core/src/Modules.hpp: intents in PreUpdate, simulation in OnUpdate.
void brief(flecs::world& world, const char* path, const char* text) {
    if (const ecs_entity_t e = ecs_lookup(world, path)) ecs_doc_set_brief(world, e, text);
}

// Returns the GalleryOnLayer relationship.
flecs::entity register_components(flecs::world& world) {
    using namespace game;
    world.component<Spin>("Spin")
        .member(&Spin::x, "x")
        .member(&Spin::y, "y")
        .member(&Spin::z, "z");
    world.component<GalleryTreeActivity>("GalleryTreeActivity")
        .member(&GalleryTreeActivity::rate, "rate")
        .member(&GalleryTreeActivity::draft, "draft")
        .member(&GalleryTreeActivity::sequence, "sequence")
        .member(&GalleryTreeActivity::elapsed, "elapsed")
        .member(&GalleryTreeActivity::started, "started");
    world.component<GalleryTreeMotion>("GalleryTreeMotion")
        .member(&GalleryTreeMotion::time, "time");
    world.component<GalleryLayerIntent>("GalleryLayerIntent")
        .member(&GalleryLayerIntent::angle, "angle")
        .member(&GalleryLayerIntent::speed, "speed")
        .member(&GalleryLayerIntent::started, "started")
        .member(&GalleryLayerIntent::glowPhase, "glowPhase")
        .member(&GalleryLayerIntent::glowRate, "glowRate");
    world.component<GalleryLayerState>("GalleryLayerState")
        .member(&GalleryLayerState::turn, "turn");
    world.component<GalleryEntry>("GalleryEntry")
        .member(&GalleryEntry::order, "order")
        .member(&GalleryEntry::width, "width")
        .member(&GalleryEntry::depth, "depth")
        .member(&GalleryEntry::kind, "kind")
        .member(&GalleryEntry::active, "active");
    world.component<GalleryPlacement>("GalleryPlacement")
        .member(&GalleryPlacement::layer, "layer")
        .member(&GalleryPlacement::slot, "slot");
    // (GalleryOnLayer, GalleryTreeLayer<n>): the ring a gallery world sits on. A world is on one ring only.
    const flecs::entity relation = world.entity("GalleryOnLayer").add(flecs::Exclusive);

    brief(world, "GalleryEntry", "A world in the gallery catalog. Active entries are placed on rings by their order.");
    brief(world, "GalleryEntry.order", "Position in the gallery: lower comes first. Ties are broken by the entity name.");
    brief(world, "GalleryEntry.width", "Fixture width of the preview; every world uses one gallery cell.");
    brief(world, "GalleryEntry.depth", "Fixture depth of the preview; every world uses one gallery cell.");
    brief(world, "GalleryEntry.kind", "Which preview fixture or creation kind the world shows.");
    brief(world, "GalleryEntry.active", "False hides the world from the gallery; it keeps its record but has no ring.");
    brief(world, "GalleryPlacement", "Where an active gallery world sits, derived from GalleryEntry by the app module. Do not write it.");
    brief(world, "GalleryPlacement.layer", "Ring number from 1. Ring n holds 6 times n worlds.");
    brief(world, "GalleryPlacement.slot", "Position on the ring from 0, in gallery order.");
    brief(world, "GalleryOnLayer", "Relationship to the GalleryTreeLayer<n> entity of the ring the world sits on. Kept by the app module.");
    return relation;
}

void register_spin(flecs::world& world) {
    using game::Spin;
    world.system<Rotation, const Spin>("SpinSystem")
        .term_at(0).self() // Write an entity's own rotation, never its prefab's.
        .each([](flecs::iter& it, std::size_t row, Rotation& rotation, const Spin& speed) {
            const auto dt = static_cast<float>(it.delta_time());
            rotation.x = std::fmod(rotation.x + speed.x * dt, 360.0f);
            rotation.y = std::fmod(rotation.y + speed.y * dt, 360.0f);
            rotation.z = std::fmod(rotation.z + speed.z * dt, 360.0f);
            it.entity(row).modified<Rotation>();
        });
}

double tree_rate(const flecs::world& w) {
    const flecs::entity settings = w.lookup("GalleryTreeSettings");
    const game::GalleryTreeActivity* activity = settings ? settings.try_get<game::GalleryTreeActivity>() : nullptr;
    return activity ? activity->rate : 1.0;
}

void update_layer(flecs::entity e, const game::GalleryLayerIntent& intent, double now, double rate) {
    const float turn = static_cast<float>(intent.angle + (now - intent.started) * intent.speed * rate);
    const game::GalleryLayerState* current = e.try_get<game::GalleryLayerState>();
    if (!current || current->turn != turn) e.set<game::GalleryLayerState>({turn});
}

void update_tree(flecs::entity e, const game::GalleryTreeActivity& activity, double now) {
    const float time = static_cast<float>(activity.elapsed + (now - activity.started) * activity.rate);
    const game::GalleryTreeMotion* current = e.try_get<game::GalleryTreeMotion>();
    if (!current || current->time != time) e.set<game::GalleryTreeMotion>({time});
}

double now(const flecs::world& w) {
    const Clock* clock = w.try_get<Clock>();
    return clock ? clock->time : 0.0;
}

// Continuous gallery motion runs as compiled systems (docs/FLECS-ARCHITECTURE-AUDIT.md):
// Script/C# write intents and activity; systems integrate them against Clock every tick, and
// observers apply a write immediately so same-frame reads match the former reactive Script.
void register_gallery_motion(flecs::world& world) {
    using namespace game;
    // Tree motion clock: elapsed + (now - started) * rate.
    world.system<const GalleryTreeActivity>("GalleryTreeMotionSystem")
        .kind(flecs::PreUpdate)
        .each([](flecs::entity e, const GalleryTreeActivity& activity) { update_tree(e, activity, now(e.world())); });

    // Ring turn: angle + (now - started) * speed * tree rate.
    world.system<const GalleryLayerIntent>("GalleryRingMotionSystem")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter& it) {
            const flecs::world w = it.world();
            const double t = now(w), rate = tree_rate(w);
            while (it.next()) {
                auto intent = it.field<const GalleryLayerIntent>(0);
                for (auto i : it) update_layer(it.entity(i), intent[i], t, rate);
            }
        });

    world.observer<const GalleryLayerIntent>("GalleryRingIntentObserver")
        .event(flecs::OnSet)
        .each([](flecs::entity e, const GalleryLayerIntent& intent) {
            const flecs::world w = e.world();
            update_layer(e, intent, now(w), tree_rate(w));
        });

    world.observer<const GalleryTreeActivity>("GalleryTreeActivityObserver")
        .event(flecs::OnSet)
        .each([](flecs::entity e, const GalleryTreeActivity& activity) {
            const flecs::world w = e.world();
            const double t = now(w);
            update_tree(e, activity, t);
            w.each([&](flecs::entity layer, const GalleryLayerIntent& intent) { update_layer(layer, intent, t, activity.rate); });
        });
}

// The ring of a world is a pure function of the active catalog: sort by (order, entity name), fill ring 1 with 6, ring 2 with 12,
// ring L with 6 * L. It is recomputed when an entry is set or removed, never per frame. Only values that differ are written.
// An entry that is not active, or was removed, loses its ring pair; its last GalleryPlacement stays as it was.
struct RingRow {
    flecs::entity entity;
    std::int32_t order;
    bool active;
};

flecs::entity ring_layer(const flecs::world& w, std::int32_t layer) {
    const std::string name = "GalleryTreeLayer" + std::to_string(layer);
    const ecs_entity_t scope = ecs_set_scope(w, 0);
    flecs::entity found = w.lookup(name.c_str());
    if (!found) found = w.entity(name.c_str());
    ecs_set_scope(w, scope);
    return found;
}

void assign_rings(const flecs::world& w, flecs::entity_t relation, flecs::entity removed) {
    static thread_local bool running = false; // the writes below never touch GalleryEntry, so this is only a safety net
    if (running || ecs_is_fini(w)) return; // a world that is being destroyed removes every entry: nothing to place
    running = true;
    using namespace game;
    std::vector<RingRow> rows;
    w.query_builder<const GalleryEntry>().build().each([&](flecs::entity e, const GalleryEntry& entry) {
        if (e != removed) rows.push_back({e, entry.order, entry.active});
    });
    std::vector<RingRow*> active;
    for (RingRow& row : rows) {
        if (row.active) active.push_back(&row);
        else if (row.entity.has(relation, flecs::Wildcard)) row.entity.remove(relation, flecs::Wildcard);
    }
    if (removed && removed.has(relation, flecs::Wildcard)) removed.remove(relation, flecs::Wildcard);
    std::sort(active.begin(), active.end(), [](const RingRow* a, const RingRow* b) {
        if (a->order != b->order) return a->order < b->order;
        const char* nameA = a->entity.name().c_str();
        const char* nameB = b->entity.name().c_str();
        const std::string_view x = nameA ? nameA : "", y = nameB ? nameB : "";
        const int byName = x.compare(y); // bytes, not culture
        return byName != 0 ? byName < 0 : a->entity.id() < b->entity.id();
    });
    std::int32_t layer = 1, slot = 0;
    flecs::entity layerEntity;
    std::int32_t layerEntityOf = 0;
    for (const RingRow* row : active) {
        if (slot == layer * 6) { ++layer; slot = 0; }
        const GalleryPlacement wanted{layer, slot++};
        const GalleryPlacement* current = row->entity.try_get<GalleryPlacement>();
        if (!current || current->layer != wanted.layer || current->slot != wanted.slot) row->entity.set<GalleryPlacement>(wanted);
        if (layerEntityOf != layer) { layerEntity = ring_layer(w, layer); layerEntityOf = layer; }
        if (!row->entity.has(relation, layerEntity)) row->entity.add(relation, layerEntity);
    }
    running = false;
}

void register_gallery_rings(flecs::world& world, flecs::entity relation) {
    using namespace game;
    const flecs::entity_t rel = relation.id();
    world.observer<const GalleryEntry>("GalleryRingAssignSetObserver")
        .event(flecs::OnSet)
        .each([rel](flecs::entity e, const GalleryEntry&) { assign_rings(e.world(), rel, flecs::entity()); });
    world.observer<const GalleryEntry>("GalleryRingAssignRemoveObserver")
        .event(flecs::OnRemove)
        .each([rel](flecs::entity e, const GalleryEntry&) { assign_rings(e.world(), rel, e); });
}

struct AppModule {
    explicit AppModule(flecs::world& world) {
        const ecs_entity_t scope = ecs_set_scope(world, 0);
        const flecs::entity relation = register_components(world);
        ecs_set_scope(world, scope);
        world.module<AppModule>("::app");
        register_spin(world);
        register_gallery_motion(world);
        register_gallery_rings(world, relation);
    }
};

} // namespace

void register_application(flecs::world& world) { world.import<AppModule>(); }

} // namespace flecs_unity
