#pragma once
#include "Core/Actor.h"
#include "Components/Render/MeshComponent.h"
#include "Components/CollisionShape/ShapeComponent.h"


class BeltConveyorActor :public Actor
{
public:
    BeltConveyorActor(const std::string& modelName) :Actor(modelName) {}

    void Initialize(const Transform& transform)override;

    void Update(float elapsedTime)override;

    void DrawImGuiDetails() override;

private:
    std::shared_ptr<SkeletalMeshComponent> skeletalMeshComponent;
};