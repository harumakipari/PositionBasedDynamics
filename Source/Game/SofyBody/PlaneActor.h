#pragma once
#include "Core/Actor.h"
#include "Components/Render/MeshComponent.h"
#include "Components/CollisionShape/ShapeComponent.h"


class PlaneActor :public Actor
{
public:
    PlaneActor(const std::string& actorName) :Actor(actorName){}

    void Initialize(const Transform& transform)override
    {
        std::shared_ptr<StaticMeshComponent> staticMeshComponent = this->AddComponent<StaticMeshComponent>("staticMeshComponent");
        staticMeshComponent->SetModel("./Data/Models/Primitives/Plane.glb", false,false);
        staticMeshComponent->SetIsVisible(false);
    }

    void Update(float elapsedTime)override {}
};