#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "QueueEvidenceCommandlet.generated.h"

UCLASS()
class UQueueEvidenceReceiver : public UObject
{
	GENERATED_BODY()
public:
	TArray<FString> Events;
	UFUNCTION() void Record(int32 Value);
	UFUNCTION() void RecordText(const FString& Value);
};

UCLASS()
class UQueueEvidenceCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UQueueEvidenceCommandlet();
	virtual int32 Main(const FString& Params) override;
};
