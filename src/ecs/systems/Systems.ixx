/*
* Copyright (c) 2025-present Henri Michelon
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
*/
export module lysa.ecs.systems;

import std;
import lysa.types;
import lysa.resources.mesh;
import lysa.resources.scene_context;
export import lysa.ecs.components;
export import lysa.ecs.flecs;

export namespace lysa::ecs {

    class TransformModule {
    public:
        TransformModule(const flecs::world& w);
        static void updateGlobalTransform(const flecs::entity& e, Transform& t);
    };

    class MeshInstanceModule {
    public:
        MeshInstanceModule(const flecs::world& w);
    private:
        MeshManager& meshManager;
        SceneContextManager& sceneContextManager;
        void addInstance(
            const flecs::entity& e,
            const Scene& sc,
            const Transform& tr) const;
    };

    struct RenderModule {
        RenderModule(const flecs::world& w);
    };

    class Modules {
    public:
        Modules(flecs::world& w);
        ~Modules();
    private:
        flecs::entity renderModule;
        flecs::entity meshInstanceModule;
        flecs::entity transformModule;
    };

}
