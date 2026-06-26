#include "pch.h"
#include "CarActor.h"

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

    // 前面の上
    frontPivotTopComponent = AddComponent<SkeletalMeshComponent>("frontPivotTopComponent", parentName);
    frontPivotTopComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    frontPivotTopComponent->SetRelativeLocationDirect({ 0.0f,size.y,size.z * 0.5f });
    frontPivotTopComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    frontPivotTopComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    frontPivotTopComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    frontPivotTopComponent->overrideDeferredPipelineName = "GltfMorphModelPS";

    // 後面の下
    backPivotBottomComponent = AddComponent<SkeletalMeshComponent>("backPivotBottonComponent", parentName);
    backPivotBottomComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    backPivotBottomComponent->SetRelativeLocationDirect({ 0.0f,0.0f,-size.z * 0.5f });
    backPivotBottomComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    backPivotBottomComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    backPivotBottomComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    backPivotBottomComponent->overrideDeferredPipelineName = "GltfMorphModelPS";

    // 後面の上
    backPivotTopComponent = AddComponent<SkeletalMeshComponent>("backPivotTopComponent", parentName);
    backPivotTopComponent->SetModel("./Data/Models/Primitives/sphere.glb");
    backPivotTopComponent->SetRelativeLocationDirect({ 0.0f,size.y,-size.z * 0.5f });
    backPivotTopComponent->SetRelativeScaleDirect({ 0.1f,0.1f,0.1f });
    backPivotTopComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    backPivotTopComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    backPivotTopComponent->overrideDeferredPipelineName = "GltfMorphModelPS";

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

        wheelSphereComponents[i] = AddComponent<SphereComponent>("wheelSphereComponent", name);
        size = wheelMeshComponents[i]->GetModelSize();
        wheelSphereComponents[i]->SetRadius(size.x);
        wheelSphereComponents[i]->SetLayer(CollisionLayer::CarWheel);
        wheelSphereComponents[i]->SetResponseToLayer(CollisionLayer::Floor, CollisionComponent::CollisionResponse::Block);
        wheelSphereComponents[i]->Initialize();
        wheelSphereComponents[i]->SetOnHitCallback([this, i](CollisionComponent* self, CollisionComponent* other)
            {
                Logger::Log(U8("地面に当たった") + std::to_string(i));
                wheelGround[i] = true;
            });
    }
}

void CarActor::Update(float elapsedTime)
{
#if 1
    using namespace DirectX;

    // --- 接地判定（前進できるかどうか） ---
    onGround = wheelGround[0] || wheelGround[1] || wheelGround[2] || wheelGround[3];

    auto intent = inputComponent->GetIntent();
    float left = intent.leftMove.x;   // ピッチ回転入力
    float forward = intent.rightMove.x;  // 前後入力

    // --- 現在の回転を取得 ---
    XMFLOAT4 rotation = rigid.rotation;
    XMVECTOR Rotation = XMLoadFloat4(&rotation);

    // ============================================================
    //  ★ サイコロ回転：回転開始時に pivot を決める
    // ============================================================
    if (!rolling)
    {
        if (fabs(left) > 0.5f)
        {
            rolling = true;
            rollingAngle = 0.0f;
            rollingDirection = (left > 0) ? -1 : 1;

            // --- 4 面の中心の高さを取得 ---
            float yFront = frontCenterComponent->GetComponentLocation().y;
            float yTop = topCenterComponent->GetComponentLocation().y;
            float yBack = backCenterComponent->GetComponentLocation().y;
            float yBottom = bottomCenterComponent->GetComponentLocation().y;

            float minY = yFront;
            int face = 0; // 0=前, 1=上, 2=後, 3=底

            if (yTop < minY) { minY = yTop;    face = 1; }
            if (yBack < minY) { minY = yBack;   face = 2; }
            if (yBottom < minY) { minY = yBottom; face = 3; }

            // ============================================================
            //  ★ pivot 決定表（縦方向サイコロ回転）
            //
            //  前入力： 底 → 前 → 上 → 後 → 底 …
            //  後入力： 底 → 後 → 上 → 前 → 底 …
            // ============================================================

            switch (face)
            {
            case 3: // 底面が下（普通の状態）
                if (left > 0)
                    rollingPivot = frontPivotBottomComponent->GetComponentLocation();
                else
                    rollingPivot = backPivotBottomComponent->GetComponentLocation();
                break;

            case 0: // 前面が下（前に倒れている）
                if (left > 0)
                    rollingPivot = frontPivotTopComponent->GetComponentLocation(); // 前→上へ
                else
                    rollingPivot = bottomCenterComponent->GetComponentLocation();  // 後→底へ
                break;

            case 1: // 上面が下（裏返っている）
                if (left > 0)
                    rollingPivot = backPivotTopComponent->GetComponentLocation();  // 前→後へ
                else
                    rollingPivot = frontPivotTopComponent->GetComponentLocation(); // 後→前へ
                break;

            case 2: // 後面が下（後ろに倒れている）
                if (left > 0)
                    rollingPivot = bottomCenterComponent->GetComponentLocation();  // 前→底へ
                else
                    rollingPivot = backPivotTopComponent->GetComponentLocation();  // 後→上へ
                break;
            }

            rollingPivot.y = 0.0f;

        }
    }

    // ============================================================
    //  ★ サイコロ回転の実行（pivot は固定）
    // ============================================================
    if (rolling)
    {
        float delta = rollingSpeed * elapsedTime;

        // 今回回す角度
        float angle = delta * rollingDirection;

        // 合計回転角は常に正で管理
        rollingAngle += delta;

        if (rollingAngle >= XM_PIDIV2)
        {
            // 最後の1フレームだけ90°ぴったりになるよう補正
            float over = rollingAngle - XM_PIDIV2;

            delta -= over;
            angle = delta * rollingDirection;

            rolling = false;
        }

        // Quaternion作成
        XMVECTOR q =
            XMQuaternionRotationAxis(
                XMVectorSet(1, 0, 0, 0),
                angle);

        // 姿勢更新
        Rotation = XMQuaternionMultiply(Rotation, q);

        XMStoreFloat4(&rigid.rotation, Rotation);
        SetQuaternionRotation(rigid.rotation);

        // Position更新
        XMVECTOR Pos = XMLoadFloat3(&rigid.position);
        XMVECTOR Pivot = XMLoadFloat3(&rollingPivot);

        Pos -= Pivot;
        Pos = XMVector3Rotate(Pos, q);
        Pos += Pivot;

        XMStoreFloat3(&rigid.position, Pos);
        SetPosition(rigid.position);
    }

    // ============================================================
    //  ★ 前進処理（地面に投影した forward）
    // ============================================================
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

    XMStoreFloat3(&rigid.linearVelocity, Velocity);
    Pos += Velocity;

    XMStoreFloat3(&rigid.position, Pos);
    SetPosition(rigid.position);

    // wheelGround リセット
    for (bool& g : wheelGround)
        g = false;
#else
    onGround = wheelGround[0] || wheelGround[1] || wheelGround[2] || wheelGround[3];

    using namespace DirectX;

    auto intent = inputComponent->GetIntent();
    float left = intent.leftMove.x;

    DirectX::XMFLOAT3 pivot = { 0.0f,0.0f,0.0f };



    // 現在の回転を取得
    DirectX::XMFLOAT4 rotation = rigid.rotation;
    DirectX::XMVECTOR Rotation = DirectX::XMLoadFloat4(&rotation);

    // X軸で回転（前に倒れる）
    DirectX::XMVECTOR AddRotation = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1, 0, 0, 0), left * elapsedTime * 2.0f);

    // 合成
    Rotation = DirectX::XMQuaternionMultiply(Rotation, AddRotation);

    // 剛体に反映
    DirectX::XMStoreFloat4(&rigid.rotation, Rotation);

    // Actor の Transform にも反映
    SetQuaternionRotation(rigid.rotation);

    float forward = intent.rightMove.x; // 前後入力
    //Logger::Log("left" + std::to_string(left) + "forward" + std::to_string(forward));
    // 剛体の forward ベクトルを取得
    DirectX::XMVECTOR rot = XMLoadFloat4(&rigid.rotation);
    DirectX::XMVECTOR forwardVec = DirectX::XMVector3Rotate(
        DirectX::XMVectorSet(0, 0, -1, 0), // モデルの前方向
        rot);

    forwardVec = XMVectorSetY(forwardVec, 0);
    forwardVec = XMVector3Normalize(forwardVec);

    // 前進速度を追加
    DirectX::XMVECTOR Velocity = XMLoadFloat3(&rigid.linearVelocity);
    DirectX::XMVECTOR Pos = DirectX::XMLoadFloat3(&rigid.position);
    // --- 接地しているときだけ前進 ---
    if (onGround)
    {
        Velocity += forwardVec * (forward * 1.0f * elapsedTime);
    }
    // --- 摩擦で減速 ---
    float drag = 4.0f;
    Velocity -= Velocity * drag * elapsedTime;
    DirectX::XMStoreFloat3(&rigid.linearVelocity, Velocity);
    // --- 位置更新 ---
    Pos += Velocity;

    XMStoreFloat3(&rigid.position, Pos);
    SetPosition(rigid.position);


#if 0
    PBD::IntegrateRigidBody(rigid, elapsedTime);
    PBD::SolvePlaneForRigidBody(rigid, 0.0f, 0.5f);

    // 自動安定トルク（JerryCarsWorld の「起き上がろうとする」感じ）
    ApplyUprightTorque(rigid, /*strength=*/2.0f, elapsedTime);

#endif // 0
    for (bool& i : wheelGround)
        i = false;
#endif // 0

}

void CarActor::DrawImGuiDetails()
{

}
