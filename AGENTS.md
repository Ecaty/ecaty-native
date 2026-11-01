# Application module instructions

Read README.md and Ecaty's canonical [PROJECT](https://github.com/Ecaty/Ecaty/blob/main/docs/PROJECT.md), [DEVELOPMENT](https://github.com/Ecaty/Ecaty/blob/main/docs/DEVELOPMENT.md) and [STATUS](https://github.com/Ecaty/Ecaty/blob/main/docs/STATUS.md). In the composed checkout they are at `../../docs/`.

Put app-specific behavior here as Flecs C++ components and systems. Register through the shared runtime's application hook. Reuse shared component types; do not define duplicate Position/Rotation types or create a second Flecs world. Native algorithms compile; data/relationships/Script remain editable at runtime. Keep Unity presentation out of this module. Verify changes through Ecaty's native and relevant Unity checks. Do not implement speculative physics or gameplay without the current task authorizing it.
