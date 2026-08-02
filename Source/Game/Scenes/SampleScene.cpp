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
#include "Game/SofyBody/CubeActor.h"
#include "Game/SofyBody/PlaneActor.h"
#include "Game/SofyBody/SphereActor.h"
#include "Game/SofyBody/CarActor.h"
#include "Game/SofyBody/GearActor.h"

#include "Physics/CollisionSystem.h"
#include "UI/UIManager.h"
#include "UI/Game/Pause.h"

bool SampleScene::Initialize(ID3D11Device* device, UINT64 width, UINT height, const std::unordered_map<std::string, std::string>& props)
{
    SceneBase::Initialize(device, width, height, props);

    Physics::Instance().Initialize();

    //アクターをセット
    SetUpActors();

    // PBD
    solver = std::make_unique<PBD::Solver>();

    auto spawn_deformable_actor =
        [&](ID3D11Device* device,
            ID3D11DeviceContext* immediateContext,
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
                immediateContext,
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
        };

    ID3D11DeviceContext* immediateContext = Graphics::GetDeviceContext();

    int model_0 = static_cast<int>(deformable_models.size());
    //deformable_models.emplace_back(std::make_unique<class deformable_model>(device, "./Data/Models/PBD/YarnEnemy.glb", 1.f/*scale_factor*/));
    deformable_models.emplace_back(std::make_unique<class deformable_model>(device, "./Data/Models/Car/red.glb", 1.0f/*scale_factor*/));
    //deformable_models.emplace_back(std::make_unique<class deformable_model>(device, "./resources/pikachu.glb", 0.03f/*scale_factor*/));
    //deformable_models.emplace_back(std::make_unique<class deformable_model>(device, "./Data/Models/PBD/kirby_1.glb", 0.05f/*scale_factor*/));
    shapeMatchingBodyIndex = spawn_deformable_actor(device, immediateContext, model_0, { -1, 2, 0 });


    // 車のアクターを生成する
    //Transform carTr(DirectX::XMFLOAT3{ -0.0f,0.0f,0.0f }, DirectX::XMFLOAT3{ 0.0f,180.0f,0.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    Transform carTr(DirectX::XMFLOAT3{ -0.0f,0.0f,0.0f }, DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    carActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<CarActor>("carActor", carTr);
    carActor->shapeMatchingBodyIndex = shapeMatchingBodyIndex;

#if 0
    index = spawn_deformable_actor(device, immediateContext, model_0, { 1, 2, 0 });

    int model_1 = static_cast<int>(deformable_models.size());
    deformable_models.emplace_back(std::make_unique<class deformable_model>(device, "./Data/Models/PBD/kirby_1.glb", 0.05f/*scale_factor*/));
    index = spawn_deformable_actor(device, immediateContext, model_1, { 0, 2, 0 });

#endif // 0
    planeIndex = world.spawn_collision_shape<PBD::plane_shape>(DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f }/*normal*/, 0.0f/*distance*/, 0x0001/*phase*/);
    frontPlaneIndex = world.spawn_collision_shape<PBD::plane_shape>(DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f }/*normal*/, -10.0f/*distance*/, 0x0001/*phase*/);
    backPlaneIndex = world.spawn_collision_shape<PBD::plane_shape>(DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f }/*normal*/, -10.0f/*distance*/, 0x0001/*phase*/);
    //world.spawn_collision_shape<PBD::plane_shape>(DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f }/*normal*/, 0.0f/*distance*/, 0x0001/*phase*/);
    sphereIndex = world.spawn_collision_shape<PBD::sphere_shape>(DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f }/*center*/, 0.5f/*radius*/, 0x0001/*phase*/);
    sphereIndex1 = world.spawn_collision_shape<PBD::sphere_shape>(DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f }/*center*/, 0.5f/*radius*/, 0x0001/*phase*/);

    boxIndex = world.spawn_collision_shape<PBD::box_shape>(DirectX::XMFLOAT3{ -1.5f,-1.5f,-1.5f }, DirectX::XMFLOAT3{ 1.5f,1.5f,1.5f }, 0x0001/*phase*/);
    boxIndex1 = world.spawn_collision_shape<PBD::box_shape>(DirectX::XMFLOAT3{ -1.5f,-1.5f,-1.5f }, DirectX::XMFLOAT3{ 1.5f,1.5f,1.5f }, 0x0001/*phase*/);

    particle_debug_renderer = std::make_unique<class particle_debug_renderer>(device, world.particles.size());

    damageConstantBuffer = std::make_shared<ConstantBuffer<DamageConstants>>(device);

    return true;
}

void SampleScene::Start()
{


}

void SampleScene::Update(float deltaTime)
{
    using namespace DirectX;

    HandleInput(deltaTime);

    SceneBase::Update(deltaTime);

    auto& boxShape = world.get_collision_shape(boxIndex);
    if (auto* box = dynamic_cast<PBD::box_shape*>(&boxShape))
    {
        box->center = cubeActor->GetPosition();
        box->extent = cubeActor->extent;
        box->rotation = cubeActor->GetRotationMatrix3X3();
    }
    auto& boxShape1 = world.get_collision_shape(boxIndex1);
    if (auto* box = dynamic_cast<PBD::box_shape*>(&boxShape1))
    {
        box->center = cubeActor1->GetPosition();
        box->extent = cubeActor1->extent;
        box->rotation = cubeActor1->GetRotationMatrix3X3();
    }

    auto& plane = world.get_collision_shape(planeIndex);
    if (auto* planeShape = dynamic_cast<PBD::plane_shape*>(&plane))
    {
        planeShape->distance = planeActor->GetPosition().y;
        planeShape->normal = DirectX::XMFLOAT3{ 0.0f,1.0f,0.0f };
    }

    //damageConstantBuffer->data.hitPosition = center;
    //damageConstantBuffer->data.radius = radius;

    DebugRender::DrawSphere(damageConstantBuffer->data.hitPosition, damageConstantBuffer->data.radius, { 1,1,1,1 }, 0.0f, true);

#if 0
    auto& frontPlane = world.get_collision_shape(frontPlaneIndex);
    if (auto* planeShape = dynamic_cast<PBD::plane_shape*>(&frontPlane))
    {
        planeShape->distance = frontPlaneActor->GetPosition().x;
        XMVECTOR q = MathHelper::QuaternionFromTo(
            XMVectorSet(0, 1, 0, 0),
            XMLoadFloat3(&planeShape->normal));
        XMFLOAT4 rotation;
        XMStoreFloat4(&rotation, q);
        frontPlaneActor->SetQuaternionRotation(rotation);
    }
    auto& backPlane = world.get_collision_shape(backPlaneIndex);
    if (auto* planeShape = dynamic_cast<PBD::plane_shape*>(&backPlane))
    {
        planeShape->distance = backPlaneActor->GetPosition().x;
        XMVECTOR q = MathHelper::QuaternionFromTo(
            XMVectorSet(0, 1, 0, 0),
            XMLoadFloat3(&planeShape->normal));
        XMFLOAT4 rotation;
        XMStoreFloat4(&rotation, q);
        backPlaneActor->SetQuaternionRotation(rotation);
    }
#endif // 1


#if 1
    // Box 衝突（複数あるなら全部チェック）
    for (int boxIndex : boxIndices)
    {
        auto& shape = world.get_collision_shape(boxIndex);
        if (auto* box = dynamic_cast<PBD::box_shape*>(&shape))
        {
            DirectX::XMFLOAT3 boxMin, boxMax;

            PBD::ComputeAABBFromOBB(
                box->center,
                box->extent,
                box->rotation,
                boxMin,
                boxMax);
            if (carActor)
                PBD::SolveBoxForRigidBody(carActor->GetRigidBody(), boxMin, boxMax, 0.5f);
        }
    }

#if 0
    // Box 衝突（複数あるなら全部チェック）
    for (int boxIndex : gearActor->GetPbdBoxIndices())
    {
        auto& shape = world.get_collision_shape(boxIndex);
        if (auto* box = dynamic_cast<PBD::box_shape*>(&shape))
        {
            DirectX::XMFLOAT3 boxMin, boxMax;

            PBD::ComputeAABBFromOBB(
                box->center,
                box->extent,
                box->rotation,
                boxMin,
                boxMax);
            if (carActor)
                PBD::SolveBoxForRigidBody(carActor->GetRigidBody(), boxMin, boxMax, 0.5f);
        }
    }

#endif // 0


#endif // 0
    Physics::Instance().Update(Time::UnscaledDeltaTime());
    CollisionSystem::DetectAndResolveCollisions();
    CollisionSystem::ApplyPushAll();
#if 1
    auto& sphereA = world.get_collision_shape(sphereIndex);
    if (auto* sphere = dynamic_cast<PBD::sphere_shape*>(&sphereA))
    {
        sphere->center = center;
        sphere->radius = radius;
        sphere->angularVelocity = angularVelocity;

        sphereActor->SetPosition(sphere->center);
        DirectX::XMFLOAT3 angleDegree = sphereActor->GetEulerRotation();
        angleDegree = MathHelper::Add(angleDegree, sphere->angularVelocity);
        sphereActor->SetEulerRotation(angleDegree);
    }

    auto& sphereB = world.get_collision_shape(sphereIndex1);
    if (auto* sphere = dynamic_cast<PBD::sphere_shape*>(&sphereB))
    {
        sphere->center = center1;
        sphere->radius = radius1;
        sphere->angularVelocity = angularVelocity1;

        sphereActor1->SetPosition(sphere->center);
        DirectX::XMFLOAT3 angleDegree = sphereActor1->GetEulerRotation();
        angleDegree = MathHelper::Add(angleDegree, sphere->angularVelocity);
        sphereActor1->SetEulerRotation(angleDegree);
    }
#endif // 0

    // CarActor と ShapeMatchingBody を紐付けてある前提
#if 1
    auto& rb = carActor->GetRigidBody();
    auto& body = world.get_shape_matching_body(carActor->shapeMatchingBodyIndex);

    // 剛体の姿勢を ShapeMatchingBody にコピー
    body.rigid_position = rb.position;
    body.rigid_rotation_quat = rb.rotation;

#endif // 0

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
                deformable_models[actor.model]->update_vertex_buffer(Graphics::GetDeviceContext(), body.instance_index, &(world.particles[body.particle_range.offset].position), sizeof(PBD::PBDParticle));
            }
        }
    }
#if 1
    // 5. 衝突後の粒子から「剛体の位置」を少しだけ追従させる
    {
        XMVECTOR c_after = body.compute_center_of_mass(world.particles);
        DirectX::XMStoreFloat3(&rb.position, c_after);
        // 回転も追従したければ、A から R を取って quaternion に戻す手もある
    }
#endif // 0
}

void SampleScene::Render(ID3D11DeviceContext* immediateContext, float deltaTime)
{
#ifdef USE_IMGUI
    imGuiGizmoBuffer->Clear(immediateContext);
    imGuiGizmoBuffer->Activate(immediateContext);
#endif
    damageConstantBuffer->Activate(immediateContext, 12);

    UpdateConstantBuffer(immediateContext, deltaTime);
    ViewConstants data = {};
    if (auto camera = cameraManager->GetRenderCamera(this))
    {
        data = camera->GetViewConstants();
        sceneRender.UpdateViewConstants(immediateContext, data);
    }
    else
    {
        Logger::Error(U8("カメラがない"));
    }

    multipleRenderTargets->Clear(immediateContext);
    multipleRenderTargets->Activate(immediateContext);

    //auto camera = CameraManager::GetRenderCamera(this);
    auto camera = cameraManager->GetRenderCamera(this);
    if (!camera)
        return;

    // スカイマップを描画
    RenderState::BindDepthStencilState(immediateContext, DEPTH_STATE::ZT_OFF_ZW_OFF);
    RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_NONE);
    skyMap->Blit(immediateContext, data.viewProjection);
    ExecuteHooks(RenderPass::Sky, immediateContext);

    auto queues = sceneRender.BuildRenderQueues();

    // オブジェクトを描画
    RenderState::BindBlendState(immediateContext, BLEND_STATE::MULTIPLY_RENDER_TARGET_ALPHA);
    RenderState::BindDepthStencilState(immediateContext, DEPTH_STATE::ZT_ON_ZW_ON);
    //RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_BACK);
    RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_NONE);
    sceneRender.currentRenderPath = RenderPath::Forward;
    sceneRender.RenderOpaque(immediateContext, queues.meshes);
    RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_NONE);
    ExecuteHooks(RenderPass::Opaque, immediateContext);
    sceneRender.RenderMask(immediateContext, queues.meshes);
    ExecuteHooks(RenderPass::Mask, immediateContext);

#if 1
    // Particleデバッグ描画
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

            //RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::WIREFRAME_CULL_NONE);
            //DebugRender::DrawBox({ 0.0f,-1.0f,0.0f }, { 10.0f,1.0f,10.0f }, { 1,1,0,1 }, 0, true);
        }
        else if (PBD::sphere_shape* sphere_shape = dynamic_cast<PBD::sphere_shape*>(collision_shape.get()))
        {
            DirectX::XMFLOAT3 sphere_center = sphere_shape->center;
            float sphere_radius = sphere_shape->radius;

            DirectX::XMFLOAT4X4 transform;
            DirectX::XMStoreFloat4x4(&transform, DirectX::XMMatrixScaling(sphere_radius, sphere_radius, sphere_radius) * DirectX::XMMatrixTranslation(sphere_center.x, sphere_center.y, sphere_center.z));
            RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::WIREFRAME_CULL_NONE);
            DebugRender::DrawSphere(sphere_center, sphere_radius, { 1,0,0,1 });
        }
    }
#endif // 0

    sceneRender.RenderBlend(immediateContext, queues.meshes);
    sceneRender.RenderBlend(immediateContext, queues.meshes);
    ExecuteHooks(RenderPass::ForwardBlend, immediateContext);

    // デバック描画
#if _DEBUG
    if (useDrawDebug)
    {
        RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_BACK);
        //Physics::Instance().Render(data.view, data.projection, { lightManager->GetLightDirection().x,lightManager->GetLightDirection().y,lightManager->GetLightDirection().z });
        DebugRender::Render(immediateContext);
        RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::WIREFRAME_CULL_NONE);
        DebugRender::WiredRender(immediateContext);
        ExecuteHooks(RenderPass::Debug, immediateContext);
    }
#endif
    RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_BACK);
    multipleRenderTargets->Deactivate(immediateContext);


    DirectX::XMFLOAT4X4 cameraView;
    DirectX::XMFLOAT4X4 cameraProjection;

    if (camera)
    {
        ViewConstants data = camera->GetViewConstants();
        cameraView = data.view;
        cameraProjection = data.projection;
    }
    // カスケードシャドウマップ生成
    cascadedShadowMaps->Clear(immediateContext);
    auto& shadow = Scene::GetCurrentScene()->GetSceneSettings().cascadedShadowMapConstants;
    cascadedShadowMaps->Activate(immediateContext, cameraView, cameraProjection, lightManager->GetLightDirection(), shadow.criticalDepthValue, 3/*cbSlot*/);
    RenderState::BindBlendState(immediateContext, BLEND_STATE::NONE);
    RenderState::BindDepthStencilState(immediateContext, DEPTH_STATE::ZT_ON_ZW_ON);
    RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_NONE);
    sceneRender.currentRenderPath = RenderPath::Shadow;
    sceneRender.CastShadowRender(immediateContext, queues.shadowCasters);
    cascadedShadowMaps->Deactivate(immediateContext);

    // ファイナルパス
    {
        RenderState::BindBlendState(immediateContext, BLEND_STATE::NONE);
        RenderState::BindDepthStencilState(immediateContext, DEPTH_STATE::ZT_OFF_ZW_OFF);
        RenderState::BindRasterizerState(immediateContext, RASTERIZE_STATE::SOLID_CULL_NONE);

        sceneEffectManager->ApplyAll(immediateContext, multipleRenderTargets->renderTargetShaderResourceViews[static_cast<int>(M_SRV_SLOT::COLOR)], multipleRenderTargets->renderTargetShaderResourceViews[static_cast<int>(M_SRV_SLOT::NORMAL)],
            multipleRenderTargets->depthStencilShaderResourceView, multipleRenderTargets->renderTargetShaderResourceViews[static_cast<int>(M_SRV_SLOT::POSITION)], nullptr/*ディファードの時に使用するmaterial の値*/, cascadedShadowMaps->depthMap().Get());


        ID3D11ShaderResourceView* shader_resource_views[]
        {
            multipleRenderTargets->renderTargetShaderResourceViews[static_cast<int>(M_SRV_SLOT::COLOR)],
            multipleRenderTargets->renderTargetShaderResourceViews[static_cast<int>(M_SRV_SLOT::POSITION)],
            multipleRenderTargets->renderTargetShaderResourceViews[static_cast<int>(M_SRV_SLOT::NORMAL)],
            multipleRenderTargets->depthStencilShaderResourceView,
            sceneEffectManager->GetOutput("BloomEffect"),
            sceneEffectManager->GetOutput("FogEffect"),
            sceneEffectManager->GetOutput("SSAOEffect"),
            sceneEffectManager->GetOutput("SSREffect"),
            cascadedShadowMaps->depthMap().Get(),
        };
        fullscreenQuad->Blit(immediateContext, shader_resource_views, 0, _countof(shader_resource_views), postEffectPs.Get());
    }
#ifdef USE_IMGUI
    imGuiGizmoBuffer->Deactivate(immediateContext);
#endif
}

void SampleScene::SetUpActors()
{
    auto mainCameraActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<MainCamera>("mainCameraActor");

    Transform targetTr(DirectX::XMFLOAT3{ -0.0f,0.0f,0.0f }, DirectX::XMFLOAT3{ 0.0f,180.0f,0.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    auto targetActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<Actor>("target", targetTr);

    mainCameraActor->SetTarget(targetActor->GetRootComponent());
    mainCameraActor->GetCameraComponent()->SetYaw(DirectX::XMConvertToRadians(90.0f));
    SetActiveCamera(mainCameraActor);
    Logger::Log(U8("SampleSceneのカメラ設定される。"));

    Transform stageTr(DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    planeActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<PlaneActor>("PlaneActor", stageTr);

    Transform backPlaneTr(DirectX::XMFLOAT3{ -3.0f,0.0f,0.0f }, DirectX::XMFLOAT3{ 0.0f,0.0f,90.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    backPlaneActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<PlaneActor>("backPlaneActor", backPlaneTr);
    backPlaneActor->SetActive(false);
    Transform frontPlaneTr(DirectX::XMFLOAT3{ 1.3f,0.0f,0.0f }, DirectX::XMFLOAT3{ 0.0f,0.0f,90.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    frontPlaneActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<PlaneActor>("frontPlaneActor", frontPlaneTr);
    frontPlaneActor->SetActive(false);

    Transform debugCameraTr(DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    auto debugCameraActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<DebugCamera>("debugCam", debugCameraTr);

    auto pauseActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<Pause>("pauseActor");
#if 1

    Transform sphereTr(DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    sphereActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<SphereActor>("sphereActor", sphereTr);
    sphereActor1 = this->GetActorManager()->CreateAndRegisterActorWithTransform<SphereActor>("sphereActor1", sphereTr);

#endif // 0

    Transform cubeTr(DirectX::XMFLOAT3{ 2.2f,0.4f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    cubeActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<CubeActor>("cubeActor", cubeTr);
    Transform cubeTr1(DirectX::XMFLOAT3{ -2.3f,0.4f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    cubeActor1 = this->GetActorManager()->CreateAndRegisterActorWithTransform<CubeActor>("cubeActor1", cubeTr1);

    auto actor = GetActorManager()->CreateAndRegisterActorWithTransform<Actor>("stage", stageTr);
    auto mesh = actor->AddComponent<SkeletalMeshComponent>("staticMeshComponent");
    mesh->SetModel("./Data/Models/BeltConveyor/scene.gltf");
    //mesh->SetModel("./Data/Models/Stage/Stage.gltf");
    auto boxComponent = actor->AddComponent<class BoxComponent>("boxComponent", "staticMeshComponent");
    boxComponent->SetHalfBoxExtent(DirectX::XMFLOAT3(80.0f, 0.2f, 80.0f));
    boxComponent->SetRelativeLocationDirect({ 0.0f,-0.1f,0.0f });
    boxComponent->SetStatic(true);
    boxComponent->SetLayer(CollisionLayer::Floor);
    boxComponent->SetResponseToLayer(CollisionLayer::CarWheel, CollisionComponent::CollisionResponse::Block);
    boxComponent->SetResponseToLayer(CollisionLayer::Car, CollisionComponent::CollisionResponse::Block);
    boxComponent->Initialize();


#if 0
    Transform gearTr(DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f }, DirectX::XMFLOAT4{ 0.0f,0.0f,0.0f,1.0f }, DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f });
    gearActor = this->GetActorManager()->CreateAndRegisterActorWithTransform<GearActor>("GearActor", gearTr);
#endif
    cameraManager->SetDebugCamera(debugCameraActor);
}

void SampleScene::HandleInput(float deltaTime)
{
    // PBD
#if 1
    auto& body = world.get_shape_matching_body(0);
    body.constrain_rotation_to_y = constrain_rotation_to_y;
    body.scale = bodyScale;
    body.deformation_blend = deformationBlend;
    body.stiffness = stiffness;

    DirectX::XMVECTOR forward = body.transform.r[2];
    DirectX::XMVECTOR axis = body.transform.r[1];
    float speed = 2.0f;
    if (enable_simulation && GetKeyState(VK_UP) & 0x8000)
    {
        body.translate(world.particles, DirectX::XMVectorScale(forward, deltaTime * speed));
    }
    if (enable_simulation && GetKeyState(VK_DOWN) & 0x8000)
    {
        body.translate(world.particles, DirectX::XMVectorScale(forward, -deltaTime * speed));
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
    if (enable_simulation && GetKeyState('I') & 0x8000)
    {
        DirectX::XMVECTOR rotation = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, -XMConvertToRadians(45.0f) * deltaTime);
        body.rotate(world.particles, rotation, body.compute_center_of_mass(world.particles));
    }
    if (enable_simulation && GetKeyState('K') & 0x8000)
    {
        DirectX::XMVECTOR rotation = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, XMConvertToRadians(45.0f) * deltaTime);
        body.rotate(world.particles, rotation, body.compute_center_of_mass(world.particles));
    }
    if (enable_simulation && GetKeyState('J') & 0x8000)
    {
        DirectX::XMVECTOR rotation = DirectX::XMQuaternionRotationRollPitchYaw(-XMConvertToRadians(45.0f) * deltaTime, 0, 0);
        body.rotate(world.particles, rotation, body.compute_center_of_mass(world.particles));
    }
    if (enable_simulation && GetKeyState('L') & 0x8000)
    {
        DirectX::XMVECTOR rotation = DirectX::XMQuaternionRotationRollPitchYaw(XMConvertToRadians(45.0f) * deltaTime, 0, 0);
        body.rotate(world.particles, rotation, body.compute_center_of_mass(world.particles));
    }
#else

    // 剛体の forward を計算
    XMVECTOR q = XMLoadFloat4(&carActor->GetRigidBody().rotation);
    XMVECTOR forward = XMVector3Rotate(
        DirectX::XMVectorSet(0, 0, 1, 0), q);

    // 地面法線（Y軸）
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    // forward を地面に投影
    XMVECTOR forward_flat = XMVector3Normalize(
        XMVectorSubtract(forward, XMVectorScale(up, XMVectorGetX(XMVector3Dot(forward, up))))
    );

    XMVECTOR v = XMLoadFloat3(&carActor->GetRigidBody().linearVelocity);

    // 前進・後退
    if (enable_simulation && GetKeyState(VK_UP) & 0x8000)
    {
        v = XMVectorAdd(v, XMVectorScale(forward_flat, 8.0f * deltaTime));
        XMStoreFloat3(&carActor->GetRigidBody().linearVelocity, v);
    }
    if (enable_simulation && GetKeyState(VK_DOWN) & 0x8000)
    {
        v = XMVectorAdd(v, XMVectorScale(forward_flat, -8.0f * deltaTime));
        XMStoreFloat3(&carActor->GetRigidBody().linearVelocity, v);
    }


    if (enable_simulation && GetKeyState(VK_LEFT) & 0x8000)
    {
        // Y軸回転トルク
        auto& rb = carActor->GetRigidBody();
        rb.angularVelocity.y += 1.0f * deltaTime;
    }
    if (enable_simulation && GetKeyState(VK_RIGHT) & 0x8000)
    {
        auto& rb = carActor->GetRigidBody();
        rb.angularVelocity.y -= 1.0f * deltaTime;
    }
    if (enable_simulation && GetKeyState('I') & 0x8000)
    {
        auto& rb = carActor->GetRigidBody();
        rb.angularVelocity.x += 8.0f * deltaTime;
    }
    if (enable_simulation && GetKeyState('K') & 0x8000)
    {
        auto& rb = carActor->GetRigidBody();
        rb.angularVelocity.x -= 8.0f * deltaTime;
    }
#endif // 0

}

bool SampleScene::Uninitialize(ID3D11Device* device)
{
    SceneBase::Uninitialize(device);
    Physics::Instance().Finalize();
    return true;
}

void SampleScene::DrawGui()
{
#ifdef USE_IMGUI

    SceneBase::DrawGui();
    ImGui::Begin("pbd");
    ImGui::Checkbox("show_particles", &show_particles);
    ImGui::Checkbox("enable_simulation", &enable_simulation);
    ImGui::Checkbox("show_wireframe", &show_wireframe);
    ImGui::Checkbox("constrain_rotation_to_y", &constrain_rotation_to_y);
    ImGui::DragFloat("scale", &bodyScale, 0.01f, 0.01f, 10.0f, "%.4f");
    ImGui::DragFloat("stiffness", &stiffness, 0.001f, 0.0001f, 1.0f, "%.4f");
    ImGui::DragFloat("deformation_blend", &deformationBlend, 0.001f, 0.0f, 1.0f, "%.4f");
    ImGui::SliderInt("solver_iterations", &solver->solver_iterations, 0, 30);

    ImGui::DragFloat3("hitPosition", &damageConstantBuffer->data.hitPosition.x, 0.1f);
    ImGui::DragFloat3("hitNormal", &damageConstantBuffer->data.hitNormal.x, 0.1f);
    ImGui::DragFloat("radius", &damageConstantBuffer->data.radius, 0.1f);
    ImGui::DragFloat("strength", &damageConstantBuffer->data.strength, 0.1f);


    auto& plane = world.get_collision_shape(backPlaneIndex);
    if (auto* planeShape = dynamic_cast<PBD::plane_shape*>(&plane))
    {
        ImGui::DragFloat3("backPlaneNormal", &planeShape->normal.x, 0.01f);
    }
    auto& frontPlane = world.get_collision_shape(frontPlaneIndex);
    if (auto* planeShape = dynamic_cast<PBD::plane_shape*>(&frontPlane))
    {
        ImGui::DragFloat3("frontPlaneNormal", &planeShape->normal.x, 0.01f);
    }



    // PBD
    auto& body = world.get_shape_matching_body(0);
    if (ImGui::Button("Reset"))
    {
        body.reset_to_rest_state(world.particles);
        body.set_position(world.particles, { 0, 2, 0 });
        //body.rigid_position = { 0,2,0 };
        body.rigid_rotation_quat = { 0,0,0,1 };
        body.scale = 1.0f;
    }

    ImGui::DragFloat3("center", &center.x, 0.1f);
    ImGui::DragFloat3("angularVelocity", &angularVelocity.x, 0.1f);
    ImGui::DragFloat("radius", &radius, 0.1f, 0.0f, 5.0f);
    ImGui::DragFloat3("center1", &center1.x, 0.1f);
    ImGui::DragFloat3("angularVelocity1", &angularVelocity1.x, 0.1f);
    ImGui::DragFloat("radius1", &radius1, 0.1f, 0.0f, 5.0f);
#if 0
    if (ImGui::Button("Button"))
    {
        body.set_position(world.particles, { 0, 2, 0 });
    }
#endif // 1
    ImGui::End();
#endif
}
