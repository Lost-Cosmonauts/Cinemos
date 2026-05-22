// Copyright Lost Cosmonauts

#include "CinemosBlueprintFunctionLibrary.h"

#include "Engine/DataTable.h"
#include "Tracks/MovieSceneSubTrack.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/ChildActorComponent.h"
#include "Kismet2/KismetEditorUtilities.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CinemosBlueprintFunctionLibrary)

UMovieSceneSubSection* UCinemosBlueprintFunctionLibrary::AddSubSequenceToSubTrack(UMovieSceneSubTrack* SubTrack, UMovieSceneSequence* Sequence, FFrameNumber StartTime, int32 Duration)
{
	return AddSubSequenceToSubTrackRow(SubTrack, Sequence, StartTime, Duration, INDEX_NONE);
}

UMovieSceneSubSection* UCinemosBlueprintFunctionLibrary::AddSubSequenceToSubTrackRow(UMovieSceneSubTrack* SubTrack, UMovieSceneSequence* Sequence, FFrameNumber StartTime, int32 Duration, int32 RowIndex)
{
    if (!SubTrack)
    {
        return nullptr;
    }

    return SubTrack->AddSequenceOnRow(Sequence, StartTime, Duration, RowIndex);
}

void UCinemosBlueprintFunctionLibrary::ClearDataTable(UDataTable* DataTable)
{
	DataTable->EmptyTable();
}

bool UCinemosBlueprintFunctionLibrary::IsPartsMapEqual(TMap<FName, bool> A, TMap<FName, bool> B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (const auto& Pair : A)
	{
		if (!B.Contains(Pair.Key) || B[Pair.Key] != Pair.Value)
		{
			return false;
		}
	}

	return true;
}

void UCinemosBlueprintFunctionLibrary::BakeChildActorsIntoAsset(AActor* SelectedLevelActor)
{
	if (!SelectedLevelActor) return;

	// Get the Blueprint asset from the selected actor in the world
	UBlueprint* ParentBP = Cast<UBlueprint>(SelectedLevelActor->GetClass()->ClassGeneratedBy);
	if (!ParentBP || !ParentBP->SimpleConstructionScript)
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected Actor is not a Blueprint Asset."));
		return;
	}

	// Get the SCS for getting the nodes
	USimpleConstructionScript* ParentSCS = ParentBP->SimpleConstructionScript;
	bool bBlueprintModified = false;

	// Search through ChildActorComponents in the world
	TArray<UChildActorComponent*> InstanceCACs;
	SelectedLevelActor->GetComponents<UChildActorComponent>(InstanceCACs);
	for (UChildActorComponent* InstanceCAC : InstanceCACs)
	{
		// Find the ChildActor in the world
		AActor* ChildActorInstance = InstanceCAC->GetChildActor();
		if (!ChildActorInstance) continue;
		// we cannot copy Blueprints safely
		if (ChildActorInstance->GetClass()->ClassGeneratedBy != nullptr)
		{
			UE_LOG(LogTemp, Log, TEXT("Skipped Blueprint Child Actor: %s to preserve its logic."), *ChildActorInstance->GetName());
			continue;
		}
		// TODO: support more than just static mesh actor?
		if (!ChildActorInstance->IsA<AStaticMeshActor>())
		{
			continue;
		}

		// Find the ChildActorComponent node in Blueprint
		USCS_Node* TargetSCSNode = nullptr;
		for (USCS_Node* Node : ParentSCS->GetAllNodes())
		{
			if (Node->ComponentTemplate == InstanceCAC->GetArchetype())
			{
				TargetSCSNode = Node;
				break;
			}
		}
		if (!TargetSCSNode) continue;

		// Get the root transform of the ChildActor. Basically, we have the root scene component's properties that apply from the child actor instance, but this is snapped to the child actor component, so that the child actor COMPONENT defines the transform at which the child actor is located at
		USceneComponent* ChildRootComp = ChildActorInstance->GetRootComponent();
		// we multiply by the child root comp transform to make sure we are getting the final representation of the transform after construction
		FTransform RelativeTransform = InstanceCAC->GetRelativeTransform() * ChildRootComp->GetRelativeTransform();

		TMap<UActorComponent*, USCS_Node*> NodeMap;
		TArray<UActorComponent*> ChildComponents;
		ChildActorInstance->GetComponents(ChildComponents);
		for (UActorComponent* ChildComp : ChildComponents)
		{
			// Copy objects from the component in the ChildActor instance in the world, to the component template for our new node
			USCS_Node* NewNode = ParentSCS->CreateNode(ChildComp->GetClass(), ChildComp->GetFName());
			if (NewNode && NewNode->ComponentTemplate)
			{
				UEngine::CopyPropertiesForUnrelatedObjects(ChildComp, NewNode->ComponentTemplate);

				// Fix up any specifics with components here
				if (USplineComponent* SourceSpline = Cast<USplineComponent>(ChildComp))
				{
					if (USplineComponent* DestSpline = Cast<USplineComponent>(NewNode->ComponentTemplate))
					{
						DestSpline->SplineCurves = SourceSpline->SplineCurves;
						DestSpline->UpdateSpline();
					}
				}
				// End component specific fixups

				// Make sure we migrate scene component properly
				if (USceneComponent* NewSceneTemplate = Cast<USceneComponent>(NewNode->ComponentTemplate))
				{
					if (ChildComp == ChildRootComp)
					{
						NewSceneTemplate->SetRelativeTransform_Direct(RelativeTransform);

						NewSceneTemplate->Mobility = InstanceCAC->Mobility;
					}
					else
					{
						if (InstanceCAC->Mobility == EComponentMobility::Static)
						{
							NewSceneTemplate->Mobility = EComponentMobility::Static;
						}
					}
				}
			}

			ParentSCS->AddNode(NewNode);
			NodeMap.Add(ChildComp, NewNode);
		}

		for (UActorComponent* ChildComp : ChildComponents)
		{
			USCS_Node* NewNode = NodeMap[ChildComp];
			if (USceneComponent* SceneComp = Cast<USceneComponent>(ChildComp))
			{
				if (SceneComp == ChildRootComp)
				{
					// Attach the new root to wherever the CAC used to be attached
					if (TargetSCSNode->ParentComponentOrVariableName != NAME_None)
					{
						NewNode->SetParent(ParentSCS->FindSCSNode(TargetSCSNode->ParentComponentOrVariableName));
					}
				}
				else if (SceneComp->GetAttachParent())
				{
					// If it was a child component INSIDE the child actor, preserve that internal attachment
					USCS_Node** ParentNodePtr = NodeMap.Find(SceneComp->GetAttachParent());
					if (ParentNodePtr)
					{
						NewNode->SetParent(*ParentNodePtr);
					}
				}
			}
		}

		// Remove the ChildActorComponent node from Blueprint
		ParentSCS->RemoveNode(TargetSCSNode);
		bBlueprintModified = true;
	}

	if (bBlueprintModified)
	{
		ParentBP->Modify();
		FKismetEditorUtilities::CompileBlueprint(ParentBP);
		UE_LOG(LogTemp, Log, TEXT("Successfully baked child actors and recompiled %s"), *ParentBP->GetName());
	}
}
