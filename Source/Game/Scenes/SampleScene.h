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

class SampleScene : public SceneBase
{
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

private:
    std::shared_ptr<Stage>  title;

    std::unique_ptr<MorphModel> morphModel;



    // PBD
    PBD::PBDWorld world;
    std::unique_ptr<PBD::Solver> solver;

    std::vector<std::unique_ptr<deformable_model>> deformable_models;

    bool enable_simulation = false;
    float time_accumulator = 0.0f;
    const float physics_time_step = 1.0f / 30.0f;

    bool show_particles = false;
    bool show_wireframe = false;

    std::vector<PBDActor> pbdActors;

    std::unique_ptr<particle_debug_renderer> particle_debug_renderer;


    DirectX::XMFLOAT3 center = { 0.0f,0.0f,0.0f };
    float radius = 1.0f;

};
