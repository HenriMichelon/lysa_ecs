/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
export module lysa.ecs.lua;

import lysa.context;

export namespace lysa::ecs {

    class LuaBindings {
    public:
        static void _register(Context& ctx);
    };

}