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
#ifdef LUA_BINDING
export import lysa.ecs.lua;
#endif

export namespace lysa::ecs {

    struct ecs {
        ecs(lysa::Context& ctx
#ifdef LUA_BINDING
            , const lysa::Lua& lua
#endif
        );

        flecs::world world;
        std::unique_ptr<Modules> modules;
    };

    flecs::entity& load(flecs::entity& root, const std::string &fileURI);
    flecs::entity& load(flecs::entity& root, std::ifstream &stream);

}
