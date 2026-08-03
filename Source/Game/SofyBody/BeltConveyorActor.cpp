#include "pch.h"
#include "BeltConveyorActor.h"

void BeltConveyorActor::Initialize(const Transform& transform)
{
    std::string parentName = "BeltConveyorActor";
    auto meshComponent= this->AddComponent<StaticMeshComponent>(parentName);
    meshComponent->SetModel("./Data/Models/BeltConveyor/scene.gltf", true);
    SetEulerRotation(DirectX::XMFLOAT3(0.0f, 180.0f, 0.0f));
    meshComponent->SetRelativeScaleDirect(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f));
    meshComponent->SetRelativeLocationDirect(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void BeltConveyorActor::Update(float elapsedTime)
{

}

void BeltConveyorActor::DrawImGuiDetails()
{
    
}