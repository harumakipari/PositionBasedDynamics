#include "pch.h"
#include "CubeActor.h"

void CubeActor::Initialize(const Transform& transform)
{
    std::string parentName = "CubeActorRootComponent";
    std::shared_ptr<StaticMeshComponent> staticMeshComponent = this->AddComponent<StaticMeshComponent>(parentName);
    staticMeshComponent->SetModel("./Data/Models/cube.glb", false, false);
    staticMeshComponent->SetIsVisible(false);
    std::shared_ptr<StaticMeshComponent> pressMeshComponent = this->AddComponent<StaticMeshComponent>("pressMesh", parentName);
    pressMeshComponent->SetModel("./Data/Models/BeltConveyor/press.glb", false, false);
    pressMeshComponent->SetRelativeScaleDirect({ 0.2f,0.5f,0.2f });
    pressMeshComponent->SetRelativeLocationDirect({ 0.0f,-0.55f,0.0f });
}

void CubeActor::Update(float elapsedTime)
{
    DirectX::XMFLOAT3 viewExtent = MathHelper::Multiply(extent, viewScale);
    DirectX::XMFLOAT3 pos = GetPosition();
    pos.x = -0.9f;
    pos.z = -0.4f;

    // 7.6f -> 4.3f
    switch (pressState)
    {
    case PressState::Idle:
        break;

    case PressState::MovingDown:
    {
        pos.y -= pressSpeed * elapsedTime;

        if (pos.y <= bottomY)
        {
            pos.y = bottomY;

            waitTimer = 0.0f;
            pressState = PressState::BottomWait;
        }

        break;
    }

    case PressState::BottomWait:
    {
        waitTimer += elapsedTime;

        if (waitTimer >= bottomWaitTime)
        {
            waitTimer = 0.0f;
            pressState = PressState::MovingUp;
        }

        break;
    }

    case PressState::MovingUp:
    {
        pos.y += returnSpeed * elapsedTime;

        if (pos.y >= topY)
        {
            pos.y = topY;

            waitTimer = 0.0f;
            pressState = PressState::TopWait;
        }

        break;
    }

    case PressState::TopWait:
    {
        waitTimer += elapsedTime;

        if (waitTimer >= topWaitTime)
        {
            waitTimer = 0.0f;

            if (repeatPress)
            {
                pressState = PressState::MovingDown;
            }
            else
            {
                pressState = PressState::Idle;
            }
        }

        break;
    }
    }

    SetPosition(pos);
}

// ’×‚·
void CubeActor::Press()
{
    if (pressState != PressState::Idle)
        return;
    pressState = PressState::MovingDown;
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