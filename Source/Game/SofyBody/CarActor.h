#pragma once
#include "Core/Actor.h"
#include "Components/Render/MeshComponent.h"
#include "Components/CollisionShape/ShapeComponent.h"
#include "Components/Controller/ControllerComponent.h"
#include "PBD/RigidBody.h"


class CarActor :public Actor
{
public:
    CarActor(const std::string& actorName) :Actor(actorName) {}

    void Initialize(const Transform& transform)override;

    void Update(float elapsedTime)override;

    void DrawImGuiDetails() override;

    PBD::RigidBody& GetRigidBody() { return rigid; }

    int shapeMatchingBodyIndex = -1;

private:
    PBD::RigidBody rigid;

    std::shared_ptr<BoxComponent> boxComponent;
    std::shared_ptr<InputComponent> inputComponent;

    std::shared_ptr<SkeletalMeshComponent> wheelMeshComponents[4];
    std::shared_ptr<SphereComponent> wheelSphereComponents[4];

    bool wheelGround[4]; // 毎フレーム false にリセット
    bool onGround;       // まとめた結果

    std::shared_ptr<SkeletalMeshComponent> frontPivotBottomComponent;
    std::shared_ptr<SkeletalMeshComponent> frontPivotTopComponent;
    std::shared_ptr<SkeletalMeshComponent> backPivotBottomComponent;
    std::shared_ptr<SkeletalMeshComponent> backPivotTopComponent;

    std::shared_ptr<SkeletalMeshComponent> topCenterComponent;  // 上面中心
    std::shared_ptr<SkeletalMeshComponent> bottomCenterComponent;   // 底面中心
    std::shared_ptr<SkeletalMeshComponent> frontCenterComponent;    // 前面中心
    std::shared_ptr<SkeletalMeshComponent> backCenterComponent;     // 後面中心
    bool rolling = false;

    float rollingAngle = 0.0f;

    // 1秒で180°回るくらい
    float rollingSpeed = DirectX::XM_PI;
    int rollingDirection = 1;
    DirectX::XMFLOAT3 rollingPivot;
};
