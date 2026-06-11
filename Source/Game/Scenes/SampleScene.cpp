#include "pch.h"
#include "SampleScene.h"

#ifdef USE_IMGUI
#define IMGUI_ENABLE_DOCKING
#include "imgui.h"
#endif

#include "Game/Actors/Player/TestPlayer.h"
#include "Components/Audio/AudioSourceComponent.h"
#include "Graphics/Core/Graphics.h"
#include "Graphics/Core/RenderState.h"
#include "Engine/Input/InputSystem.h"
#include "Core/ActorManager.h"
#include "Engine/Utility/Time.h"

#include "Game/Actors/Camera/LoadingCamera.h"
#include "Game/Actors/Enemy/Boss/BossEnemy.h"
#include "Game/Actors/Player/Player.h"
#include "Game/Actors/Stage/ElasticBuilding.h"
#include "Game/Actors/Stage/Cloth.h"


#include "Physics/Physics.h"
#include "Game/DarkGame/DarkActors/DarkStage.h"
#include "Game/DarkGame/DarkActors/DarkStageCandelabraActor.h"
#include "Game/DarkGame/DarkActors/DarkStageChandelierActor.h"

#include "Game/DarkGame/DarkActors/DarkEnemy/SkeletonWarriorEnemy.h"

#include "Physics/CollisionSystem.h"
#include "UI/UIManager.h"
#include "UI/Game/Pause.h"

bool SampleScene::Initialize(ID3D11Device* device, UINT64 width, UINT height, const std::unordered_map<std::string, std::string>& props)
{
    SceneBase::Initialize(device, width, height, props);

    Physics::Instance().Initialize();

    //アクターをセット
    SetUpActors();

#if 0
    morphModel = std::make_unique<MorphModel>(device, "./Data/Models/Morph/morphSphere.gltf");

    RegisterRenderHook(RenderPass::Opaque, [&](ID3D11DeviceContext* immediateContext)
        {
            if (const auto cloth = GetActorManager()->GetActorByName("cloth"))
            {
                morphModel->Render(immediateContext, cloth->GetWorldTransform(), {}, MorphModel::RenderPass::All);
            }
        });
#endif // 0

    RegisterRenderHook(RenderPass::Opaque, [&](ID3D11DeviceContext* immediateContext)
        {
            if (const auto cloth = GetActorManager()->GetActorByName("pauseActor"))
            {
            }
        });

    // PBD
    solver = std::make_unique<PBD::Solver>();

    auto spawn_deformable_actor =
        [&](ID3D11Device* device,
            ID3D11DeviceContext* immediate_context,
            int model_index,
            const DirectX::XMFLOAT3& position) -> int
        {
            // --- Create body in physics world ---
            size_t body_index = world.bodies.size();

            int index = world.spawn_shape_matching_body(
                deformable_models.at(model_index).get(),
                0.08f,  // stiffness
                0.5f,   // deformation_blend
                0.05f,  // radius
                1.0f,   // total_mass
                16);    // voxel_resolution

            auto& body = world.get_shape_matching_body(index);

            // --- Allocate GPU instance buffer ---
            body.instance_index =
                deformable_models[model_index]->allocate_instance_buffer(device);

            // --- Set initial position ---
            body.set_position(world.particles, position);

            // --- Upload particle positions to GPU ---
            deformable_models[model_index]->update_vertex_buffer(
                immediate_context,
                body.instance_index,
                &(world.particles[body.particle_range.offset].position),
                sizeof(PBD::PBDParticle));

#if 1
            // --- Register actor ---
            auto& actor = pbdActors.emplace_back();
            actor.model = model_index;
            actor.body = static_cast<int>(body_index);

            return static_cast<int>(pbdActors.size() - 1);

#endif // 0
            return true;
        };

    ID3D11DeviceContext* immediateContext = Graphics::GetDeviceContext();

    int model_0 = static_cast<int>(deformable_models.size());
    deformable_models.emplace_back(std::make_unique<class deformable_model>(device, "./Data/Models/PBD/YarnEnemy.glb", 1.f/*scale_factor*/));
    //deformable_models.emplace_back(std::make_unique<class deformable_model>(device.Get(), "./resources/pikachu.glb", 0.03f/*scale_factor*/));
    spawn_deformable_actor(device, immediateContext, model_0, { -1, 2, 0 });
    spawn_deformable_actor(device, immediateContext, model_0, { 1, 2, 0 });

    int model_1 = static_cast<int>(deformable_models.size());
    deformable_models.emplace_back(std::make_unique<class deformable_model>(device ,"./Data/Models/PBD/kirby_1.glb", 0.05f/*scale_factor*/));
    spawn_deformable_actor(device, immediateContext, model_1, { 0, 2, 0 });


    world.spawn_collision_shape<PBD::plane_shape>(DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f }/*normal*/, -1.0f/*distance*/, 0x0001/*phase*/);
    world.spawn_collision_shape<PBD::sphere_shape>(DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f }/*center*/, 0.5f/*radius*/, 0x0001/*phase*/);

    particle_debug_renderer = std::make_unique<class particle_debug_renderer>(device, world.particles.size());


    RegisterRenderHook(RenderPass::ForwardBlend, [&](ID3D11DeviceContext* immediateContext)
        {
            RenderState::BindDepthStencilState(immediateContext, DEPTH_STATE::ZT_ON_ZW_ON);
            RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_BACK);
#if 1

            // --------------------------------------------------------
            // Particleデバッグ描画
            // --------------------------------------------------------
            if (show_particles)
            {
                particle_debug_renderer->draw(immediateContext, world.particles);
            }
            else
            {
                // 現在のParticle位置からGPU頂点バッファを更新し、
                // 変形後のメッシュを描画する
                for (const auto& actor : pbdActors)
                {
                    auto& body = world.bodies[actor.body];
#if 0
                    if (body.active == false)
                    {
                        continue;
                    }
#endif // 0

                    DirectX::XMFLOAT4X4 deformation_rotation;
                    DirectX::XMStoreFloat4x4(&deformation_rotation, body.transform);
                    if (show_wireframe)
                        RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::WIREFRAME_CULL_NONE);
                    else
                        RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_NONE);
                    deformable_models[actor.model]->draw(immediateContext, body.instance_index, deformation_rotation);
                }
            }
#endif // 0
        });


    return true;
}

void SampleScene::Start()
{


}

void SampleScene::Update(float deltaTime)
{
    using namespace DirectX;
    SceneBase::Update(deltaTime);

    Physics::Instance().Update(Time::UnscaledDeltaTime());
    CollisionSystem::DetectAndResolveCollisions();
    CollisionSystem::ApplyPushAll();

    // PBD
    auto& body = world.get_shape_matching_body(0);
    if (enable_simulation && GetAsyncKeyState('R') & 0x8000)
    {
        body.reset_to_rest_state(world.particles);
        body.set_position(world.particles, { 0, 2, 0 });
        body.scale = 1.0f;
    }

    if (enable_simulation && GetAsyncKeyState('T') & 0x8000)
    {
        body.set_position(world.particles, { 0, 2, 0 });
    }

    DirectX::XMVECTOR forward = body.transform.r[2];
    DirectX::XMVECTOR axis = body.transform.r[1];
    if (enable_simulation && GetKeyState(VK_UP) & 0x8000)
    {
        body.translate(world.particles, DirectX::XMVectorScale(forward, deltaTime));
    }
    if (enable_simulation && GetKeyState(VK_DOWN) & 0x8000)
    {
        body.translate(world.particles, DirectX::XMVectorScale(forward, -deltaTime));
    }
    if (enable_simulation && GetKeyState(VK_LEFT) & 0x8000)
    {
        DirectX::XMVECTOR rotation = DirectX::XMQuaternionRotationRollPitchYaw(0, -1.0f * deltaTime, 0);
        body.rotate(world.particles, rotation, body.compute_center_of_mass(world.particles));
    }
    if (enable_simulation && GetKeyState(VK_RIGHT) & 0x8000)
    {
        DirectX::XMVECTOR rotation = DirectX::XMQuaternionRotationRollPitchYaw(0, 1.0f * deltaTime, 0);
        body.rotate(world.particles, rotation, body.compute_center_of_mass(world.particles));
    }

    //#ifdef _DEBUG
#if 0
    if (InputSystem::GetInputState("Space", InputStateMask::Trigger))
    {
        const char* types[] = { "0", "1" };
        Scene::_transition("LoadingScene", { std::make_pair("preload", "SampleScene"), std::make_pair("type", types[rand() % 2]) });
    }

#endif // 0
    //#endif // !_DEBUG
}

void SampleScene::Render(ID3D11DeviceContext* immediateContext, float deltaTime)
{

    // PBD
    if (GetAsyncKeyState('K') & 0x8000)
    {
        enable_simulation = !enable_simulation;
    }
    if (enable_simulation)
    {
        // --------------------------------------------------------
        // 経過時間を蓄積する
        //
        // 描画フレームレートは可変なので、
        // 物理シミュレーションを固定時間ステップで
        // 実行するために時間を蓄積する。
        // --------------------------------------------------------
        time_accumulator += deltaTime;

        // --------------------------------------------------------
        // 固定時間ステップで物理シミュレーションを進める
        //
        // 蓄積時間が physics_time_step を超えている場合、
        // 1フレーム中に複数回シミュレーションを実行する。
        // --------------------------------------------------------
        while (time_accumulator >= physics_time_step)
        {
            // PBD Solverを1ステップ実行
            solver->step(world, physics_time_step);
            // 使用した時間分を蓄積時間から減算
            time_accumulator -= physics_time_step;


            // ----------------------------------------------------
            // 変形メッシュの頂点バッファ更新
            //
            // Particle位置をGPUへ転送し、
            // メッシュ形状へ反映する
            // ----------------------------------------------------
            for (const auto& actor : pbdActors)
            {
                auto& body = world.bodies[actor.body];
                if (body.active == false)
                {
                    continue;
                }
                deformable_models[actor.model]->update_vertex_buffer(immediateContext, body.instance_index, &(world.particles[body.particle_range.offset].position), sizeof(PBD::PBDParticle));
            }
        }
    }

    SceneBase::Render(immediateContext, deltaTime);

#if 0

    // --------------------------------------------------------
    // Particleデバッグ描画
    // --------------------------------------------------------
    if (show_particles)
    {
        particle_debug_renderer->draw(immediateContext, world.particles);
    }
    else
    {
        // 現在のParticle位置からGPU頂点バッファを更新し、
        // 変形後のメッシュを描画する
        for (const auto& actor : pbdActors)
        {
            auto& body = world.bodies[actor.body];
#if 0
            if (body.active == false)
            {
                continue;
            }
#endif // 0

            DirectX::XMFLOAT4X4 deformation_rotation;
            DirectX::XMStoreFloat4x4(&deformation_rotation, body.transform);
            if (show_wireframe)
                RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::WIREFRAME_CULL_NONE);
            else
                RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_NONE);
            deformable_models[actor.model]->draw(immediateContext, body.instance_index, deformation_rotation);
        }
    }
#endif // 0



#if 0
    for (const auto& collision_shape : world.collision_shapes)
    {
        //position_based_dynamics::plane_shape* plane_shape = dynamic_cast<position_based_dynamics::plane_shape*>(collision_shape.get());
        if (PBD::plane_shape* plane_shape = dynamic_cast<PBD::plane_shape*>(collision_shape.get()))
        {

            DirectX::XMFLOAT3 plane_normal = plane_shape->normal;
            float plane_d = plane_shape->distance;

            DirectX::XMVECTOR N = DirectX::XMVector3Normalize(XMLoadFloat3(&plane_normal));
            DirectX::XMVECTOR T;
            if (fabsf(DirectX::XMVectorGetX(N)) < 0.9f)
                T = DirectX::XMVectorSet(1, 0, 0, 0);
            else
                T = DirectX::XMVectorSet(0, 0, 1, 0);

            DirectX::XMVECTOR B = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(T, N));
            T = DirectX::XMVector3Cross(N, B);

            DirectX::XMMATRIX R;
            R.r[0] = T;
            R.r[1] = N;
            R.r[2] = B;
            R.r[3] = DirectX::XMVectorSet(0, 0, 0, 1);

            DirectX::XMVECTOR O = DirectX::XMVectorScale(N, plane_d);

            DirectX::XMFLOAT4X4 transform;
            DirectX::XMStoreFloat4x4(&transform, DirectX::XMMatrixScaling(5.0f, 5.0f, 5.0f) * R * DirectX::XMMatrixTranslationFromVector(O));

            RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::WIREFRAME_CULL_NONE);
            plane->render(immediateContext, transform, {});
        }
        else if (PBD::sphere_shape* sphere_shape = dynamic_cast<PBD::sphere_shape*>(collision_shape.get()))
        {
            DirectX::XMFLOAT3 sphere_center = sphere_shape->center;
            float sphere_radius = sphere_shape->radius;

            DirectX::XMFLOAT4X4 transform;
            DirectX::XMStoreFloat4x4(&transform, DirectX::XMMatrixScaling(sphere_radius, sphere_radius, sphere_radius) * DirectX::XMMatrixTranslation(sphere_center.x, sphere_center.y, sphere_center.z));
            RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::WIREFRAME_CULL_NONE);
            sphere->render(immediateContext, transform, {});
        }
    }
#endif // 0

}

void SampleScene::SetUpActors()
{
    auto mainCameraActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<MainCamera>("mainCameraActor");
    auto mainCameraComponent = mainCameraActor->GetComponent<TPSCameraComponent>();

    Transform enemyTr(DirectX::XMFLOAT3{ -0.0f,0.0f,0.0f }, DirectX::XMFLOAT3{ 0.0f,180.0f,0.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    auto enemy = this->GetActorManager()->CreateAndRegisterActorWithTransform<Actor>("enemy", enemyTr);

    mainCameraActor->SetTarget(enemy->GetRootComponent());
    SetActiveCamera(mainCameraActor);
    Logger::Log(U8("morphシーンのカメラ設定される。"));

    Transform stageTr(DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    auto stage = this->GetActorManager()->CreateAndRegisterActorWithTransform<Stage>("stage", stageTr);

    auto debugCameraActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<DebugCamera>("debugCam");
    debugCameraActor->SetPosition({ 0.0f,10.0f,-20.0f });

    //building->AddComponent<StaticMeshComponent>("cloth")->SetModel("./Data/Models/ClothFlag/pole.gltf");

    Transform buildTr2(DirectX::XMFLOAT3{ -3.0f,0.45f,3.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 0.8f,0.8f,0.8f });
    auto pauseActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<Pause>("pauseActor", buildTr2);

    Transform playerTr(DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    auto player = this->GetActorManager()->CreateAndRegisterActorWithTransform<Player>("player", playerTr);

#if 1
    Transform testPlayerTr(DirectX::XMFLOAT3{ 3.0f,0.0f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    auto testPlayer = this->GetActorManager()->CreateAndRegisterActorWithTransform<TestPlayer>("testPlayer", testPlayerTr);
#endif // 1

    std::shared_ptr<StageAsset> stageCandelabraAsset = std::make_shared<StageAsset>();
    stageCandelabraAsset->model = std::make_shared<InterleavedGltfModel>(Graphics::GetDevice(), "./Data/Models/DarkStageAssets/Candelabra/Candelabra.gltf", ModelTypes::ModelMode::StaticMesh, false, true);
    stageCandelabraAsset->spawnPoints = stageCandelabraAsset->model->spawnPoints;

    Transform chandelierTr(DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f }, DirectX::XMFLOAT3{ 0.0f,180.0f,0.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    auto chandelier = this->GetActorManager()->CreateAndRegisterActorWithTransform<DarkStageCandelabraActor>("chandelier", chandelierTr);
    chandelier->SetModel(stageCandelabraAsset);


    //Transform chandelierTr(DirectX::XMFLOAT3{ 0.0f,3.0f,0.0f }, DirectX::XMFLOAT3{ 0.0f,180.0f,0.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    //auto chandelier = this->GetActorManager()->CreateAndRegisterActorWithTransform<DarkStageChandelierActor>("chandelier", chandelierTr);

    cameraManager->SetDebugCamera(debugCameraActor);
}

bool SampleScene::Uninitialize(ID3D11Device* device)
{
    SceneBase::Uninitialize(device);
    Physics::Instance().Finalize();
    return true;
}

void SampleScene::DrawGui()
{
    SceneBase::DrawGui();
}
