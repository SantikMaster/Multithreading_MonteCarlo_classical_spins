// Copyright Epic Games, Inc. All Rights Reserved.

#include "MonteCarloATT2Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

#include "Math.h"
#include <algorithm>
#include <future>
#include <thread>
#include "Async/Async.h"
#include "Async/AsyncWork.h"
#include <stdio.h>
#include <numeric>
#include "Spin_Num.h"

#define _USE_MATH_DEFINES 
#define triangle

using namespace std;

//FAsyncTask<FMyAsyncTask> MyTask;
spin S[MaxNumber][MaxNumber];
//vector<vector<spin>> S;

int MaxX = 10, MaxY = 10;
int Raws_Begin = 5;
vector<float> Magnatization;
vector<float> MagnatizationSquare;

double J = -0.5, Jrow = -0.5;
float h = -1, Ansotropy = -1.75; // to Edit
double hstart = 0, hend= -1.75, hdelta = 0.1;
int Raws = 10,  RawsPerL = 1;
double T = 0.0001;        // to Edit
bool Processing = false;
bool Ising = false;
spin  NewSpin;
void TextOut(std::string tText, int Number);
AMonteCarloATT2Pawn::AMonteCarloATT2Pawn()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		FConstructorStatics()
			: PlaneMesh(TEXT("/Game/Flying/Meshes/UFO.UFO"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	// Create static mesh component
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh0"));
	PlaneMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());	// Set static mesh
	RootComponent = PlaneMesh;

	// Create a spring arm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
	SpringArm->SetupAttachment(RootComponent);	// Attach SpringArm to RootComponent
	SpringArm->TargetArmLength = 160.0f; // The camera follows at this distance behind the character	
	SpringArm->SocketOffset = FVector(0.f,0.f,60.f);
	SpringArm->bEnableCameraLag = false;	// Do not allow camera to lag
	SpringArm->CameraLagSpeed = 15.f;

	// Create camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);	// Attach the camera
	Camera->bUsePawnControlRotation = false; // Don't rotate camera with controller

	// Set handling parameters
	Acceleration = 500.f;
	TurnSpeed = 50.f;
	MaxSpeed = 4000.f;
	MinSpeed = 0.f;
	CurrentForwardSpeed = 500.f;
	ASpin_Num::Number1 = 0;
	EraseAllSpins();////  to Edit
}


void AMonteCarloATT2Pawn::Tick(float DeltaSeconds)
{
	const FVector LocalMove = FVector(CurrentForwardSpeed * DeltaSeconds, 0.f, 0.f);

	// Move plan forwards (with sweep so we stop when we collide with things)
	AddActorLocalOffset(LocalMove, true);

	// Calculate change in rotation this frame
	FRotator DeltaRotation(0,0,0);
	DeltaRotation.Pitch = CurrentPitchSpeed * DeltaSeconds;
	DeltaRotation.Yaw = CurrentYawSpeed * DeltaSeconds;
	DeltaRotation.Roll = CurrentRollSpeed * DeltaSeconds;

	// Rotate plane
	AddActorLocalRotation(DeltaRotation);

	// Call any parent class Tick implementation
	Super::Tick(DeltaSeconds);


	NewTime = FPlatformTime::Seconds();


		if ( (MC_Modeling) 
			&&	(!Processing) 
			)
		{
			this->SynchronizeSpins();
	//		auto A = std::async(std::launch::async, [&]()
	//		AsyncTask(ENamedThreads::BackgroundThreadPriority
			Async(EAsyncExecution::Thread, [&]()
			{
			Processing = true;
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("Process started, raw %i,  h = %f"), CurrenRaw, h));

	//		auto MyTask = new FAsyncTask<FMyAsyncTask>(0); // to Edit
	//		MyTask->StartBackgroundTask();
    //      MyTask->EnsureCompletion();
			MonteCarloCalculations(100);// Work!!
			CurrenRaw++;
			ensure(Raws > Raws_Begin);
			if ( (CurrenRaw > Raws_Begin))
			{
				Magnatization.push_back(GetMagnetization());
				MagnatizationSquare.push_back(GetMagnetizationSquare());
			}
		
			if (CurrenRaw >= Raws + 1)
			{

				FString result;
				double M, Msq;
				M = std::accumulate(Magnatization.begin(), Magnatization.end(), 0);
				Msq = std::accumulate(MagnatizationSquare.begin(), MagnatizationSquare.end(), 0);
				if (Magnatization.size()!=0)
				{
					M /= Magnatization.size();
					Msq /= Magnatization.size();
				}
				Magnatization.clear();
				MagnatizationSquare.clear();

				double Susceptability = 1 / T * (Msq - M * M);

				result = FString::SanitizeFloat(h);
				result +=  "      ";
				result += FString::SanitizeFloat(M);
				result += "      " + FString::SanitizeFloat(Susceptability);
				result = result + "    \n";

				FString file = FPaths::ProjectConfigDir();
				file.Append(TEXT("Data.txt"));

				if (FFileHelper::SaveStringToFile(result, *file, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append))
				{
					UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written to the text file"));
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed"));


				}

				CurrenRaw = 1;
				EraseAllSpins();
				h = h + hdelta;
   

			}
		
		
			
		
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("Process ended")));

			}
			);
		}

/**/

}
double AMonteCarloATT2Pawn::GetMagnetization()
{
	double M = 0;
	for (int i = 0; i < MaxX; i++)
	{
		for (int j = 0; j < MaxY; j++)
		{
			M += S[i][j].Sz;
		}


	}
	return M;
}
double AMonteCarloATT2Pawn::GetMagnetizationSquare()
{
	double Msq = 0;
	for (int i = 0; i < MaxX; i++)
	{
		for (int j = 0; j < MaxY; j++)
		{
			Msq += S[i][j].Sz*S[i][j].Sz;
		}


	}
	return Msq;
}


void FMyAsyncTask::DoWork()
{
	
	GEngine->AddOnScreenDebugMessage(-5, 5.f, FColor::Red, FString::Printf(TEXT("Async Begin")));
	//
	//FPlatformProcess::Sleep(3);
     MonteCarloCalculations(Actors);
	GEngine->AddOnScreenDebugMessage(-5,5.f, FColor::Red, FString::Printf(TEXT("Async Task Started Actors: %i"), Actors));

}


UFUNCTION(BlueprintCallable)
void AMonteCarloATT2Pawn::SetParameters(float hstart_, float hend_, float hdelta_, float Anis_, float T_, float J_, float Jrow_, int Raws_, int RawsPerL_, int MaxX_, int MaxY_, bool Ising_)
{
	
//	double h = 1, Ansotropy = -1.75; // to Edit
	hstart = hstart_;
	hend = hend_;
	hdelta = hdelta_;
	Ansotropy = Anis_;
	T = T_;
	J = J_;
	Jrow = Jrow_;
	Raws = Raws_,	
	RawsPerL = RawsPerL_;

	MaxX = MaxX_;
	MaxY = MaxY_;

	Ising = Ising_;

	h = hstart;

	bool retflag;
	SetStartingSpinRotations(retflag);
	if (retflag) return;
	UE_LOG(LogTemp, Warning, TEXT("Set Parameters"));
}


void AMonteCarloATT2Pawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Deflect along the surface when we collide.
	FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FQuat::Slerp(CurrentRotation.Quaternion(), HitNormal.ToOrientationQuat(), 0.025f));
}


void AMonteCarloATT2Pawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    // Check if PlayerInputComponent is valid (not NULL)
	check(PlayerInputComponent);

	// Bind our control axis' to callback functions
	PlayerInputComponent->BindAxis("Thrust", this, &AMonteCarloATT2Pawn::ThrustInput);
	PlayerInputComponent->BindAxis("MoveUp", this, &AMonteCarloATT2Pawn::MoveUpInput);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMonteCarloATT2Pawn::MoveRightInput);
}

void AMonteCarloATT2Pawn::ThrustInput(float Val)
{
	// Is there any input?
	bool bHasInput = !FMath::IsNearlyEqual(Val, 0.f);
	// If input is not held down, reduce speed
	float CurrentAcc = bHasInput ? (Val * Acceleration) : (-0.5f * Acceleration);
	// Calculate new speed
	float NewForwardSpeed = CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * CurrentAcc);
	// Clamp between MinSpeed and MaxSpeed
	CurrentForwardSpeed = FMath::Clamp(NewForwardSpeed, MinSpeed, MaxSpeed);
}

void AMonteCarloATT2Pawn::MoveUpInput(float Val)
{
	// Target pitch speed is based in input
	float TargetPitchSpeed = (Val * TurnSpeed * -1.f);

	// When steering, we decrease pitch slightly
	TargetPitchSpeed += (FMath::Abs(CurrentYawSpeed) * -0.2f);

	// Smoothly interpolate to target pitch speed
	CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, TargetPitchSpeed, GetWorld()->GetDeltaSeconds(), 2.f);
}

void AMonteCarloATT2Pawn::MoveRightInput(float Val)
{
	// Target yaw speed is based on input
	float TargetYawSpeed = (Val * TurnSpeed);

	// Smoothly interpolate to target yaw speed
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, TargetYawSpeed, GetWorld()->GetDeltaSeconds(), 2.f);

	// Is there any left/right input?
	const bool bIsTurning = FMath::Abs(Val) > 0.2f;

	// If turning, yaw value is used to influence roll
	// If not turning, roll to reverse current roll value.
	float TargetRollSpeed = bIsTurning ? (CurrentYawSpeed * 0.5f) : (GetActorRotation().Roll * -2.f);

	// Smoothly interpolate roll speed
	CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, TargetRollSpeed, GetWorld()->GetDeltaSeconds(), 2.f);
}
void AMonteCarloATT2Pawn::BeginPlay()
{
	Super::BeginPlay();


	InputComponent = GetOwner()->FindComponentByClass<UInputComponent>();
	if (!InputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("No Input "));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Input "));
		InputComponent->BindAction("StartModelling", IE_Pressed, this, &AMonteCarloATT2Pawn::MonteCarlo);


	}

	

	LastTime = FPlatformTime::Seconds();


}
void AMonteCarloATT2Pawn::SetStartingSpinRotations(bool& retflag)
{
	retflag = true;
	FVector Location(100.f, 100.f, 300.f);
	FRotator Rotation;

	float Fi,  Teta, cosTeta;

	if (SpinsBP == NULL)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("CLASS == NULL")));
		return;
	}

	for (int i = 0; i< MaxX; i++)
	{
		for (int j = 0; j < MaxY; j++)
		{
			Fi = (float)(std::rand() % 360);
				cosTeta = (float)(std::rand() % 2000 - 1000) * 0.001;

				Teta = 180 / M_PI * acos(cosTeta); //radians to grad
				if (Ising == true)
				{
				    Teta = 180 * (std::rand() % 2);
				//	Teta = 180;
				}

			Rotation = SetSpinRotation(Teta, Fi);
	//			UE_LOG(LogTemp, Warning, TEXT("Start!  Pith  %f,  Roll %f , Yaw %f"), Rotation.Pitch, Rotation.Roll, Rotation.Yaw);


#ifdef square
			Location = FVector(StartX + i * Distance_between, StartY + j * Distance_between, StartZ);
			GetWorld()->SpawnActor<AActor>(SpinsBP, Location, Rotation);
#endif square

#ifdef triangle

			enum SpinPosition
			{

				FirstTriangleLeft,
				FirstTriangleRight,
				FirstTriangleUp,
				SecondTriangleLeft,
				SecondTriangleRight,
				SecondTriangleDown,
			};
			SpinPosition Position;
		//	int X = (i + 1), Y = j + 1;

		//	float Test = 10000;
			if ((j % 2) == 1)
			{
				switch (i % 6)
				{
				case 0:
				{
					Position = FirstTriangleLeft;
					Location = FVector(StartX + 0, StartY + 0, StartZ);
				}break;
				case 1:
				{
					Position = FirstTriangleRight;
					Location = FVector(StartX + Distance_between, StartY + 0, StartZ);
				}break;
				case 2:
				{
					Position = FirstTriangleUp;
					Location = FVector(StartX + Distance_between / 2, StartY + Distance_between * sqrt(2) / 2, StartZ);
				}break;
				case 3:
				{
					Position = SecondTriangleLeft;
					Location = FVector(StartX + 2 * Distance_between, StartY + 0, StartZ);
				}break;
				case 4:
				{
					Position = SecondTriangleRight;
					Location = FVector(StartX + 3 * Distance_between, StartY + 0, StartZ);
				}break;
				case 5:
				{
					Position = SecondTriangleDown;
					Location = FVector(StartX + 2 * Distance_between + Distance_between / 2, StartY - Distance_between * sqrt(2) / 2, StartZ);
				}break;
				}
			}
			if ((j % 2) == 0)
			{
				switch (i % 6)
				{
				case 0:
				{
					Position = SecondTriangleLeft;
					Location = FVector(StartX + 0, StartY+ 3 * Distance_between + 0, StartZ );
				}break;
				case 1:
				{
					Position = SecondTriangleRight;
					Location = FVector(StartX + Distance_between , StartY + 0+ 3 * Distance_between, StartZ);
				}break;
				case 2:
				{
					Position = SecondTriangleDown;
					Location = FVector(StartX + Distance_between / 2, StartY+ 3 * Distance_between - Distance_between * sqrt(2) / 2, StartZ );

				}break;
				case 3:
				{
					Position = FirstTriangleLeft;
					Location = FVector(StartX + 2 * Distance_between, StartY + 3 * Distance_between + 0, StartZ);
				}break;
				case 4:
				{
					Position = FirstTriangleRight;
					Location = FVector(StartX + 3 * Distance_between, StartY + 3 * Distance_between + 0, StartZ);
				}break;
				case 5:
				{
					Position = FirstTriangleUp;
					Location = FVector(StartX + 2 * Distance_between + Distance_between / 2, StartY  + 3 * Distance_between+ Distance_between * sqrt(2) / 2, StartZ);

				}break;
				}
			}
			FVector SiteLocation = FVector((i / 6) * 4 * Distance_between, (j / 2) * 6 * Distance_between, 0);

			Location += SiteLocation;

			//		Location = FVector(StartX + i * Distance_between, StartY + j * Distance_between*3 , StartZ);
			GetWorld()->SpawnActor<AActor>(SpinsBP, Location, Rotation);
#endif triangle

		}

	}
	retflag = false;
}
void SetSpinRandomRotation(FRotator &Rotation)
{
	double Fi = (float)(std::rand() % 360);
	double cosTeta = (float)(std::rand() % 2000 - 1000) * 0.001;
	double Teta = 180 / M_PI * acos(cosTeta); //radians to grad
	if (Ising == true)
	{
    	Teta = M_PI * (std::rand() % 2);
	}

	//*Rotation = FRotator(Teta, Fi, 0.f);
//	Rotation = FRotator(0.f, 180.f, 180.f);
	Rotation = FRotator(0.f, Teta, Fi);

	// 
//	UE_LOG(LogTemp, Warning, TEXT("Pith  %f,  Roll %f , Yaw %f"), Rotation.Pitch, Rotation.Roll, Rotation.Yaw);

}
FRotator AMonteCarloATT2Pawn::SetSpinRotation(float Teta, float Fi)
{
//*	FRotator Rotation(Teta, -Fi, 0.f);
	FRotator Rotation(0.f, Teta, Fi);

	return Rotation;
}

UFUNCTION(BlueprintCallable)
void AMonteCarloATT2Pawn::MonteCarlo()
{



	MC_Modeling = !MC_Modeling;
	

}
void MonteCarloCalculations(int Actors1)
{


	UE_LOG(LogTemp, Warning, TEXT("Anisotropy %f,  h %f"), Ansotropy, h);

	// S1 on S 
	GEngine->AddOnScreenDebugMessage(1, 1.f,FColor::Black,"Start");
	int SpinNumber;
	int Steps = 1000000;//*
	UE_LOG(LogTemp, Warning, TEXT("MC"));

	int X = 0, Y = 0;
	int Actors = MaxX * MaxY; // to Edit

double OldEnergy = 0, NewEnergy = 0;
	double W, probe;// w - the probabitity of temperature transition



	for (int i = 0; i < Steps; i++)
	{
		SpinNumber = std::rand() % Actors;
		CalculateRawAndColumn(SpinNumber, X, Y);
	//	UE_LOG(LogTemp, Warning, TEXT("Sz:   %f"), S[X - 1][Y - 1].Sz);
		NewSpin.reset();
		OldEnergy = EnergyCalc(S[X - 1][Y - 1], X, Y);
		NewEnergy = EnergyCalc(NewSpin, X, Y);

//		UE_LOG(LogTemp, Warning, TEXT("OldEnergy   %f,  NewEnergy   %f"), OldEnergy, NewEnergy);
		if ((NewEnergy - OldEnergy) <= 0)//>
		{
			ensure(X >= 1 && X <= MaxX);
			ensure(Y >= 1 && Y <= MaxY);
			S[X - 1][Y - 1] = NewSpin;

			double Energy = EnergyCalcTotal();
	//		UE_LOG(LogTemp, Warning, TEXT("Anisotropy %f,  h %f"), Ansotropy, h);

		}
		else
		{
			W = exp(-(NewEnergy - OldEnergy) / T);

			probe = (rand() % 1001) * 0.001;
			if (probe < W)
			{

				S[X - 1][Y - 1] = NewSpin;
			}
			// if new energy is lower then    Old spin  = new spin 
	//		UE_LOG(LogTemp, Warning, TEXT("line 2 X  %i,   Y %i"), X, Y);
		}
	}
	Processing = false;
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("Process ended calculation")));

}


void  AMonteCarloATT2Pawn::EraseAllSpins()
{
	int X, Y;
	for (X = 1; X < MaxX+1; X++)
	{
		for (Y = 1; Y < MaxY+1; Y++)
		{
			S[X-1][Y-1].reset();
		}
	}

}

double EnergyCalcTotal()
{
	int X = 0, Y = 0;
	double Energy = 0;
	for (X = 1; X < MaxX + 1; X++)
	{
		for (Y = 1; Y < MaxY + 1; Y++)
		{

			Energy += EnergyCalc(S[X-1][Y-1], X , Y );
		}
	}
	return  Energy;
}

void PrintSpins()
{

	int X = 0, Y = 0;

	for (X = 1; X< MaxX + 1; X++)
	{
		for (Y = 1; Y < MaxY + 1; Y++)
		{

		//	UE_LOG(LogTemp, Warning, TEXT("Syncronization Z  %f,   X %f"), S[X - 1][Y - 1].Sz, S[X - 1][Y - 1].Sx);

		}
	}

}

double EnergyCalc(const spin &Spin, int X, int Y)
{
	double Energy = 0;

	ensure(X >= 1 && X <= MaxX);
	ensure(Y >= 1 && Y <= MaxY);

#ifdef square

 // square

	if (X< MaxX )
	{
		Energy += J * Spin.Sz * S[X ][Y-1].Sz;
		Energy += J * Spin.Sx * S[X ][Y-1].Sx;
		Energy += J * Spin.Sy * S[X ][Y-1].Sy;

	}
	if (Y < MaxY)
	{
		Energy += J * Spin.Sz * S[X-1][Y ].Sz;
		Energy += J * Spin.Sx * S[X-1][Y ].Sx;
		Energy += J * Spin.Sy * S[X-1][Y ].Sy;
	}
	if (X > 1)
	{
		Energy += J * Spin.Sz * S[X-2 ][Y-1].Sz;
		Energy += J * Spin.Sx * S[X-2 ][Y-1].Sx;
		Energy += J * Spin.Sy * S[X-2 ][Y-1].Sy;
	}
	if (Y > 1)
	{
		Energy += J * Spin.Sz * S[X-1][Y - 2].Sz;
		Energy += J * Spin.Sx * S[X-1][Y - 2].Sx;
		Energy += J * Spin.Sy * S[X-1][Y - 2].Sy;
	}


	Energy += -h * Spin.Sz - Ansotropy* Spin.Sz * Spin.Sz;
#endif square
 
#ifdef triangle

	enum SpinPosition
	{

		FirstTriangleLeft,
		FirstTriangleRight,
		FirstTriangleUp,
		SecondTriangleLeft,
		SecondTriangleRight,
		SecondTriangleDown,
	};
	SpinPosition Position;

	if (Y % 2 == 1)
	{
		switch ((X-1) % 6)
		{ 
		case 0:
		{
			Position = FirstTriangleLeft;
		}break;
		case 1:
		{
			Position = FirstTriangleRight;
		}break;
		case 2:
		{
			Position = FirstTriangleUp;
		}break;
		case 3:
		{
			Position = SecondTriangleLeft;
		}break;
		case 4:
		{
			Position = SecondTriangleRight;
		}break;
		case 5:
		{
			Position = SecondTriangleDown;
		}break;
		}
	}
	if (Y % 2 == 0)
	{
		switch ((X - 1) % 6)
		{
		case 0:
		{
			Position = SecondTriangleLeft;
		}break;
		case 1:
		{
			Position = SecondTriangleRight;
		}break;
		case 2:
		{
			Position = SecondTriangleDown ;
		}break;
		case 3:
		{
			Position = FirstTriangleLeft;
		}break;
		case 4:
		{
			Position = FirstTriangleRight;
		}break;
		case 5:
		{
			Position = FirstTriangleUp;
		}break;
		}
	}

	if (X < MaxX)
	{
		Energy += J * Spin.Sz * S[X][Y - 1].Sz;
		Energy += J * Spin.Sx * S[X][Y - 1].Sx;
		Energy += J * Spin.Sy * S[X][Y - 1].Sy;

	}

	if (X > 1)
	{
		Energy += J * Spin.Sz * S[X - 2][Y - 1].Sz;
		Energy += J * Spin.Sx * S[X - 2][Y - 1].Sx;
		Energy += J * Spin.Sy * S[X - 2][Y - 1].Sy;
	}

	switch (Position)
	{
		case (FirstTriangleUp):
		{
			if ((Y < MaxY))
			{
				Energy += Jrow * Spin.Sz * S[X - 1][Y].Sz;
				Energy += Jrow * Spin.Sx * S[X - 1][Y].Sx;
				Energy += Jrow * Spin.Sy * S[X - 1][Y].Sy;
			}
		};
		case (SecondTriangleDown):
		{
			if (Y > 1)
	        {
		
				Energy += Jrow * Spin.Sz * S[X - 1][Y-2].Sz;
				Energy += Jrow * Spin.Sx * S[X - 1][Y-2].Sx;
				Energy += Jrow * Spin.Sy * S[X - 1][Y-2].Sy;
	        }
		}
		case FirstTriangleLeft:
		{
			if ((X < MaxX - 1))
			{
				Energy += J * Spin.Sz * S[X + 1][Y - 1].Sz;
				Energy += J * Spin.Sx * S[X + 1][Y - 1].Sx;
				Energy += J * Spin.Sy * S[X + 1][Y - 1].Sy;
			}
		}
		case FirstTriangleRight:
		{
			if ((X < 2))
			{
				Energy += J * Spin.Sz * S[X - 3][Y - 1].Sz;
				Energy += J * Spin.Sx * S[X - 3][Y - 1].Sx;
				Energy += J * Spin.Sy * S[X - 3][Y - 1].Sy;
			}
		}
		case SecondTriangleLeft:
		{
			if ((X < MaxX - 1))
			{
				Energy += J * Spin.Sz * S[X + 1][Y - 1].Sz;
				Energy += J * Spin.Sx * S[X + 1][Y - 1].Sx;
				Energy += J * Spin.Sy * S[X + 1][Y - 1].Sy;
			}
		}
		case SecondTriangleRight:
		{
			if ((X < 2))
			{
				Energy += J * Spin.Sz * S[X - 3][Y - 1].Sz;
				Energy += J * Spin.Sx * S[X - 3][Y - 1].Sx;
				Energy += J * Spin.Sy * S[X - 3][Y - 1].Sy;
			}
		}
	}
	
	

	Energy += -h * Spin.Sz - Ansotropy * Spin.Sz * Spin.Sz;
#endif triangle
	return Energy;

}
void AMonteCarloATT2Pawn::SynchronizeSpins()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), SpinsBP, FoundActors);//	GetAllActors

	int X = 0, Y = 0;
	int jj;

	for ( jj = 0; jj < FoundActors.Num(); jj++)
	{
		CalculateRawAndColumn(jj, X, Y);

//		UE_LOG(LogTemp, Warning, TEXT("Syncronization Yaw %f, Syncronization Roll %f,  SZ  %f  , Sx %f, Sy   %f"),
//			S[X - 1][Y - 1].rotation().Yaw, S[X - 1][Y - 1].rotation().Roll,
//			S[X - 1][Y - 1].Sz, S[X - 1][Y - 1].Sx, S[X - 1][Y - 1].Sy	);
		FoundActors[jj]->SetActorRotation(S[X-1][Y-1].rotation());  // to Edit

	}
	UE_LOG(LogTemp, Warning, TEXT("Syncronizaton"));

}

int AMonteCarloATT2Pawn::ConvertRowToNumber(int Column, int Row)
{

	int rez;

	rez = ((Row-1) * MaxX) + (Column-1); /// is result correct??
	return rez;
}
void CalculateRawAndColumn(int Number, int &Column, int &Row)
{

	//  column = 1 ... MaxNumb, Row column = 1  
	Row = Number / MaxX+1;
	Column = Number % MaxX+1;
	
}

spin::spin()
{
	FRotator Rot(0.f, 0.f , 0.f);
//	UE_LOG(LogTemp, Warning, TEXT("Pith  %i,  Roll %i"), Rot.Pitch, Rot.Roll);
//	UE_LOG(LogTemp, Warning, TEXT("Pith  %f,  Roll %f"), Rot.Pitch, Rot.Roll);
	SetSpinRandomRotation(Rot);

//*	Sz = cos(Rot.Yaw);
//*	Sx = sin(Rot.Yaw) * cos(Rot.Pitch);
//*	Sy = sin(Rot.Yaw) * sin(Rot.Pitch);
	Sz = cos(Rot.Roll * M_PI / 180);
	Sx = sin(Rot.Roll * M_PI / 180) * cos(Rot.Yaw * M_PI / 180);
	Sy = sin(Rot.Roll * M_PI / 180) * sin(Rot.Yaw * M_PI / 180);


//	UE_LOG(LogTemp, Warning, TEXT("Z  %f,  Y   %f,   X %f"), Sz, Sy, Sx);

}
void spin::reset()
{
	FRotator Rot(0.f, 0.f, 0.f);
	//	UE_LOG(LogTemp, Warning, TEXT("Pith  %i,  Roll %i"), Rot.Pitch, Rot.Roll);
	//	UE_LOG(LogTemp, Warning, TEXT("Pith  %f,  Roll %f"), Rot.Pitch, Rot.Roll);
	SetSpinRandomRotation(Rot);


//*	Sz = cos(Rot.Yaw);
//*	Sx = sin(Rot.Yaw) * cos(Rot.Pitch);
//*	Sy = sin(Rot.Yaw) * sin(Rot.Pitch);
	Sz = cos(Rot.Roll * M_PI/180);
	Sx = sin(Rot.Roll * M_PI / 180) * cos(Rot.Yaw * M_PI / 180);
	Sy = sin(Rot.Roll * M_PI / 180) * sin(Rot.Yaw * M_PI / 180);

//	UE_LOG(LogTemp, Warning, TEXT("rotat Yaw %f, rotat Roll %f,  SZ  %f  , Sx %f, Sy   %f"),
//		Rot.Yaw, Rot.Roll,
//		Sz, Sx, Sy);


}
void AMonteCarloATT2Pawn::TextOut(FString Text, int Number)
{

	FILE* stream;
	UE_LOG(LogTemp, Warning, TEXT("to file:  "));

	char Stri[25];


	stream =
		fopen("DataEnh.txt", "a");
	fopen(Stri, "a");

	
	//  "Data.txt"
	char* p;
	p = TCHAR_TO_ANSI(*Text);
	if (stream != NULL)
	{
		fprintf(stream, p, "DataEnh.txt");

		fclose(stream);
	}
	return;
}

/*
void MonteCarlo()
{
	int i, j;
	double OldEnergy, NewEnergy;
	long double W;
	i = rand() % SIZE;
	j = rand() % SIZE;
	OldEnergy = EnergyCalc(i, j);
	S[i][j].Sz = -1 * S[i][j].Sz;
	NewEnergy = EnergyCalc(i, j);
	if (OldEnergy < NewEnergy)
	{
		S[i][j].Sz = -1 * S[i][j].Sz;

		W = exp(-(NewEnergy - OldEnergy) / T);

		if ((rand() % 1001) / 1000 < W)
		{
			S[i][j].Sz = -1 * S[i][j].Sz;
		}
	}



}
double EnergyCalc(int i, int j)
{
double Energy = 0, dist;

	if (i!= SIZE)
	{
		Energy+= J[i][j][0]*S[i][j].Sz*S[i+1][j].Sz;
	}
	if (j!= SIZE)
	{
		Energy+= J[i][j][1]*S[i][j].Sz*S[i][j+1].Sz;
	}
	if (i!= 0)
	{
		Energy+= J[i-1][j][0]*S[i][j].Sz*S[i-1][j].Sz;
	}
	if (j!= 0)
	{
		Energy+= J[i][j-1][1]*S[i][j].Sz*S[i][j-1].Sz;
	}

		int ii, jj;
	for (ii = 0; ii<SIZE; ii++)
	{
		for (jj = 0; jj<SIZE; jj++)
		{
			dist = distance (S[i][j], S[ii][jj]);
			if (dist!=0) Energy += J_d/dist/dist*S[i][j].Sz*S[ii][jj].Sz;
		}
   }
Energy -= h*S[i][j].Sz;

return Energy;

}
void TextOut(string Text, int Number)
{

	// #include <stdio.h>

	FILE	 	*stream;
	char str[10];


	char Stri[25];


	stream =
		fopen("DataEnh.txt",  "a");
fopen(Stri,  "a");


	//  "Data.txt"
	char * p;
	p = (char *)Text.c_str();
	if (stream != NULL)
	{
		fprintf(stream, p, "DataEnh.txt");

		fclose(stream);
	}
	return;
}

*/

