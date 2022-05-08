// Fill out your copyright notice in the Description page of Project Settings.


#include "Spin_Num.h"

// Sets default values
int ASpin_Num::Number1 = 0;
ASpin_Num::ASpin_Num()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ASpin_Num::Number1++;
	idNumber = ASpin_Num::Number1;
}

// Called when the game starts or when spawned
void ASpin_Num::BeginPlay()
{
	Super::BeginPlay();

//	ASpin_Num::Number1=1;
//	idNumber = ASpin_Num::Number1;
}

// Called every frame
void ASpin_Num::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

