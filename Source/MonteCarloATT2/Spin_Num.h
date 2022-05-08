// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spin_Num.generated.h"

UCLASS()
class MONTECARLOATT2_API ASpin_Num : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpin_Num();
	static int Number1;

	UPROPERTY(Category = "User", EditAnywhere)
		int idNumber;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
