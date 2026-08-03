#pragma once

#include "Engine/Scene/Scene.h"

#include <d3d11.h>
#include <wrl.h>
#include <memory>

#include "Core/ActorManager.h"
#include "Engine/Scene/SceneBase.h"


#include "Graphics/Renderer/SceneRenderer.h"

#include "Game/Actors/Camera/LoadingCamera.h"
#include "Game/Actors/Stage/ClothSimulate.h"
#include "Game/Actors/Stage/Stage.h"
#include "Game/Actors/WaterSphere/MorphModel.h"

#include "UI/Widgets/Widget.h"

#include "PBD/PositionBasedDynamics.h"
#include "PBD/PbdParticleDebugRenderer.h"
#include "PBD/PBDActor.h"

class PlaneActor;
class GearActor;
class CubeActor;
class SphereActor;
class CarActor;

class SampleScene : public SceneBase
{
    struct DamageConstants
    {
        DirectX::XMFLOAT3 hitPosition{};
        float radius = 1.0f;
        DirectX::XMFLOAT3 hitNormal{};
        float strength = 1.0f;
    };

public:
    bool Initialize(ID3D11Device* device, UINT64 width, UINT height, const std::unordered_map<std::string, std::string>& props) override;

    void Start() override;

    void Update(float deltaTime) override;

    bool Uninitialize(ID3D11Device* device) override;

    void Render(ID3D11DeviceContext* immediateContext, float deltaTime) override;

    void DrawGui() override;

    void SetUpActors()override;

    //ÉVÅ[ÉìÇÃé©ìÆìoò^
    static inline Scene::Autoenrollment<SampleScene> _autoenrollment;

    // PBDWorldÇéÊìæÇ∑ÇÈä÷êî
    PBD::PBDWorld& GetPbdWorld() { return world; }
private:
    void HandleInput(float deltaTime);

private:
    std::shared_ptr<ConstantBuffer<DamageConstants>> damageConstantBuffer;

    std::shared_ptr<Stage>  title;
    std::unique_ptr<MorphModel> morphModel;

    // PBD
    PBD::PBDWorld world;
    std::unique_ptr<PBD::Solver> solver;

    std::vector<std::unique_ptr<deformable_model>> deformable_models;

    bool enable_simulation = true;
    float time_accumulator = 0.0f;
    const float physics_time_step = 1.0f / 30.0f;

    bool show_particles = false;
    bool show_wireframe = false;

    std::vector<PBDActor> pbdActors;

    std::unique_ptr<particle_debug_renderer> particle_debug_renderer;

    bool constrain_rotation_to_y = true;
    float bodyScale = 1.95f;
    float stiffness = 0.1f;
    float deformationBlend = 0.2f;

    DirectX::XMFLOAT3 center = { 0.0f,1.6f,-5.8f };
    DirectX::XMFLOAT3 angularVelocity = { 0.0f,0.0f,0.0f };    // äpë¨ìx
    float radius = 1.0f;
    std::shared_ptr<SphereActor> sphereActor;
    int sphereIndex = 0;

    std::shared_ptr<SphereActor> sphereActor1;
    DirectX::XMFLOAT3 center1 = { 0.0f,0.1f,-3.9f };
    DirectX::XMFLOAT3 angularVelocity1 = { 0.0f,0.0f,0.0f };    // äpë¨ìx
    float radius1 = 1.0f;
    int sphereIndex1 = 0;

    std::shared_ptr<CubeActor> cubeActor;
    std::shared_ptr<CubeActor> cubeActor1;
    int boxIndex = 0;
    int boxIndex1 = 0;
    std::vector<int> boxIndices;

    std::shared_ptr<CarActor> carActor;
    int shapeMatchingBodyIndex = 0;

    int planeIndex = 0;
    int frontPlaneIndex = 0;
    int backPlaneIndex = 0;

    std::shared_ptr<GearActor> gearActor;

    std::shared_ptr<PlaneActor> planeActor;
    std::shared_ptr<PlaneActor> frontPlaneActor;
    std::shared_ptr<PlaneActor> backPlaneActor;

    std::shared_ptr<CinemaCamera> cinemaCameraActor;




};
