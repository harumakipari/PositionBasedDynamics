#pragma once
#include "Core/Actor.h"
#include "Components/Render/MeshComponent.h"
#include "Components/CollisionShape/ShapeComponent.h"


class CubeActor :public Actor
{
public:
    CubeActor(const std::string& actorName) :Actor(actorName) {}

    void Initialize(const Transform& transform)override;

    void Update(float elapsedTime)override;

    void DrawImGuiDetails() override;

    DirectX::XMFLOAT3X3 GetRotationMatrix3X3();

    DirectX::XMFLOAT3 extent = { 3.0f,1.0f,3.0f };
private:
    float viewScale = 0.8f;
};