# Ecaty native application

Application-specific C++17 Flecs components and systems. Initially this provides the reflected Spin component and SpinSystem. The shared runtime owns world lifecycle; the app implements `flecs_unity::register_application(flecs::world&)`.

Build from the parent Ecaty project's `Native` directory alongside `Native/Core`; both modules form one `flecs_unity.dll`.

Continue from [Ecaty AGENTS](https://github.com/Ecaty/Ecaty/blob/main/AGENTS.md), [PROJECT](https://github.com/Ecaty/Ecaty/blob/main/docs/PROJECT.md), [DEVELOPMENT](https://github.com/Ecaty/Ecaty/blob/main/docs/DEVELOPMENT.md) and [STATUS](https://github.com/Ecaty/Ecaty/blob/main/docs/STATUS.md).
