#include "pch.h"
#include "GearActor.h"

#include "Engine/Scene/Scene.h"
#include "Game/Scenes/SampleScene.h"
#include "PBD/PbdWorld.h"
#include "PBD/PbdCollision.h"

void GearActor::Initialize(const Transform& transform)
{
    std::string parentName = "GearActor";
    skeletalMeshComponent = AddComponent<SkeletalMeshComponent>(parentName);
    skeletalMeshComponent->SetModel("./Data/Models/Gear.glb");
    skeletalMeshComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    skeletalMeshComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    skeletalMeshComponent->overrideDeferredPipelineName = "GltfMorphModelPS";

    //DirectX::XMFLOAT3 size = { 3.6f,0.5f,3.5f };
    DirectX::XMFLOAT3 size = { 0.6f,0.5f,0.5f };
    {
        auto boxComponent = AddComponent<BoxComponent>("boxComponent", parentName);
        boxComponent->SetBoxExtent(size);
        boxComponent->SetLayer(CollisionLayer::Gear);
        boxComponent->SetResponseToLayer(CollisionLayer::Car, CollisionComponent::CollisionResponse::Block);
        boxComponent->SetRelativeLocationDirect({ 0.9f,0.3f,0.0f });
        boxComponent->SetRelativeEulerRotationDirect({ 0.0f,0.0f,113.0f });
        boxComponent->Initialize();
        boxComponents.push_back(boxComponent.get());
    }
    {
        auto boxComponent = AddComponent<BoxComponent>("boxComponent", parentName);
        boxComponent->SetBoxExtent(size);
        boxComponent->SetLayer(CollisionLayer::Gear);
        boxComponent->SetResponseToLayer(CollisionLayer::Car, CollisionComponent::CollisionResponse::Block);
        boxComponent->SetRelativeLocationDirect({ 0.6f,-0.8f,0.0f });
        boxComponent->SetRelativeEulerRotationDirect({ 0.0f,0.0f,38.0f });
        boxComponent->Initialize();
        boxComponents.push_back(boxComponent.get());
    }
#if 1
    auto scene = GetOwnerScene();
    auto sampleScene = dynamic_cast<SampleScene*>(scene);
    auto& world = sampleScene->GetPbdWorld();
    for (auto box : boxComponents)
    {
        int boxIndex= world.spawn_collision_shape<PBD::box_shape>(DirectX::XMFLOAT3{ -1.5f,-1.5f,-1.5f }, DirectX::XMFLOAT3{ 1.5f,1.5f,1.5f }, 0x0001/*phase*/);
        pbdBoxIndices.push_back(boxIndex);
    }
#endif
}

void GearActor::Update(float elapsedTime)
{
#if 1
    auto scene = GetOwnerScene();
    auto sampleScene = dynamic_cast<SampleScene*>(scene);
    auto& world = sampleScene->GetPbdWorld();

    for (size_t i = 0; i < boxComponents.size(); i++)
    {
        auto* boxComp = boxComponents[i];

        auto& shape = world.get_collision_shape(pbdBoxIndices[i]);

        auto* box = dynamic_cast<PBD::box_shape*>(&shape);
        if (!box)
            continue;

        box->center = boxComp->GetComponentLocation();
        box->rotation = boxComp->GetRotationMatrix3X3();
        box->extent = boxComp->GetBoxExtent();
    }
#endif // 0

}

void GearActor::DrawImGuiDetails()
{

}
