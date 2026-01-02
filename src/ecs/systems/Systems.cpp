/*
* Copyright (c) 2025-present Henri Michelon
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
*/
module lysa.ecs.systems;

import lysa.aabb;
import lysa.log;
import lysa.math;
import lysa.resources.camera;
import lysa.resources.render_target;
import lysa.renderers.graphic_pipeline_data;

namespace lysa::ecs {
    Modules::Modules(flecs::world& w) {
        meshInstanceModule = w.import<MeshInstanceModule>();
        renderModule = w.import<RenderModule>();
        transformModule = w.import<TransformModule>();
    }

    Modules::~Modules() {
        transformModule.disable();
        renderModule.disable();
        meshInstanceModule.disable();
    }

    void MeshInstanceModule::createInstance(
        const flecs::entity& e,
        MeshInstance& mi,
        const Transform& tr) const {
        if (mi.mesh_instance == INVALID_ID) {
            mi.mesh_instance = meshInstanceManager.create(
                mi.mesh,
                e.has<Visible>(),
                e.has<CastShadows>(),
                meshManager[mi.mesh].getAABB().toGlobal(tr.global),
                tr.global,
                meshManager[mi.mesh].getName()).id;
        }
    }

    void MeshInstanceModule::addInstance(
        const flecs::entity& e,
        const Scene& sc,
        const Transform& tr) const {
        if (e.has<MeshInstance>()) {
            auto& mi = e.get_mut<MeshInstance>();
            createInstance(e, mi, tr);
            sceneContextManager[sc.context].addInstance(mi.mesh_instance, false);
        }
        e.children([&](const flecs::entity& child) {
            addInstance(child, sc, tr);
        });
    }

    MeshInstanceModule::MeshInstanceModule(const flecs::world& w):
        meshManager(w.get<Context>().ctx->res.get<MeshManager>()),
        meshInstanceManager(w.get<Context>().ctx->res.get<MeshInstanceManager>()),
        sceneContextManager(w.get<Context>().ctx->res.get<SceneContextManager>()) {
        w.module<MeshInstanceModule>();
        w.component<Visible>();
        w.component<CastShadows>();
        w.component<MeshInstance>();
        w.observer<const Scene, const MeshInstance, const Transform>()
            .term_at(0).parent()
            .event(flecs::OnSet)
            .each([&](const flecs::entity e, const Scene& sc, const MeshInstance& mi, const Transform&tr) {
                if (mi.mesh == INVALID_ID) { return; }
                if (mi.mesh_instance != INVALID_ID) {
                    e.add<Updated>();
                } else {
                    addInstance(e, sc, tr);
                }
            });
        w.observer<const Scene, Transform>()
            .term_at(0).parent()
            .event(flecs::OnAdd)
            .each([&](const flecs::entity e, const Scene& sc, Transform& tr) {
                if (sc.context == INVALID_ID ) { return; }
                addInstance(e, sc, tr);
                TransformModule::updateGlobalTransform(e, tr);
            });
        w.observer<MeshInstance>()
            .event(flecs::OnRemove)
            .with(flecs::Prefab)
            .each([&](MeshInstance& mi) {
                meshInstanceManager.destroy(mi.mesh_instance);
                mi.mesh_instance = INVALID_ID;
                mi.mesh = INVALID_ID;
            });
        w.observer<MeshInstance>()
            .event(flecs::OnRemove)
            .each([&](MeshInstance& mi) {
                meshInstanceManager.destroy(mi.mesh_instance);
                mi.mesh_instance = INVALID_ID;
                mi.mesh = INVALID_ID;
            });
        w.observer<MeshInstance, const Transform>()
            .event(flecs::OnSet)
            .each([&](const flecs::entity e, MeshInstance&mi, const Transform& tr) {
                if (mi.mesh == INVALID_ID) { return; }
                if (mi.mesh_instance != INVALID_ID) {
                    e.add<Updated>();
                } else {
                    createInstance(e, mi, tr);
                }
            });
        w.observer<const Scene, const MeshInstance, const Transform>()
            .term_at(0).parent()
            .event(flecs::OnRemove)
            .each([&](const flecs::entity e, const Scene& sc, const MeshInstance& mi, const Transform&) {
                if (mi.mesh != INVALID_ID && mi.mesh_instance != INVALID_ID) {
                    auto& scene = sceneContextManager[sc.context];
                    scene.removeInstance(mi.mesh_instance, false);
                    e.children([&](const flecs::entity child) {
                        if (child.has<Transform>() && child.has<MeshInstance>()) {
                            scene.removeInstance(child.get<MeshInstance>().mesh_instance, false);
                        }
                    });
                }
            });
        w.observer<const Scene, const MeshInstance, const Visible>()
            .term_at(0).parent()
            .event(flecs::OnAdd)
            .event(flecs::OnRemove)
            .each([&](const flecs::entity e, const Scene&, const MeshInstance& mi, const Visible&) {
               if (mi.mesh != INVALID_ID && mi.mesh_instance != INVALID_ID) {
                   e.add<Updated>();
               }
           });
        w.observer<const Scene, MeshInstance, const MaterialOverride>()
            .term_at(0).parent()
            .event(flecs::OnSet)
            .each([&](const Scene&sc, MeshInstance& mi, const MaterialOverride&mo) {
               if (mi.mesh != INVALID_ID && mi.mesh_instance != INVALID_ID) {
                    auto& scene = sceneContextManager[sc.context];
                    scene.removeInstance(mi.mesh_instance, false);
                    auto& newMeshInstance = meshInstanceManager.create(meshInstanceManager[mi.mesh_instance]);
                    newMeshInstance.getMaterialsOverride()[mo.surfaceIndex] = mo.material;
                    meshInstanceManager.destroy(mi.mesh_instance);
                    mi.mesh_instance = newMeshInstance.id;
                    scene.addInstance(mi.mesh_instance, false);
               }
           });
        w.observer<const Scene, MeshInstance, const MaterialOverride>()
            .term_at(0).parent()
            .event(flecs::OnRemove)
            .each([&](const Scene&sc, MeshInstance& mi, const MaterialOverride&mo) {
               if (mi.mesh != INVALID_ID && mi.mesh_instance != INVALID_ID) {
                    auto& scene = sceneContextManager[sc.context];
                    scene.removeInstance(mi.mesh_instance, false);
                    auto& newMeshInstance = meshInstanceManager.create(meshInstanceManager[mi.mesh_instance]);
                    newMeshInstance.getMaterialsOverride().erase(mo.surfaceIndex);
                    meshInstanceManager.destroy(mi.mesh_instance);
                    mi.mesh_instance = newMeshInstance.id;
                    scene.addInstance(mi.mesh_instance, false);
               }
           });
        w.system<const Scene, const MeshInstance, const Transform, const Updated>()
            .term_at(0).parent()
            .kind(flecs::OnUpdate)
            .each([&](const flecs::entity e, const Scene& sc, const MeshInstance& mi, const Transform& tr, const Updated&) {
                if (mi.mesh != INVALID_ID && mi.mesh_instance != INVALID_ID) {
                    e.remove<Updated>();
                    auto& meshInstance = meshInstanceManager[mi.mesh_instance];
                    meshInstance.setVisible(e.has<Visible>());
                    meshInstance.setAABB(meshManager[mi.mesh].getAABB().toGlobal(tr.global));
                    meshInstance.setTransform(tr.global);
                    auto& scene = sceneContextManager[sc.context];
                    scene.updateInstance(mi.mesh_instance);
                }
            });
    }

    RenderModule::RenderModule(const flecs::world& w) {
        auto& renderTargetManager = w.get<Context>().ctx->res.get<RenderTargetManager>();
        auto& sceneContextManager = w.get<Context>().ctx->res.get<SceneContextManager>();
        auto& cameraManager = w.get<Context>().ctx->res.get<CameraManager>();
        w.module<RenderModule>();
        w.component<Scene>();
        w.component<SceneRef>();
        w.component<Camera>();
        w.component<CameraRef>();
        w.component<Viewport>();
        w.component<RenderTarget>();
        w.observer<Scene, const AmbientLight>()
            .event(flecs::OnSet)
            .event(flecs::OnAdd)
            .each([&](const Scene& sc, const AmbientLight& al) {
                auto& scene = sceneContextManager[sc.context];
                scene.setAmbientLight(float4(al.color, al.intensity));
            });
        w.observer<Scene>()
            .event(flecs::OnAdd)
            .each([&](Scene&sc) {
                sc.context = sceneContextManager.create().id;
            });
        w.observer<const Scene>()
           .event(flecs::OnRemove)
           .each([&](const Scene&sc) {
               sceneContextManager.destroy(sc.context);
           });
        w.observer<Camera, const Transform>()
            .event(flecs::OnAdd)
            .event(flecs::OnSet)
            .each([&](Camera&c, const Transform&) {
                if (c.camera == INVALID_ID) {
                    c.camera = cameraManager.create().id;
                }
            });
        w.observer<const Camera, const Transform>()
           .event(flecs::OnRemove)
           .each([&](const Camera&c, const Transform&) {
               cameraManager.destroy(c.camera);
           });
        w.observer<const RenderTarget, const CameraRef, const SceneRef>()
            .term_at(0).parent()
            .event(flecs::OnSet)
            .each([&](const flecs::entity e, const RenderTarget&rt, const CameraRef &cr, const SceneRef&sr) {
                if (!renderTargetManager.have(rt.renderTarget)) return;
                auto& renderTarget = renderTargetManager[rt.renderTarget];
                auto& scene = sceneContextManager[sr.scene.get<Scene>().context];
                const auto& camera = cameraManager[cr.camera.get<Camera>().camera];
                Viewport viewport;
                if (e.has<Viewport>()) {
                   viewport = e.get<Viewport>();
                }
                renderTarget.waitIdle();
                renderTarget.getViews().push_back({
                    static_cast<const unique_id>(e.id()),
                    viewport.viewport,
                    viewport.scissors,
                    camera,
                    scene});
        });
        w.observer<const Viewport>()
            .event(flecs::OnSet)
            .each([&](const flecs::entity e, const Viewport vp) {
                if (e.parent() && e.parent().has<RenderTarget>()) {
                    auto& rt = e.parent().get<RenderTarget>();
                    if (!renderTargetManager.have(rt.renderTarget)) return;
                    auto& renderTarget = renderTargetManager[rt.renderTarget];
                    const auto id = static_cast<const unique_id>(e.id());
                    const auto& view = std::ranges::find_if(renderTarget.getViews(), [&](const RenderView&v) {
                        return v.id == id;
                    });
                    if (view != renderTarget.getViews().end()) {
                        view->viewport = vp.viewport;
                        view->scissors = vp.scissors;
                    }
                }
        });
        w.observer<const RenderTarget, const CameraRef, const SceneRef>()
            .term_at(0).parent()
            .event(flecs::OnRemove)
            .each([&](const flecs::entity e, const RenderTarget&rt, const CameraRef &, const SceneRef&) {
                if (!renderTargetManager.have(rt.renderTarget)) return;
                auto& renderTarget = renderTargetManager[rt.renderTarget];
                const auto id = static_cast<const unique_id>(e.id());
                renderTarget.waitIdle();
                renderTarget.getViews().remove_if([&](const RenderView&view) {
                    return view.id == id;
                });
            });
        w.system<const Camera, const Transform>()
            .kind(flecs::OnUpdate)
            .each([&](const Camera& c, const Transform& tr) {
                auto& camera = cameraManager[c.camera];
                float4x4 projection{float4x4::identity()};
                if (c.isPerspective) {
                    projection = perspective(radians(c.fov), c.aspectRatio, c.near, c.far);
                } else {
                    projection = orthographic(c.left, c.right, c.top, c.bottom, c.near, c.far);
                }
                camera.position =  tr.global[3].xyz;
                camera.transform = tr.global;
                camera.projection = projection;
            });
        w.system<const RenderTarget>()
            .kind(flecs::OnUpdate)
            .each([&](const RenderTarget& rt) {
                auto& renderTarget = renderTargetManager[rt.renderTarget];
                if (!renderTargetManager.have(rt.renderTarget)) return;
                renderTarget.render();
            });

}


}
