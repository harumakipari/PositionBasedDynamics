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
    void UpdateRollingPivot(bool toForward);

    // 面を更新する
    void AdvanceCurrentFace(bool toForward);

    // 差分の角度で位置を更新する
    void ApplyRollingDelta(float deltaAngle);

    void StartRoll(bool toForward);
    void StartAutoRoll(bool toForward);
    void ApplyRollDelta(float deltaAngle);
    void EndRoll(bool completed);


    void ApplyRollDeltaImmediate(float deltaAngle);
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

    float rollingAngle = 0.0f;      // 今の面から何度倒れているか radian
    float previousRollingAngle = 0.0f;  // 前のフレームの角度
    bool rolling = false;

    DirectX::XMFLOAT3 rollingPivot;
    DirectX::XMFLOAT4 baseRotation = { 0,0,0,1 };

    float rollingAngularVelocity = 0.0f;

    float rollTargetAngle = 0.0f;
    bool rollToForward = true;
    bool continueRollThroughSide = false;
};
