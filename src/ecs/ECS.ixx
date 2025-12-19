/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
export module lysa.ecs;

import lysa;
export import lysa.ecs.components;
export import lysa.ecs.flecs;
export import lysa.ecs.systems;

export namespace lysa::ecs {

    struct ecs {
        ecs(lysa::Context& ctx):
            world(flecs::world()) {
            world.set<Context>({&ctx});
            modules = std::make_unique<Modules>(world);
            ctx.events.subscribe(MainLoopEvent::PHYSICS_PROCESS, [&] (const Event&){
                if (!world.progress()) {
                    ctx.exit = true;
                }
            });
        }

        flecs::world world;
        std::unique_ptr<Modules> modules;
    };
}
