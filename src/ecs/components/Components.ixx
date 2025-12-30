/*
* Copyright (c) 2025-present Henri Michelon
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
*/
export module lysa.ecs.components;

import vireo;
import lysa.aabb;
import lysa.context;
import lysa.math;
import lysa.context;
#ifdef LUA_BINDING
import lysa.lua;
#endif
import lysa.renderers.graphic_pipeline_data;

export import lysa.ecs.components.transform;
export import lysa.ecs.flecs;

export namespace lysa::ecs {
    struct Updated {};
    struct Visible {};
    struct CastShadows {};

    struct Context {
        lysa::Context* ctx;
    };

#ifdef LUA_BINDING
    struct Lua {
        const lysa::Lua* lua;
    };
#endif

    struct RenderTarget {
        unique_id renderTarget{INVALID_ID};
    };

    struct Viewport {
        //! Low‑level viewport (x, y, width, height, minDepth, maxDepth).
        vireo::Viewport viewport{};
        //! Scissors rectangle limiting rendering to a sub‑area.
        vireo::Rect scissors{};

        Viewport() = default;
        Viewport(const vireo::Viewport& v) : viewport(v) {}
        Viewport(const vireo::Viewport& v, const vireo::Rect& s) : viewport(v), scissors(s) {}
    };

    struct Camera {
        // Is the projection perspective?
        bool isPerspective{true};
        // Field of view in degrees
        float fov{75.0};
        // Camera view aspect ratio
        float aspectRatio{16.0f / 9.0f};
        // Nearest clipping distance
        float near{0.05f};
        // Furthest clipping distance
        float far{100.0f};
        // left of orthographic projection:
        float left{};
        // left of orthographic projection:
        float right{};
        // top of orthographic projection:
        float top{};
        // bottom of orthographic projection:
        float bottom{};
        // World projection matrix
        float4x4 projection{float4x4::identity()};

        unique_id camera{INVALID_ID};
    };

    struct CameraRef {
        flecs::entity camera;
    };

    struct MaterialOverride {
        uint32 surfaceIndex;
        unique_id material;
    };

    struct MeshInstance {
        unique_id mesh{INVALID_ID};
        std::shared_ptr<MeshInstanceDesc> meshInstance{};
    };

    struct Scene {
        unique_id context{INVALID_ID};
    };

    struct SceneRef {
        flecs::entity scene;
    };

    struct AmbientLight {
        float3 color{1.0f, 1.0f, 1.0f};
        float intensity{1.0f};
    };


}
