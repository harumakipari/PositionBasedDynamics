#pragma once
#include "Core/Actor.h"
#include "Components/Render/MeshComponent.h"
#include "Components/CollisionShape/ShapeComponent.h"
#include "Components/Controller/ControllerComponent.h"
#include "PBD/RigidBody.h"

class GearActor :public Actor
{
public:
    GearActor(const std::string& actorName) :Actor(actorName) {}

    void Initialize(const Transform& transform)override;

    void Update(float elapsedTime)override;

    void DrawImGuiDetails() override;

    std::vector<int> GetPbdBoxIndices() { return pbdBoxIndices; }
private:
    std::shared_ptr<SkeletalMeshComponent> skeletalMeshComponent;

    std::vector<BoxComponent*> boxComponents;
    std::vector<int> pbdBoxIndices;
};
