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

    DirectX::XMFLOAT3 extent = { 1.0f,1.0f,1.0f };
private:

};