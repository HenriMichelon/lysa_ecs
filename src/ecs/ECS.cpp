/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module lysa.ecs;

import lysa.ecs.components;

namespace lysa::ecs {

    ecs::ecs(lysa::Context& ctx
#ifdef LUA_BINDING
        , const lysa::Lua& lua
#endif
        ):
        world(flecs::world()) {
        world.set<Context>({&ctx});
#ifdef LUA_BINDING
        LuaBindings::_register(lua);
        world.set<Lua>({&lua});
#endif
        modules = std::make_unique<Modules>(world);
        ctx.events.subscribe(MainLoopEvent::PROCESS, [&] (const Event&){
            if (!world.progress()) {
                ctx.exit = true;
            }
        });
    }

    flecs::entity& load(flecs::entity& root, const std::string &fileURI) {
        auto stream = root.world().get<Context>().ctx->fs.openReadStream(fileURI);
        return load(root, stream);
    }

    flecs::entity& load(flecs::entity& root, std::ifstream &stream) {
        const auto& world = root.world();
        const bool isPrefab = root.has(flecs::Prefab);
        AssetsPack::load(*world.get<Context>().ctx, stream, [&](
           const std::vector<AssetsPack::NodeHeader>& nodeHeaders,
           const std::vector<unique_id>& meshes,
           const std::vector<std::vector<uint32>>& childrenIndexes) {
            std::vector<flecs::entity> entities{(nodeHeaders.size())};
            for (auto nodeIndex = 0; nodeIndex < nodeHeaders.size(); ++nodeIndex) {
                entities[nodeIndex] =
                    world.entity(nodeHeaders[nodeIndex].name)
                    .add<lysa::ecs::Visible>();
                if (isPrefab) {
                    entities[nodeIndex].add(flecs::Prefab);
                }
                if (nodeHeaders[nodeIndex].meshIndex != -1) {
                    const auto meshId = meshes[nodeHeaders[nodeIndex].meshIndex];
                    entities[nodeIndex].set<lysa::ecs::MeshInstance>({.mesh = meshId});
                }
                entities[nodeIndex].set<lysa::ecs::Transform>({.local = nodeHeaders[nodeIndex].transform});
            }

            // for (auto animationIndex = 0; animationIndex < header.animationsCount; animationIndex++) {
            //   for (auto trackIndex = 0; trackIndex < animationHeaders[animationIndex].tracksCount; trackIndex++) {
            //       auto nodeIndex = tracksInfos[animationIndex][trackIndex].nodeIndex;
            //       auto& player = animationPlayers[nodeIndex];
            //       if (!player->getParent()) {
            //           auto& node = nodes[nodeIndex];
            //           // player->setNode(node);
            //           node->addChild(player);
            //       }
            //   }
            // }

            // Build the scene tree
            for (auto nodeIndex = 0; nodeIndex < nodeHeaders.size(); ++nodeIndex) {
               auto& parent = entities[nodeIndex];
               for (auto i = 0; i < nodeHeaders[nodeIndex].childrenCount; i++) {
                   entities[childrenIndexes[nodeIndex][i]].add(flecs::ChildOf, parent);
               }
            }
            // find the top nodes, with no parents
            for (auto &entity : entities) {
                if (!entity.parent()) {
                    entity.add(flecs::ChildOf, root);
                }
            }
        });
        return root;
    }

}
