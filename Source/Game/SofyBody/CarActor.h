#pragma once
#include "Core/Actor.h"
#include "Components/Render/MeshComponent.h"
#include "Components/CollisionShape/ShapeComponent.h"
#include "Components/Controller/ControllerComponent.h"
#include "PBD/RigidBody.h"


class CarActor :public Actor
{
    enum class Face :uint8_t
    {
        Bottom,
        Front,
        Top,
        Back
    };

    Face currentFace = Face::Bottom;

    enum  RollDirection
    {
        Forward = 1,
        Backward = -1
    };
    RollDirection rollingDirection;

    enum class RollState :uint8_t
    {
        None,       // 回転していない
        Input,      // 押している間
        Return,     // 元に戻る
        Finish      // 90°まで倒れる
    };
    RollState rollState = RollState::None;
public:
    CarActor(const std::string& actorName) :Actor(actorName) {}

    void Initialize(const Transform& transform)override;

    void Update(float elapsedTime)override;

    void DrawImGuiDetails() override;

    PBD::RigidBody& GetRigidBody() { return rigid; }

    int shapeMatchingBodyIndex = -1;

private:
    // 基準点を更新する
    void UpdateRollingPivot();

    // 面を更新する
    void AdvanceCurrentFace();

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

    float rollingAngle = 0.0f;
    float rollingAngularVelocity = 0.0f;  // 角速度
    // 次の面へ行くか戻るか決まったか
    bool rollingCommitted = false;

    float rollingAngularAccel = 4.0f;     // 入力による角加速度
    float rollingFriction = 6.0f;         // 減衰
    // 1秒で180°回るくらい
    float rollingSpeed = DirectX::XM_PI;

    bool rolling = false;
    DirectX::XMFLOAT3 rollingPivot;// 今回の支点
    float rollingProgress = 0.0f;      // 0～90°
    DirectX::XMFLOAT3 rollingStartPosition;
    DirectX::XMFLOAT4 rollingStartRotation;
};
