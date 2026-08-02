#include "pch.h"
#include "CubeActor.h"

void CubeActor::Initialize(const Transform& transform)
{
    std::shared_ptr<StaticMeshComponent> staticMeshComponent = this->AddComponent<StaticMeshComponent>("staticMeshComponent");
    staticMeshComponent->SetModel("./Data/Models/cube.glb", false, false);
}

void CubeActor::Update(float elapsedTime)
{
    DirectX::XMFLOAT3 viewExtent = MathHelper::Multiply(extent, viewScale);
    SetScale(viewExtent);

}

void CubeActor::DrawImGuiDetails()
{
#ifdef USE_IMGUI
    ImGui::DragFloat("viewScale", &viewScale, 0.05f, 0.1f, 1.0f);
    ImGui::DragFloat3("extent", &extent.x, 0.5f, 0.1f, 3.0f);
#endif
}

DirectX::XMFLOAT3X3 CubeActor::GetRotationMatrix3X3()
{
    DirectX::XMFLOAT4 rotation = GetQuaternionRotation();

    DirectX::XMVECTOR q = DirectX::XMLoadFloat4(&rotation);

    DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(q);

    DirectX::XMFLOAT3X3 result;
    DirectX::XMStoreFloat3x3(&result, rotMat);

    return result;
}