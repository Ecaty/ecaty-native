# Ecaty native application

Application-specific C++17 Flecs components and systems. Initially this provides the reflected Spin component and SpinSystem. The shared runtime owns world lifecycle; the app implements `flecs_unity::register_application(flecs::world&)`.

Module `app` holds these systems and observers; the component names stay at the root scope.

- `SpinSystem` turns entities that have `Spin`.
- `GalleryTreeMotionSystem` and `GalleryRingMotionSystem` (PreUpdate) integrate the tree clock and the ring turns against `Clock`; `GalleryTreeActivityObserver` and `GalleryRingIntentObserver` apply a write at once.
- `GalleryRingAssignSetObserver` and `GalleryRingAssignRemoveObserver` place every active `GalleryEntry` on a ring when an entry is set or removed. Entries are sorted by `order`, then by entity name (bytes). Ring 1 holds 6 worlds, ring 2 holds 12, ring n holds 6 times n. Each world gets `GalleryPlacement {layer, slot}` and the exclusive pair `(GalleryOnLayer, GalleryTreeLayer<n>)`. A world whose entry is inactive or removed loses the pair and keeps its last placement. Prefabs are ignored. Nothing runs per frame.

Build from the parent Ecaty project's `Native` directory alongside `Native/Core`; both modules form one `flecs_unity.dll`.

Continue from [Ecaty AGENTS](https://github.com/Ecaty/Ecaty/blob/main/AGENTS.md), [PROJECT](https://github.com/Ecaty/Ecaty/blob/main/docs/PROJECT.md), [DEVELOPMENT](https://github.com/Ecaty/Ecaty/blob/main/docs/DEVELOPMENT.md) and [STATUS](https://github.com/Ecaty/Ecaty/blob/main/docs/STATUS.md).
