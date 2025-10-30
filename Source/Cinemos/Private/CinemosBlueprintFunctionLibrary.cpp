// Copyright Lost Cosmonauts

#include "CinemosBlueprintFunctionLibrary.h"

#include "Tracks/MovieSceneSubTrack.h"

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
