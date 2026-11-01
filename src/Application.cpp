#include "Application.hpp"
#include <cmath>

namespace flecs_unity {

void register_application(flecs::world& world) {
    world.component<ecaty::Spin>("Spin")
        .member("x", &ecaty::Spin::x)
        .member("y", &ecaty::Spin::y)
        .member("z", &ecaty::Spin::z);

    using ecaty::Spin;
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

} // namespace flecs_unity
