/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
export module lysa.ecs.lua;

import lysa.lua;

export namespace lysa::ecs {

    struct LuaBindings {
        static void _register(const Lua& lua);
    };

}