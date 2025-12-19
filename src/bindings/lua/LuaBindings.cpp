/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module lysa.ecs.lua;

import vireo;
import lysa.context;
import lysa.lua;
import lysa.math;
import lysa.ecs.components;
import lysa.ecs.systems;

namespace lysa::ecs {

    void LuaBindings::_register(lysa::Context& ctx) {
        const auto lua = Lua(ctx, {});
        lua.beginNamespace("ecs")
            .beginClass<RenderTarget>("RenderTarget")
                .addConstructor<void(unique_id)>()
                .addProperty("render_target", &RenderTarget::renderTarget)
            .endClass()
            .beginClass<Viewport>("Viewport")
                .addConstructor<void(vireo::Viewport), void(vireo::Viewport, vireo::Rect)>()
                .addProperty("viewport", &Viewport::viewport)
                .addProperty("scissors", &Viewport::scissors)
            .endClass()
            .beginClass<Camera>("Camera")
                .addConstructor<void()>()
                .addProperty("is_perspective", &Camera::isPerspective)
                .addProperty("fov", &Camera::fov)
                .addProperty("aspect_ratio", &Camera::aspectRatio)
                .addProperty("near", &Camera::near)
                .addProperty("far", &Camera::far)
                .addProperty("left", &Camera::left)
                .addProperty("right", &Camera::right)
                .addProperty("top", &Camera::top)
                .addProperty("bottom", &Camera::bottom)
            .endClass()
            .beginClass<CameraRef>("CameraRef")
                .addConstructor<void(flecs::entity)>()
                .addProperty("camera", &CameraRef::camera)
            .endClass()
            .beginClass<MaterialOverride>("MaterialOverride")
                .addConstructor<void(), void(uint32, unique_id)>()
                .addProperty("surface_index", &MaterialOverride::surfaceIndex)
                .addProperty("material", &MaterialOverride::material)
            .endClass()
            .beginClass<MeshInstance>("MeshInstance")
                .addConstructor<void(unique_id)>()
                .addProperty("mesh", &MeshInstance::mesh)
            .endClass()
            .beginClass<Scene>("Scene")
                .addConstructor<void(unique_id)>()
                .addProperty("scene", &Scene::context)
            .endClass()
            .beginClass<SceneRef>("SceneRef")
                .addConstructor<void(flecs::entity)>()
                .addProperty("scene", &SceneRef::scene)
            .endClass()
            .beginClass<AmbientLight>("AmbientLight")
                .addConstructor<void()>()
                .addProperty("color", &AmbientLight::color)
                .addProperty("intensity", &AmbientLight::intensity)
            .endClass()
            .beginClass<Visible>("Visible")
                .addConstructor<void()>()
            .endClass()
            .beginClass<Transform>("Transform")
                .addConstructor<void()>()
                .addProperty("local", &Transform::local)
                .addProperty("global", &Transform::global)
            .endClass()

            .addFunction("set_position",
                luabridge::overload<flecs::entity, const float3&>(&setPosition),
                luabridge::overload<flecs::entity, float, float, float>(&setPosition)
            )
            .addFunction("translate",
                luabridge::overload<flecs::entity, const float3&>(&translate),
                luabridge::overload<flecs::entity, float, float, float>(&translate)
            )
            .addFunction("rotate_x",&rotateX)
            .addFunction("rotate_y",&rotateY)
            .addFunction("rotate_z",&rotateZ)

            .beginClass<flecs::world>("world")
                .addFunction("entity", +[](const flecs::world* w) { return w->entity<>(); })
            .endClass()
            .addProperty("child_of", +[]{ return flecs::ChildOf;})

            .beginClass<flecs::entity>("entity")
                .addProperty("is_alive", +[](const flecs::entity* e) { return e->is_alive(); })
                .addFunction("destruct", &flecs::entity::destruct)
                .addFunction("child_of", +[](const flecs::entity* e, const flecs::entity p) {
                    return e->add(flecs::ChildOf, p);
                })
                .addFunction("add",
                    luabridge::overload<const flecs::entity*, const flecs::entity>(+[](const flecs::entity* e, const flecs::entity c) {
                       return e->set(c);
                    }),
                    luabridge::overload<const flecs::entity*, const flecs::entity, const flecs::entity>(+[](const flecs::entity* e, const flecs::entity f, const flecs::entity s) {
                      return e->add(f, s);
                    }),
                    luabridge::overload<const flecs::entity*, const flecs::entity_t, const flecs::entity>(+[](const flecs::entity* e, const flecs::entity_t f, const flecs::entity s) {
                      return e->add(f, s);
                    }),
                    luabridge::overload<const flecs::entity*, const RenderTarget&>(+[](const flecs::entity* e, const RenderTarget& p) {
                        return e->set<RenderTarget>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const Viewport&>(+[](const flecs::entity* e, const Viewport& p) {
                        return e->set<Viewport>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const Camera&>(+[](const flecs::entity* e, const Camera& p) {
                        return e->set<Camera>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const CameraRef&>(+[](const flecs::entity* e, const CameraRef& p) {
                        return e->set<CameraRef>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const MaterialOverride&>(+[](const flecs::entity* e, const MaterialOverride& p) {
                        return e->set<MaterialOverride>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const MeshInstance&>(+[](const flecs::entity* e, const MeshInstance& p) {
                        return e->set<MeshInstance>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const Scene&>(+[](const flecs::entity* e, const Scene& p) {
                        return e->set<Scene>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const SceneRef&>(+[](const flecs::entity* e, const SceneRef& p) {
                        return e->set<SceneRef>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const AmbientLight&>(+[](const flecs::entity* e, const AmbientLight& p) {
                        return e->set<AmbientLight>(p);
                    }),
                    luabridge::overload<const flecs::entity*, const Visible&>(+[](const flecs::entity* e, const Visible&) {
                        return e->add<Visible>();
                    }),
                    luabridge::overload<const flecs::entity*, const Transform&>(+[](const flecs::entity* e, const Transform& p) {
                        return e->set<Transform>(p);
                    })
                    )
                .addFunction("has",
                    luabridge::overload<const flecs::entity*, const flecs::entity>(+[](const flecs::entity* e, const flecs::entity c) {
                       return e->has(c);
                   }),
                   luabridge::overload<const flecs::entity*, const RenderTarget&>(+[](const flecs::entity* e, const RenderTarget& p) {
                        return e->has<RenderTarget>();
                    }),
                    luabridge::overload<const flecs::entity*, const Viewport&>(+[](const flecs::entity* e, const Viewport& p) {
                        return e->has<Viewport>();
                    }),
                    luabridge::overload<const flecs::entity*, const Camera&>(+[](const flecs::entity* e, const Camera& p) {
                        return e->has<Camera>();
                    }),
                    luabridge::overload<const flecs::entity*, const CameraRef&>(+[](const flecs::entity* e, const CameraRef& p) {
                        return e->has<CameraRef>();
                    }),
                    luabridge::overload<const flecs::entity*, const MaterialOverride&>(+[](const flecs::entity* e, const MaterialOverride& p) {
                        return e->has<MaterialOverride>();
                    }),
                    luabridge::overload<const flecs::entity*, const MeshInstance&>(+[](const flecs::entity* e, const MeshInstance& p) {
                        return e->has<MeshInstance>();
                    }),
                    luabridge::overload<const flecs::entity*, const Scene&>(+[](const flecs::entity* e, const Scene& p) {
                        return e->has<Scene>();
                    }),
                    luabridge::overload<const flecs::entity*, const SceneRef&>(+[](const flecs::entity* e, const SceneRef& p) {
                        return e->has<SceneRef>();
                    }),
                    luabridge::overload<const flecs::entity*, const AmbientLight&>(+[](const flecs::entity* e, const AmbientLight& p) {
                        return e->has<AmbientLight>();
                    }),
                    luabridge::overload<const flecs::entity*, const Visible&>(+[](const flecs::entity* e, const Visible&) {
                        return e->has<Visible>();
                    }),
                    luabridge::overload<const flecs::entity*, const Transform&>(+[](const flecs::entity* e, const Transform& p) {
                        return e->has<Transform>();
                    })
                )
                .addFunction("remove",
                     luabridge::overload<const flecs::entity*, const flecs::entity>(+[](const flecs::entity* e, const flecs::entity c) {
                        return e->remove(c);
                    }),
                    luabridge::overload<const flecs::entity*, const flecs::entity, const flecs::entity>(+[](const flecs::entity* e, const flecs::entity f, const flecs::entity s) {
                       return e->remove(f, s);
                    }),
                    luabridge::overload<const flecs::entity*, const flecs::entity_t, const flecs::entity>(+[](const flecs::entity* e, const flecs::entity_t f, const flecs::entity s) {
                       return e->remove(f, s);
                    }),
                    luabridge::overload<const flecs::entity*, const RenderTarget&>(+[](const flecs::entity* e, const RenderTarget& p) {
                        return e->remove<RenderTarget>();
                    }),
                    luabridge::overload<const flecs::entity*, const Viewport&>(+[](const flecs::entity* e, const Viewport& p) {
                        return e->remove<Viewport>();
                    }),
                    luabridge::overload<const flecs::entity*, const Camera&>(+[](const flecs::entity* e, const Camera& p) {
                        return e->remove<Camera>();
                    }),
                    luabridge::overload<const flecs::entity*, const CameraRef&>(+[](const flecs::entity* e, const CameraRef& p) {
                        return e->remove<CameraRef>();
                    }),
                    luabridge::overload<const flecs::entity*, const MaterialOverride&>(+[](const flecs::entity* e, const MaterialOverride& p) {
                        return e->remove<MaterialOverride>();
                    }),
                    luabridge::overload<const flecs::entity*, const MeshInstance&>(+[](const flecs::entity* e, const MeshInstance& p) {
                        return e->remove<MeshInstance>();
                    }),
                    luabridge::overload<const flecs::entity*, const Scene&>(+[](const flecs::entity* e, const Scene& p) {
                        return e->remove<Scene>();
                    }),
                    luabridge::overload<const flecs::entity*, const SceneRef&>(+[](const flecs::entity* e, const SceneRef& p) {
                        return e->remove<SceneRef>();
                    }),
                    luabridge::overload<const flecs::entity*, const AmbientLight&>(+[](const flecs::entity* e, const AmbientLight& p) {
                        return e->remove<AmbientLight>();
                    }),
                    luabridge::overload<const flecs::entity*, const Visible&>(+[](const flecs::entity* e, const Visible&) {
                        return e->remove<Visible>();
                    }),
                    luabridge::overload<const flecs::entity*, const Transform&>(+[](const flecs::entity* e, const Transform& p) {
                        return e->remove<Transform>();
                    })
                )
                .addProperty("render_target", [](const flecs::entity* e) { return e->get<RenderTarget>(); })
                .addProperty("viewport", [](const flecs::entity* e) { return e->get<Viewport>(); })
                .addProperty("camera", [](const flecs::entity* e) { return e->get_mut<Camera>(); })
                .addProperty("camera_ref", [](const flecs::entity* e) { return e->get<CameraRef>(); })
                .addProperty("material_override", [](const flecs::entity* e) { return e->get<MaterialOverride>(); })
                .addProperty("mesh_instance", [](const flecs::entity* e) { return e->get<MeshInstance>(); })
                .addProperty("scene_ref", [](const flecs::entity* e) { return e->get<SceneRef>(); })
                .addProperty("ambient_light", [](const flecs::entity* e) { return e->get<AmbientLight>(); })
                .addProperty("transform", [](const flecs::entity* e) { return e->get<Transform>(); })
            .endClass()
        .endNamespace();
    }

}

