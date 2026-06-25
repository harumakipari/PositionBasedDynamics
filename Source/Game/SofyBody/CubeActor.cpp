#include "pch.h"
#include "CubeActor.h"

void CubeActor::Initialize(const Transform& transform)
{
    std::shared_ptr<StaticMeshComponent> staticMeshComponent = this->AddComponent<StaticMeshComponent>("staticMeshComponent");
    staticMeshComponent->SetModel("./Data/Models/cube.glb", false, false);
}

void CubeActor::Update(float elapsedTime)
{
    SetScale(extent);
}

void CubeActor::DrawImGuiDetails()
{
#ifdef USE_IMGUI
    ImGui::DragFloat3("extent", &extent.x, 0.5f, 0.1f, 3.0f);
#endif
}