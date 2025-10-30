// Copyright Lost Cosmonauts

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "Misc/FrameNumber.h"

#include "CinemosBlueprintFunctionLibrary.generated.h"

class UMovieSceneSequence;
class UMovieSceneSubTrack;
class UMovieSceneSequence;
class UMovieSceneSubSection;
class UDataTable;

/**
 * Blueprint utility functions for Cinemos.
 */
UCLASS()
class CINEMOS_API UCinemosBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sequencer")
	static UMovieSceneSubSection* AddSubSequenceToSubTrack(UMovieSceneSubTrack* SubTrack, UMovieSceneSequence* Sequence, FFrameNumber StartTime, int32 Duration);

	UFUNCTION(BlueprintCallable, Category = "Sequencer")
	static UMovieSceneSubSection* AddSubSequenceToSubTrackRow(UMovieSceneSubTrack* SubTrack, UMovieSceneSequence* Sequence, FFrameNumber StartTime, int32 Duration, int32 RowIndex);

	UFUNCTION(BlueprintCallable, Category = "Data Table")
	static void ClearDataTable(UDataTable* DataTable);

	UFUNCTION(BlueprintCallable, Category = "Map")
	static bool IsPartsMapEqual(TMap<FName, bool> A, TMap<FName, bool> B);
};