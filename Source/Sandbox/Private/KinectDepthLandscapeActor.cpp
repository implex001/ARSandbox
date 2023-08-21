// Fill out your copyright notice in the Description page of Project Settings.



#include "Sandbox/Public/KinectDepthLandscapeActor.h"


// Sets default values
AKinectDepthLandscapeActor::AKinectDepthLandscapeActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//setup component
	RawKinectComponent = CreateDefaultSubobject<URawKinectComponent>(TEXT("RawKinectComponent"));
	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("MeshComponent"));
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	
	MeshComponent->EditMesh([](FDynamicMesh3& Mesh)
	{
		int width = 128;
		int height = 128;
		//create mesh plane with 512x424 vertices
		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < height; j++)
			{
				FVector3d Vertex = FVector3d(i, j, 0);
				Mesh.AppendVertex(Vertex);
			}
		}
		
		//create the triangles
		for (int i = 0; i < width - 1; i++)
		{
			for (int j = 0; j < height - 1; j++)
			{
				Mesh.AppendTriangle(i + j * width, i + 1 + j * width, i + (j + 1) * width);
				Mesh.AppendTriangle(i + 1 + j * width, i + 1 + (j + 1) * width, i + (j + 1) * width);
			}
		}
			
	});
	MeshComponent->bEnableComplexCollision = true;
	MeshComponent->CollisionType = CTF_UseComplexAsSimple;

	//set collision preset to blockall
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

	MeshComponent->UpdateCollision(false);
}

// Called when the game starts or when spawned
void AKinectDepthLandscapeActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKinectDepthLandscapeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UDynamicMesh* Mesh = MeshComponent->GetDynamicMesh();
	FDynamicMesh3* MeshDynamic =  Mesh->GetMeshPtr();

	int width = 512;
	int height = 512;
	int finalWidth = 128;
	int finalheight= 128;

	TArray<int> averagedDepthData;
	averagedDepthData.Init(0, width*height);
	
	// 4x4 convolution on depth data
	for (int i=0; i< finalWidth; i++)
	{
		for (int j=0; j < finalheight; j++)
		{
			int sum = 0;
			for (int k=0; k < 4; k++)
			{
				for (int l=0; l < 4; l++)
				{
					if ((i*4) + k >= width || (j*4) + l >= 424)
					{
						continue;
					}
					sum += RawKinectComponent->KinectDevice->DepthData[((i*4) + k) + ((j*4) + l) * 424] * (1/16);
				}
			}
			averagedDepthData[i + j * finalWidth] = sum;
		}
	}

	for (int i = 0; i < finalWidth; i++)
	{
		for (int j = 0; j < finalheight; j++) 
		{
			FVector3d Vertex = FVector3d(i, j, RawKinectComponent->KinectDevice->DepthData[((i*4)) + ((j*4)) * 424]/ 100.0f);
			MeshDynamic->SetVertex(i + j * finalWidth, Vertex);
		}
	}
	MeshComponent->FastNotifyPositionsUpdated();
	MeshComponent->UpdateCollision(false);
}

