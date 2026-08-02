#include "pch.h"
#include "CarActor.h"

#include <magic_enum.hpp>

void CarActor::Initialize(const Transform& transform)
{
    rigid.position = transform.GetLocation();
    rigid.rotation = transform.GetRotation();
    rigid.mass = 1.0f;
    rigid.inverseMass = 1.0f / rigid.mass;
    rigid.inertia = 1.0f;
    rigid.inverseInertia = 1.0f / rigid.inertia;

    std::string parentName = "SkeletalMeshComponent";

    auto meshComponent = AddComponent<SkeletalMeshComponent>(parentName);
    meshComponent->SetModel("./Data/Models/Car/car.glb");
    meshComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    meshComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    meshComponent->overrideDeferredPipelineName = "GltfMorphModelPS";
    meshComponent->SetIsVisible(false);

    DirectX::XMFLOAT3 size = meshComponent->GetModelSize();

    boxComponent = AddComponent<BoxComponent>("BoxComponent", parentName);
    boxComponent->SetBoxExtent(size);
    boxComponent->SetLayer(CollisionLayer::Car);
    boxComponent->SetResponseToLayer(CollisionLayer::Floor, CollisionComponent::CollisionResponse::Block);
    boxComponent->SetCollisionOffsetY(size.y * 0.5f);
    boxComponent->Initialize();


    // 回転の基準点を追加
    // 前面の下
    frontPivotBottomComponent = AddComponent<SkeletalMeshComponent>("frontPivotBottomComponent", parentName);
    frontPivotBottomComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    frontPivotBottomComponent->SetRelativeLocationDirect({ 0.0f,0.0f,size.z * 0.5f });
    frontPivotBottomComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    frontPivotBottomComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    frontPivotBottomComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    frontPivotBottomComponent->overrideDeferredPipelineName = "GltfMorphModelPS";
    frontPivotBottomComponent->SetIsVisible(false);

    // 後面の下
    backPivotBottomComponent = AddComponent<SkeletalMeshComponent>("backPivotBottonComponent", parentName);
    backPivotBottomComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    backPivotBottomComponent->SetRelativeLocationDirect({ 0.0f,0.0f,-size.z * 0.5f });
    backPivotBottomComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    backPivotBottomComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    backPivotBottomComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    backPivotBottomComponent->overrideDeferredPipelineName = "GltfMorphModelPS";
    backPivotBottomComponent->SetIsVisible(false);

    // 前面の上
    frontPivotTopComponent = AddComponent<SkeletalMeshComponent>("frontPivotTopComponent", parentName);
    frontPivotTopComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    frontPivotTopComponent->SetRelativeLocationDirect({ 0.0f,size.y,size.z * 0.5f });
    frontPivotTopComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    frontPivotTopComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    frontPivotTopComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    frontPivotTopComponent->overrideDeferredPipelineName = "GltfMorphModelPS";
    frontPivotTopComponent->SetIsVisible(false);



    // 後面の上
    backPivotTopComponent = AddComponent<SkeletalMeshComponent>("backPivotTopComponent", parentName);
    backPivotTopComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    backPivotTopComponent->SetRelativeLocationDirect({ 0.0f,size.y,-size.z * 0.5f });
    backPivotTopComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    backPivotTopComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    backPivotTopComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    backPivotTopComponent->overrideDeferredPipelineName = "GltfMorphModelPS";
    backPivotTopComponent->SetIsVisible(false);
#if 0

    // 上面の中心
    topCenterComponent = AddComponent<SkeletalMeshComponent>("topCenterComponent", parentName);
    topCenterComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    topCenterComponent->SetRelativeLocationDirect({ 0.0f,size.y,0.0f });
    topCenterComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    topCenterComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    topCenterComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    topCenterComponent->overrideDeferredPipelineName = "GltfMorphModelPS";

    // 底面の中心
    bottomCenterComponent = AddComponent<SkeletalMeshComponent>("bottomCenterComponent", parentName);
    bottomCenterComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    bottomCenterComponent->SetRelativeLocationDirect({ 0.0f,0.0f,0.0f });
    bottomCenterComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    bottomCenterComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    bottomCenterComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    bottomCenterComponent->overrideDeferredPipelineName = "GltfMorphModelPS";

    // 前面の中心
    frontCenterComponent = AddComponent<SkeletalMeshComponent>("frontCenterComponent", parentName);
    frontCenterComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    frontCenterComponent->SetRelativeLocationDirect({ 0.0f,size.y * 0.5f,size.z * 0.5f });
    frontCenterComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    frontCenterComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    frontCenterComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    frontCenterComponent->overrideDeferredPipelineName = "GltfMorphModelPS";

    // 後面の中心
    backCenterComponent = AddComponent<SkeletalMeshComponent>("backCenterComponent", parentName);
    backCenterComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    backCenterComponent->SetRelativeLocationDirect({ 0.0f,size.y * 0.5f,-size.z * 0.5f });
    backCenterComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    backCenterComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    backCenterComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    backCenterComponent->overrideDeferredPipelineName = "GltfMorphModelPS";


#endif // 0
    // 入力用のコンポーネントを追加
    inputComponent = this->AddComponent<class InputComponent>("inputComponent", parentName);

    DirectX::XMFLOAT3 wheelLocalPos[4] =
    {
    {  0.6f, 0.3f,  0.7f }, // Front Right
    { -0.6f, 0.3f,  0.7f }, // Front Left
    {  0.6f, 0.3f, -0.7f }, // Rear Right
    { -0.6f, 0.3f, -0.7f }, // Rear Left
    };

    for (int i = 0; i < 4; ++i)
    {
        std::string name = "wheelComponent" + std::to_string(i);

        wheelMeshComponents[i] = AddComponent<SkeletalMeshComponent>(name, parentName);
        wheelMeshComponents[i]->SetModel("./Data/Models/Car/wheel.glb");
        wheelMeshComponents[i]->SetRelativeLocationDirect(wheelLocalPos[i]);
        wheelMeshComponents[i]->renderPass = MeshComponent::MeshRenderPass::Forward;
        wheelMeshComponents[i]->overrideForwardPipelineName = "GltfMorphModelPS";
        wheelMeshComponents[i]->overrideDeferredPipelineName = "GltfMorphModelPS";
        wheelMeshComponents[i]->SetIsVisible(false);

        wheelSphereComponents[i] = AddComponent<SphereComponent>("wheelSphereComponent", name);
        size = wheelMeshComponents[i]->GetModelSize();
        wheelSphereComponents[i]->SetRadius(size.x);
        wheelSphereComponents[i]->SetLayer(CollisionLayer::CarWheel);
        wheelSphereComponents[i]->SetResponseToLayer(CollisionLayer::Floor, CollisionComponent::CollisionResponse::Block);
        wheelSphereComponents[i]->Initialize();
        wheelSphereComponents[i]->SetOnHitCallback([this, i](CollisionComponent* self, CollisionComponent* other)
            {
                //Logger::Log(U8("地面に当たった") + std::to_string(i));
                wheelGround[i] = true;
            });
    }

    currentFace = Face::Bottom;
}

void CarActor::Update(float elapsedTime)
{
#if 1
    using namespace DirectX;

    onGround = wheelGround[0] || wheelGround[1] || wheelGround[2] || wheelGround[3];

    auto intent = inputComponent->GetIntent();

    const float rotateInput = intent.leftMove.x;
    const float forward = intent.rightMove.x;

    const float inputDeadZone = 0.1f;
    const float rollingSpeed = XMConvertToRadians(90.0f);

    if (rollState == RollState::None)
    {
        if (rotateInput > inputDeadZone)
        {
            StartRoll(true);
        }
        else if (rotateInput < -inputDeadZone)
        {
            StartRoll(false);
        }
    }
    else if (rollState == RollState::Input)
    {
        if (std::abs(rotateInput) <= inputDeadZone)
        {
            if (rollingAngle < XMConvertToRadians(45.0f))
            {
                continueRollThroughSide = false;
                rollState = RollState::Return;
            }
            else
            {
                continueRollThroughSide = true;
                rollState = RollState::Finish;
            }
        }
    }

    if (rollState != RollState::None)
    {
        float oldAngle = rollingAngle;

        if (rollState == RollState::Input || rollState == RollState::Finish)
        {
            rollingAngle += rollingSpeed * elapsedTime;

            if (rollingAngle >= rollTargetAngle)
            {
                rollingAngle = rollTargetAngle;
            }
        }
        else if (rollState == RollState::Return)
        {
            rollingAngle -= rollingSpeed * elapsedTime;

            if (rollingAngle <= 0.0f)
            {
                rollingAngle = 0.0f;
            }
        }

        float deltaAngle = rollingAngle - oldAngle;
        ApplyRollDelta(deltaAngle);

        previousRollingAngle = rollingAngle;

        if (rollState == RollState::Return && rollingAngle <= 0.0f)
        {
            EndRoll(false);
        }
        else if ((rollState == RollState::Input || rollState == RollState::Finish) &&
            rollingAngle >= rollTargetAngle)
        {
            EndRoll(true);
        }
    }

    // XZ二だけ移動
    XMVECTOR rotQ = XMLoadFloat4(&rigid.rotation);

    XMVECTOR forwardVec = XMVector3Rotate(XMVectorSet(0, 0, -1, 0), rotQ);
    forwardVec = XMVectorSetY(forwardVec, 0.0f);

    if (XMVectorGetX(XMVector3LengthSq(forwardVec)) > 1e-6f)
        forwardVec = XMVector3Normalize(forwardVec);
    else
        forwardVec = XMVectorZero();

    XMVECTOR velocity = XMLoadFloat3(&rigid.linearVelocity);
    velocity = XMVectorSetY(velocity, 0.0f);

    if (onGround)
        velocity += forwardVec * (forward * 1.0f * elapsedTime);

    const float drag = 4.0f;
    velocity -= velocity * drag * elapsedTime;
    velocity = XMVectorSetY(velocity, 0.0f);

    XMVECTOR pos = XMLoadFloat3(&rigid.position);
    pos += velocity;

    XMStoreFloat3(&rigid.linearVelocity, velocity);
    XMStoreFloat3(&rigid.position, pos);

    SetPosition(rigid.position);
    SetQuaternionRotation(rigid.rotation);

    for (bool& g : wheelGround)
        g = false;

#else
    using namespace DirectX;
    // --- 接地判定（前進できるかどうか） ---
    onGround = wheelGround[0] || wheelGround[1] || wheelGround[2] || wheelGround[3];

    auto intent = inputComponent->GetIntent();
    float rotateInput = intent.leftMove.x;   // ピッチ回転入力
    float forward = intent.rightMove.x;  // 前後入力
    const float rollingSpeed = DirectX::XMConvertToRadians(60.0f);
    if (rotateInput > 0.0f)
    {// 前に入力があったら
        switch (currentFace)
        {
        case Face::Bottom:
            if (rollingAngle >= 0)
            {
                rollingPivot = frontPivotBottomComponent->GetComponentLocation();
                baseRotation = { 0,0,0,1 };
            }
            else if (rollingAngle < 0)
            {
                rollingPivot = backPivotBottomComponent->GetComponentLocation();
                baseRotation = { 0,0,0,1 };
            }
#if 0
            if (!rolling)
            {
                rolling = true;
                rollingAngle = 0.0f;
                previousRollingAngle = 0.0f;
            }
            if (rolling)
#endif // 0
            {
                // 回転の処理
                rollingAngle += rollingSpeed * elapsedTime;
                rollingAngle = MathHelper::ClampAngle(rollingAngle);
                XMVECTOR base = XMLoadFloat4(&baseRotation);
                XMVECTOR offset = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), rollingAngle);
                XMVECTOR rotation = XMQuaternionMultiply(base, offset);
                DirectX::XMStoreFloat4(&rigid.rotation, rotation);
                // 位置の移動
                float deltaAngle = rollingAngle - previousRollingAngle;
                XMVECTOR q = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), deltaAngle);
                XMVECTOR Pos = XMLoadFloat3(&rigid.position);
                XMVECTOR Pivot = XMLoadFloat3(&rollingPivot);
                Pos -= Pivot;
                Pos = XMVector3Rotate(Pos, q);
                Pos += Pivot;
                DirectX::XMStoreFloat3(&rigid.position, Pos);
                previousRollingAngle = rollingAngle;
                Logger::Log("rollingAngle:" + std::to_string(DirectX::XMConvertToDegrees(rollingAngle)));
            }
            if (DirectX::XMConvertToDegrees(rollingAngle) >= 90)
            {
                currentFace = Face::Front;
            }
            break;
        case Face::Front:
        {
            if (DirectX::XMConvertToDegrees(rollingAngle) >= 90)
            {
                rollingPivot = frontPivotTopComponent->GetComponentLocation();
                baseRotation = { 0,0,0,1 };
            }
            // 回転の処理
            rollingAngle += rollingSpeed * elapsedTime;
            rollingAngle = MathHelper::ClampAngle(rollingAngle);
            XMVECTOR base = XMLoadFloat4(&baseRotation);
            XMVECTOR offset = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), rollingAngle);
            XMVECTOR rotation = XMQuaternionMultiply(base, offset);
            DirectX::XMStoreFloat4(&rigid.rotation, rotation);
            // 位置の移動
            float deltaAngle = rollingAngle - previousRollingAngle;
            XMVECTOR q = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), deltaAngle);
            XMVECTOR Pos = XMLoadFloat3(&rigid.position);
            XMVECTOR Pivot = XMLoadFloat3(&rollingPivot);
            Pos -= Pivot;
            Pos = XMVector3Rotate(Pos, q);
            Pos += Pivot;
            DirectX::XMStoreFloat3(&rigid.position, Pos);
            previousRollingAngle = rollingAngle;
            Logger::Log("rollingAngle:" + std::to_string(DirectX::XMConvertToDegrees(rollingAngle)));
            if (DirectX::XMConvertToDegrees(rollingAngle) < 90)
            {
                currentFace = Face::Bottom;
            }
        }
        break;
        case Face::Top:
            break;
        case Face::Back:
            break;
        }
    }
    else if (rotateInput < 0.0f)
    {
        switch (currentFace)
        {
        case Face::Bottom:
            if (rollingAngle >= 0)
            {
                rollingPivot = frontPivotBottomComponent->GetComponentLocation();
                baseRotation = { 0,0,0,1 };
            }
            else if (rollingAngle < 0)
            {
                rollingPivot = backPivotBottomComponent->GetComponentLocation();
                baseRotation = { 0,0,0,1 };
            }
            {
                // 回転の処理
                rollingAngle -= rollingSpeed * elapsedTime;
                rollingAngle = MathHelper::ClampAngle(rollingAngle);
                XMVECTOR base = XMLoadFloat4(&baseRotation);
                XMVECTOR offset = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), rollingAngle);
                XMVECTOR rotation = XMQuaternionMultiply(base, offset);
                DirectX::XMStoreFloat4(&rigid.rotation, rotation);
                // 位置の移動
                float deltaAngle = rollingAngle - previousRollingAngle;
                XMVECTOR q = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), deltaAngle);
                XMVECTOR Pos = XMLoadFloat3(&rigid.position);
                XMVECTOR Pivot = XMLoadFloat3(&rollingPivot);
                Pos -= Pivot;
                Pos = XMVector3Rotate(Pos, q);
                Pos += Pivot;
                DirectX::XMStoreFloat3(&rigid.position, Pos);
                previousRollingAngle = rollingAngle;
                Logger::Log("rollingAngle:" + std::to_string(DirectX::XMConvertToDegrees(rollingAngle)));
            }
            break;
        case Face::Front:
            break;
        case Face::Top:
            break;
        case Face::Back:
            break;
        }
    }
    else
    {
        float oldAngle = rollingAngle;
        float rollingAngleDegree = DirectX::XMConvertToDegrees(rollingAngle);

        // 入力なしでも、角度が中途半端なら rolling 中として扱う
        if (std::abs(rollingAngleDegree) > 0.01f)
        {
            rolling = true;
        }

        if (90.0f < rollingAngleDegree && rollingAngleDegree < 180.0f)
        {
            rollingAngle += rollingSpeed * elapsedTime;

            if (rollingAngle > DirectX::XM_PI)
                rollingAngle = DirectX::XM_PI;
        }
        else if (0.0f < rollingAngleDegree && rollingAngleDegree <= 90.0f)
        {
            rollingAngle -= rollingSpeed * elapsedTime;

            if (rollingAngle < 0.0f)
                rollingAngle = 0.0f;
        }
        else if (-90.0f <= rollingAngleDegree && rollingAngleDegree < 0.0f)
        {
            rollingAngle += rollingSpeed * elapsedTime;

            if (rollingAngle > 0.0f)
                rollingAngle = 0.0f;
        }
        else if (-180.0f < rollingAngleDegree && rollingAngleDegree < -90.0f)
        {
            rollingAngle -= rollingSpeed * elapsedTime;

            if (rollingAngle < -DirectX::XM_PI)
                rollingAngle = -DirectX::XM_PI;
        }

        // 角度
        XMVECTOR base = XMLoadFloat4(&baseRotation);
        XMVECTOR offset = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), rollingAngle);
        XMVECTOR rotation = XMQuaternionMultiply(base, offset);
        rotation = XMQuaternionNormalize(rotation);
        DirectX::XMStoreFloat4(&rigid.rotation, rotation);

        // 位置の移動
        float deltaAngle = rollingAngle - oldAngle;

        if (std::abs(deltaAngle) > 1e-6f)
        {
            XMVECTOR q = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), deltaAngle);
            XMVECTOR Pos = XMLoadFloat3(&rigid.position);
            XMVECTOR Pivot = XMLoadFloat3(&rollingPivot);

            Pos -= Pivot;
            Pos = XMVector3Rotate(Pos, q);
            Pos += Pivot;

            DirectX::XMStoreFloat3(&rigid.position, Pos);
        }

        previousRollingAngle = rollingAngle;

        // 完全に戻った / 倒れ切った時だけ rolling 終了
        if (std::abs(rollingAngle) < 1e-5f)
        {
            rollingAngle = 0.0f;
            previousRollingAngle = 0.0f;
            rolling = false;
            currentFace = Face::Bottom;
        }
        else if (std::abs(rollingAngle - DirectX::XM_PI) < 1e-5f)
        {
            rolling = false;
            currentFace = Face::Top;
        }
        else if (std::abs(rollingAngle + DirectX::XM_PI) < 1e-5f)
        {
            rolling = false;
            currentFace = Face::Top;
        }
    }

    //  前進処理（地面に投影した forward）
    XMVECTOR rotQ = XMLoadFloat4(&rigid.rotation);
    XMVECTOR forwardVec = XMVector3Rotate(XMVectorSet(0, 0, -1, 0), rotQ);
    forwardVec = XMVectorSetY(forwardVec, 0);
    forwardVec = XMVector3Normalize(forwardVec);

    XMVECTOR Velocity = XMLoadFloat3(&rigid.linearVelocity);
    XMVECTOR Pos = XMLoadFloat3(&rigid.position);

    if (onGround)
    Velocity += forwardVec * (forward * 1.0f * elapsedTime);

    // 摩擦
    float drag = 4.0f;
    Velocity -= Velocity * drag * elapsedTime;

    DirectX::XMStoreFloat3(&rigid.linearVelocity, Velocity);
    Pos += Velocity;

    DirectX::XMStoreFloat3(&rigid.position, Pos);
    SetPosition(rigid.position);
    SetQuaternionRotation(rigid.rotation);

    // wheelGround リセット
    for (bool& g : wheelGround)
        g = false;

#endif // 0
}

void CarActor::StartRoll(bool toForward)
{
    rollToForward = toForward;
    rolling = true;
    rollState = RollState::Input;

    continueRollThroughSide = false;

    rollingAngle = 0.0f;
    previousRollingAngle = 0.0f;
    rollTargetAngle = DirectX::XM_PIDIV2;

    baseRotation = rigid.rotation;

    UpdateRollingPivot(toForward);
}

void CarActor::StartAutoRoll(bool toForward)
{
    rollToForward = toForward;
    rolling = true;
    rollState = RollState::Finish;

    rollingAngle = 0.0f;
    previousRollingAngle = 0.0f;
    rollTargetAngle = DirectX::XM_PIDIV2;

    baseRotation = rigid.rotation;

    UpdateRollingPivot(toForward);
}

void CarActor::ApplyRollDelta(float deltaAngle)
{
    using namespace DirectX;

    if (std::abs(deltaAngle) < 1e-6f)
        return;

    const float signedDelta = rollToForward ? deltaAngle : -deltaAngle;

    XMVECTOR q = XMQuaternionRotationAxis(
        XMVectorSet(1, 0, 0, 0),
        signedDelta);

    XMVECTOR pos = XMLoadFloat3(&rigid.position);
    XMVECTOR pivot = XMLoadFloat3(&rollingPivot);

    pos -= pivot;
    pos = XMVector3Rotate(pos, q);
    pos += pivot;

    XMStoreFloat3(&rigid.position, pos);

    XMVECTOR rot = XMLoadFloat4(&rigid.rotation);
    rot = XMQuaternionMultiply(q, rot);
    rot = XMQuaternionNormalize(rot);

    XMStoreFloat4(&rigid.rotation, rot);
}

void CarActor::EndRoll(bool completed)
{
    rolling = false;
    rollState = RollState::None;

    rollingAngle = 0.0f;
    previousRollingAngle = 0.0f;
    rollTargetAngle = 0.0f;

    if (completed)
        AdvanceCurrentFace(rollToForward);

    if (completed &&
        continueRollThroughSide &&
        (currentFace == Face::Front || currentFace == Face::Back))
    {
        StartAutoRoll(rollToForward);
        return;
    }

    if (currentFace == Face::Top || currentFace == Face::Bottom)
    {
        continueRollThroughSide = false;
    }
}

// 基準点を更新する
void CarActor::UpdateRollingPivot(bool toForward)
{
#if 0
    // --- 4 面の中心の高さを取得 ---
    float yFront = frontCenterComponent->GetComponentLocation().y;
    float yTop = topCenterComponent->GetComponentLocation().y;
    float yBack = backCenterComponent->GetComponentLocation().y;
    float yBottom = bottomCenterComponent->GetComponentLocation().y;

    float minY = yFront;
    currentFace = Face::Front; // 0=前, 1=上, 2=後, 3=底

    if (yTop < minY) { minY = yTop;    currentFace = Face::Top; }
    if (yBack < minY) { minY = yBack;   currentFace = Face::Back; }
    if (yBottom < minY) { minY = yBottom; currentFace = Face::Bottom; }

#endif // 0

    // ============================================================
    //  ★ pivot 決定表（縦方向サイコロ回転）
    //
    //  前入力： 底 → 前 → 上 → 後 → 底 …
    //  後入力： 底 → 後 → 上 → 前 → 底 …
    // ============================================================
    switch (currentFace)
    {
    case Face::Bottom: // 底面が下（普通の状態）
        if (toForward)
            rollingPivot = frontPivotBottomComponent->GetComponentLocation();
        else
            rollingPivot = backPivotBottomComponent->GetComponentLocation();
        break;

    case Face::Front: // 前面が下（前に倒れている）
        if (toForward)
            rollingPivot = frontPivotTopComponent->GetComponentLocation(); // 前→上へ
        else
            rollingPivot = frontPivotBottomComponent->GetComponentLocation();  // 後→底へ
        break;

    case Face::Top: // 上面が下（裏返っている）
        if (toForward)
            rollingPivot = backPivotTopComponent->GetComponentLocation();  // 前→後へ
        else
            rollingPivot = frontPivotTopComponent->GetComponentLocation(); // 後→前へ
        break;

    case Face::Back: // 後面が下（後ろに倒れている）
        if (toForward)
            rollingPivot = backPivotBottomComponent->GetComponentLocation();  // 前→底へ
        else
            rollingPivot = backPivotTopComponent->GetComponentLocation();  // 後→上へ
        break;
    }
    rollingPivot.y = 0.0f;
}

// 面を更新する // 前方向かどうか
void CarActor::AdvanceCurrentFace(bool toForward)
{
    if (toForward)
    {// 前方向へ
        switch (currentFace)
        {
        case Face::Bottom:
            currentFace = Face::Front;
            break;

        case Face::Front:
            currentFace = Face::Top;
            break;

        case Face::Top:
            currentFace = Face::Back;
            break;

        case Face::Back:
            currentFace = Face::Bottom;
            break;
        }
    }
    else
    {// 後ろ方向へ
        switch (currentFace)
        {
        case Face::Bottom:
            currentFace = Face::Back;
            break;

        case Face::Back:
            currentFace = Face::Top;
            break;

        case Face::Top:
            currentFace = Face::Front;
            break;

        case Face::Front:
            currentFace = Face::Bottom;
            break;
        }
    }

    Logger::Log(("currentFace") + std::string(magic_enum::enum_name(currentFace)));

}

void CarActor::ApplyRollDeltaImmediate(float deltaAngle)
{
    using namespace DirectX;

    if (std::abs(deltaAngle) < 1e-6f)
        return;

    const bool toForward = deltaAngle > 0.0f;

    UpdateRollingPivot(toForward);

    XMVECTOR q = XMQuaternionRotationAxis(
        XMVectorSet(1, 0, 0, 0),
        deltaAngle);

    XMVECTOR pos = XMLoadFloat3(&rigid.position);
    XMVECTOR pivot = XMLoadFloat3(&rollingPivot);

    pos -= pivot;
    pos = XMVector3Rotate(pos, q);
    pos += pivot;

    XMStoreFloat3(&rigid.position, pos);

    XMVECTOR rot = XMLoadFloat4(&rigid.rotation);
    rot = XMQuaternionMultiply(q, rot);
    rot = XMQuaternionNormalize(rot);

    XMStoreFloat4(&rigid.rotation, rot);
}

// 差分の角度で位置を更新する
void CarActor::ApplyRollingDelta(float deltaAngle)
{
    using namespace DirectX;

    if (std::abs(deltaAngle) < 1e-6f)
        return;

    XMVECTOR q = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), deltaAngle);

    XMVECTOR pos = XMLoadFloat3(&rigid.position);
    XMVECTOR pivot = XMLoadFloat3(&rollingPivot);

    pos = XMVectorSubtract(pos, pivot);
    pos = XMVector3Rotate(pos, q);
    pos = XMVectorAdd(pos, pivot);

    XMStoreFloat3(&rigid.position, pos);
}

void CarActor::DrawImGuiDetails()
{
#ifdef USE_IMGUI
    ImGui::DragFloat("rollingAngle", &rollingAngle, 0.01f);
#endif
}
