// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject6.h"
#include "WallSlideObject.h"


// Sets default values
AWallSlideObject::AWallSlideObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AWallSlideObject::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWallSlideObject::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

