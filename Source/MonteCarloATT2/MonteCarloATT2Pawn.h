// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#define MaxNumber 100

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
//#include <stdio.h>

#include "MonteCarloATT2Pawn.generated.h"
#define M_PI  3.14159265359
//class spin;
//class string;
using namespace std;

UCLASS(Config=Game)
class AMonteCarloATT2Pawn : public APawn
{
	GENERATED_BODY()

	/** StaticMesh component that will be the visuals for our flying pawn */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* PlaneMesh;

	/** Spring arm that will offset the camera */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArm;

	/** Camera component that will be our viewpoint */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;

	UPROPERTY(Category = "Input Spins", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> SpinsBP;


	virtual void BeginPlay() override;

	void SetStartingSpinRotations(bool& retflag);
	
	FRotator SetSpinRotation(float Teta, float Fi);

	bool MC_Modeling = false;

	float StartX = -5000.;
	float StartY = -5000.;
	float StartZ = 300.;
	float NewTime, LastTime = 0.f, DeltaTime = 0.1f;
	int CurrenRaw = 0;

	float Distance_between = 1000.;


	int ConvertRowToNumber(int Column,  int Row);

	void SynchronizeSpins();
	
	void TextOut(FString tText, int Number);

	double GetMagnetization();
	double GetMagnetizationSquare();

	FVector SetSpinLocation(int i, int j);

public:


	AMonteCarloATT2Pawn();
	void EraseAllSpins();

	// Begin AActor overrides
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
	// End AActor overrides

	UFUNCTION(BlueprintCallable)
		void MonteCarlo();

	UFUNCTION(BlueprintCallable)
		void SetParameters(float hstart_, float hend_, float hdelta_, float Anis_, float T_, float J_, float Jrow_, int Raws_, int RawsPerL_, int MaxX_, int MaxY_, bool Ising_);

protected:

	// Begin APawn overrides
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override; // Allows binding actions/axes to functions
	// End APawn overrides

	/** Bound to the thrust axis */
	void ThrustInput(float Val);
	
	/** Bound to the vertical axis */
	void MoveUpInput(float Val);

	/** Bound to the horizontal axis */
	void MoveRightInput(float Val);

private:

	/** How quickly forward speed changes */
	UPROPERTY(Category=Plane, EditAnywhere)
	float Acceleration;

	/** How quickly pawn can steer */
	UPROPERTY(Category=Plane, EditAnywhere)
	float TurnSpeed;

	/** Max forward speed */
	UPROPERTY(Category = Pitch, EditAnywhere)
	float MaxSpeed;

	/** Min forward speed */
	UPROPERTY(Category=Yaw, EditAnywhere)
	float MinSpeed;

	/** Current forward speed */
	float CurrentForwardSpeed;

	/** Current yaw speed */
	float CurrentYawSpeed;

	/** Current pitch speed */
	float CurrentPitchSpeed;

	/** Current roll speed */
	float CurrentRollSpeed;

public:
	/** Returns PlaneMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetPlaneMesh() const { return PlaneMesh; }
	/** Returns SpringArm subobject **/
	FORCEINLINE class USpringArmComponent* GetSpringArm() const { return SpringArm; }
	/** Returns Camera subobject **/
	FORCEINLINE class UCameraComponent* GetCamera() const { return Camera; }
};

class spin
{
public:

	spin();
	void reset();


	FRotator rotation()
	{


		FRotator Result(0., 0., 0.);


		Result.Roll = 0 + 180 / M_PI * acos(Sz);
		if (Sy != 0 )
		{
			Result.Yaw = 360 / M_PI * atan(Sx / Sy);
		//	Result.Yaw = 0;
		}
		else
		{

			Result.Yaw= 0;
		}


		return Result;
	}


	double Sz, Sx, Sy;
};

void CalculateRawAndColumn(int Number, int& Column, int& Row);


double EnergyCalc(const spin& Spin, int X, int Y);
//static void SetSpinRandomRotation(FRotator& Rotation);
void MonteCarloCalculations(int SpinNumber);
double EnergyCalcTotal();

void PrintSpins();
