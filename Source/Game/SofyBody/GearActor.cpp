#include "pch.h"
#include "GearActor.h"

void GearActor::Initialize(const Transform& transform)
{
    std::string parentName = "GearActor";
    skeletalMeshComponent = AddComponent<SkeletalMeshComponent>(parentName);
    skeletalMeshComponent->SetModel("./Data/Models/Gear.glb");
    skeletalMeshComponent->renderPass = MeshComponent::MeshRenderPass::Forward;
    skeletalMeshComponent->overrideForwardPipelineName = "GltfMorphModelPS";
    skeletalMeshComponent->overrideDeferredPipelineName = "GltfMorphModelPS";




}

void GearActor::Update(float elapsedTime)
{
    
}

void GearActor::DrawImGuiDetails()
{
    
}
