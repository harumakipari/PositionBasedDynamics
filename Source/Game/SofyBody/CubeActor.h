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

    // í◊Ç∑
    void Press();

    // í◊Ç∑ÇÃÇé~ÇﬂÇÈ
    void StopPress();

    //DirectX::XMFLOAT3 extent = { 10.0f,3.0f,10.0f };
    DirectX::XMFLOAT3 extent = { 7.0f,3.0f,7.0f };
    //DirectX::XMFLOAT3 extent = { 3.0f,1.0f,3.0f };
private:
    std::shared_ptr<StaticMeshComponent> staticMeshComponent;


    float viewScale = 0.8f;

    enum class PressState
    {
        Idle,       // ë“ã@
        MovingDown, // ç~â∫
        BottomWait, // â∫Ç≈í‚é~
        MovingUp,   // è„è∏
        TopWait     // è„Ç≈í‚é~
    };

    PressState pressState = PressState::Idle;

    // ÉvÉåÉXÇ…égópÇ∑ÇÈïœêî
    float topY = 7.6f;
    float bottomY = 4.3f;

    float pressSpeed = 3.0f;
    float returnSpeed = 2.0f;

    float waitTimer = 0.0f;
    float bottomWaitTime = 0.5f;
    float topWaitTime = 1.0f;

    bool repeatPress = true;
};