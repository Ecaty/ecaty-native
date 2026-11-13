#include "Application.hpp"
#include <cmath>

namespace flecs_unity {

namespace {

void register_spin_components(flecs::world& world) {
    world.component<game::Spin>("Spin")
        .member(&game::Spin::x, "x")
        .member(&game::Spin::y, "y")
        .member(&game::Spin::z, "z");
}

// Systems live in modules (see FlecsComponents.hpp); components stay at the root.
struct SpinModule {
    explicit SpinModule(flecs::world& world) {
        world.module<SpinModule>("engine::spin");
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
};

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

// Continuous gallery motion runs as compiled systems (https://github.com/Ecaty/flecs-native/blob/main/docs/FLECS-ARCHITECTURE-AUDIT.md):
// Script/C# write intents and activity; systems integrate them against Clock every tick, and
// observers apply a write immediately so same-frame reads match the former reactive Script.
void register_gallery_components(flecs::world& world) {
    using namespace game;
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
}

struct GalleryModule {
    explicit GalleryModule(flecs::world& world) {
        world.module<GalleryModule>("engine::gallery");
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
};

} // namespace

void register_application(flecs::world& world) {
    // Components at the root first, then the modules (import order = system order within a phase).
    register_spin_components(world);
    register_gallery_components(world);
    world.import<SpinModule>();
    world.import<GalleryModule>();
}

} // namespace flecs_unity
