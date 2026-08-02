#include "pch.h"
#include "SphereActor.h"


void SphereActor::Initialize(const Transform& transform)
{
    std::shared_ptr<StaticMeshComponent> staticMeshComponent = this->AddComponent<StaticMeshComponent>("staticMeshComponent");
    //staticMeshComponent->SetModel("./Data/Models/ball.glb", false, false);
    staticMeshComponent->SetModel("./Data/Models/Car/red.glb", false, false);
}

void SphereActor::DrawImGuiDetails()
{
#ifdef USE_IMGUI
    //ImGui::DragFloat3("angularSpeed",)
#endif
}