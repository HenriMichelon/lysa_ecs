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

    void MeshInstanceModule::addInstance(
        const flecs::entity& e,
        const Scene& sc,
        const Transform& tr) const {
        if (e.has<MeshInstance>()) {
            auto& mi = e.get_mut<MeshInstance>();
            if (!mi.meshInstance) {
                mi.meshInstance = std::make_shared<MeshInstanceDesc>(
                    meshManager[mi.mesh],
                    e.has<Visible>(),
                    e.has<CastShadows>(),
                    meshManager[mi.mesh].getAABB().toGlobal(tr.global),
                    tr.global);
            }
            sceneContextManager[sc.context].addInstance(mi.meshInstance, false);
        }
        e.children([&](const flecs::entity child) {
            addInstance(child, sc, tr);
        });
    }

    MeshInstanceModule::MeshInstanceModule(const flecs::world& w):
        meshManager(w.get<Context>().ctx->res.get<MeshManager>()),
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
                if (mi.meshInstance) {
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
            .each([&](MeshInstance& mi) {
                meshManager.destroy(mi.mesh);
                mi.mesh = INVALID_ID;
                mi.meshInstance.reset();
            });
        w.observer<const Scene, const MeshInstance, const Transform>()
            .term_at(0).parent()
            .event(flecs::OnRemove)
            .each([&](const flecs::entity e, const Scene& sc, const MeshInstance& mi, const Transform&) {
                if (mi.mesh != INVALID_ID && mi.meshInstance) {
                    auto& scene = sceneContextManager[sc.context];
                    scene.removeInstance(mi.meshInstance, false);
                    e.children([&](const flecs::entity child) {
                        if (child.has<Transform>() && child.has<MeshInstance>()) {
                            scene.removeInstance(child.get<MeshInstance>().meshInstance, false);
                        }
                    });
                }
            });
        w.observer<const Scene, const MeshInstance, const Visible>()
            .term_at(0).parent()
            .event(flecs::OnAdd)
            .event(flecs::OnRemove)
            .each([&](const flecs::entity e, const Scene&, const MeshInstance& mi, const Visible&) {
               if (mi.mesh != INVALID_ID && mi.meshInstance) {
                   e.add<Updated>();
               }
           });
        w.observer<const Scene, MeshInstance, const MaterialOverride>()
            .term_at(0).parent()
            .event(flecs::OnSet)
            .each([&](const Scene&sc, MeshInstance& mi, const MaterialOverride&mo) {
               if (mi.mesh != INVALID_ID && mi.meshInstance) {
                    auto& scene = sceneContextManager[sc.context];
                    scene.removeInstance(mi.meshInstance, false);
                    mi.meshInstance = std::make_shared<MeshInstanceDesc>(*mi.meshInstance);
                    mi.meshInstance->materialsOverride[mo.surfaceIndex] = mo.material;
                    scene.addInstance(mi.meshInstance, false);
               }
           });
        w.observer<const Scene, MeshInstance, const MaterialOverride>()
            .term_at(0).parent()
            .event(flecs::OnRemove)
            .each([&](const Scene&sc, MeshInstance& mi, const MaterialOverride&mo) {
               if (mi.mesh != INVALID_ID && mi.meshInstance) {
                    auto& scene = sceneContextManager[sc.context];
                    scene.removeInstance(mi.meshInstance, false);
                    mi.meshInstance = std::make_shared<MeshInstanceDesc>(*mi.meshInstance);
                    mi.meshInstance->materialsOverride.erase(mo.surfaceIndex);
                    scene.addInstance(mi.meshInstance, false);
               }
           });
        w.system<const Scene, const MeshInstance, const Transform, const Updated>()
            .term_at(0).parent()
            .kind(flecs::OnUpdate)
            .each([&](const flecs::entity e, const Scene& sc, const MeshInstance& mi, const Transform& tr, const Updated&) {
                if (mi.mesh != INVALID_ID && mi.meshInstance) {
                    e.remove<Updated>();
                    mi.meshInstance->visible = e.has<Visible>();
                    mi.meshInstance->worldAABB = meshManager[mi.mesh].getAABB().toGlobal(tr.global);
                    mi.meshInstance->worldTransform = tr.global;
                    auto& scene = sceneContextManager[sc.context];
                    scene.updateInstance(mi.meshInstance);
                }
            });
    }

    RenderModule::RenderModule(const flecs::world& w) {
        auto& renderTargetManager = w.get<Context>().ctx->res.get<RenderTargetManager>();
        auto& sceneContextManager = w.get<Context>().ctx->res.get<SceneContextManager>();
        w.module<RenderModule>();
        w.component<Scene>();
        w.component<SceneRef>();
        w.component<Camera>();
        w.component<CameraRef>();
        w.component<Viewport>();
        w.component<RenderTarget>();
        w.observer<Camera>()
            .event(flecs::OnSet)
            .event(flecs::OnAdd)
            .each([&](Camera& cp) {
                if (cp.isPerspective) {
                    cp.projection = perspective(radians(cp.fov), cp.aspectRatio, cp.near, cp.far);
                } else {
                    cp.projection = orthographic(cp.left, cp.right, cp.top, cp.bottom, cp.near, cp.far);
                }
            });
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
        w.observer<const RenderTarget, const CameraRef, const SceneRef>()
            .term_at(0).parent()
            .event(flecs::OnSet)
            .each([&](const flecs::entity e, const RenderTarget&rt, const CameraRef &cr, const SceneRef&sr) {
                if (!renderTargetManager.have(rt.renderTarget)) return;
                auto& renderTarget = renderTargetManager[rt.renderTarget];
                auto& scene = sceneContextManager[sr.scene.get<Scene>().context];
                const auto camera = cr.camera.get<Camera>();
                const auto cameraTransform = cr.camera.get<Transform>().global;
                const auto cameraDesc = CameraDesc{
                   .position = cameraTransform[3].xyz,
                   .transform = cameraTransform,
                   .projection = camera.projection
                };
                Viewport viewport;
                if (e.has<Viewport>()) {
                   viewport = e.get<Viewport>();
                }
                renderTarget.waitIdle();
                renderTarget.getViews().push_back({
                    static_cast<const unique_id>(e.id()),
                    viewport.viewport,
                    viewport.scissors,
                    cameraDesc,
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
        w.system<const RenderTarget>()
            .kind(flecs::OnUpdate)
            .each([&](const RenderTarget& rt) {
                auto& renderTarget = renderTargetManager[rt.renderTarget];
                if (!renderTargetManager.have(rt.renderTarget)) return;
                renderTarget.render();
            });
        }


}
