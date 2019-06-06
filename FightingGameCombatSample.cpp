/*
    @Copywrite: C Dylan Pfenning 2019
    @Author: Dylan Pfenning
    All of the following code was written for Unreal Engine 4.21 
    Header Files are available if you'd like to see them however they're pretty standard.
    Assume all #includes are present and correct.
*/

/* 
    This files is specific to combat code. IE; throwing out moves, adjusting hitboxes and registering hits.
    All code is up to date for the project as of June 7th 2019
*/




//In unreal you must validate before implementation of server side code. The validate method for TakeHit returned true by default.

void AFighterBase::TakeHit_Implementation(UPrimitiveComponent* HitComp, AFighterBase* HitBy, EHitPower power, float DamageToDo)
{

    //Makeing sure its actually a thing we're hitting
	if (this != nullptr)
	{
		//Check if I am dodging
		if (bIsDodging)
		{
            //if I am dodging then we just return no damage is done
			return;
		}
		else
		{
            //Because knockback ALWAYS sends us upwards we get hit into the air.
			SM->SetCurrentState(ECharacterStates::HIT_AIR);

            //Apply damage
			Health -= DamageToDo;

			/* 
              Start the KB 
            */

           //Creating a vector to add as force to the character (Getting hit..) 
           //X = 0 as we're working in YZ space for the game (Z being up in unreal)
			FVector launchVec = FVector(0.0f, KnockbackDistance, KnockbackDistance) * HitBy->GetActorForwardVector();
            //Because we multiplied by the other actors forward vector the character will go up and away from the player who hit him.

            
            //giving the launch a z vecor as the multiplication will make Z = 0;
			launchVec.Z += KnockbackHeight;

            //Launching the character-> Syntax for this is (FVector, XY Override, Z Override) for forcing launch velocities
			this->LaunchCharacter(launchVec, false, true);
			
		}
	}
}

/*The following is from a class that inherits from the fighter base to do it own stuff. */

//Overlap is in here because some characters need to do specific things 
void ACustomFighter::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& sweepResult)
{
    //Grabing my custom collider component
	UHitboxComponent* MyHittingComp = Cast<UHitboxComponent>(OverlappedComp);
    /*
        I should mention the only "special" thing I did in the custom comp was added a way to swap between a hit and hurtbox whenever I want to    
     */
	if (MyHittingComp != nullptr)
	{
        //Checking to see if my comp can do damage.
		if (MyHittingComp->GetCanDamage())
		{
            //Making sure its me throwing out the moves
			if (bIsHitting)
			{
				if (OtherActor != this && OtherActor != nullptr)
				{
                    //Calling the server overlap function to handle server side code.
					DoOverlap(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, sweepResult);
					UWorld* World = GetWorld();
				}
			}
		}
	}
}

//The overlap implementation on the server. ->As said above unreal requires a validate + implementation. assume validate returns true in this case again.
void ACustomFighter::DoOverlap_Implementation(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& sweepResult)
{
    //Making sure that its not double overlaping
	if (bHasHit == false)
	{
        //Double checking to make sure the hitbox comp still exists. Usually not needed but Unreal is Unreal sometimes...
		UHitboxComponent* MyHittingComp = Cast<UHitboxComponent>(OverlappedComp);
		if (MyHittingComp != nullptr)
		{
            //Grab the other fighter
            AFighterBase* OtherFighter = Cast<AFighterBase>(OtherActor);
            //CHeck to see what attack was hit with
            if (bLightAtk || bLightAtkAir)
            {
                //Light does lowest damage and knockback
                OtherFighter->HitChar(OverlappedComp, this, EHitPower::LIGHT, Damage);
                bHasHit = true;
                //Add damage and attacks dones-> This is for the aggression calculation.
                DamageDone += Damage;
                AttacksHit++;
            }
            else if (bMedAtk || bMedAtkAir)
            {
                //Medium is the strong version of the attack more damage more power ect...
                OtherFighter->HitChar(OverlappedComp, this, EHitPower::MEDIUM, Damage * 1.25f);
                bHasHit = true;
                DamageDone += Damage * 1.25f;
                AttacksHit++;
            }
		}
	}

}

/*Quick Hibox activation and enable/disable animations code. */
//Both of these get called from animation triggers in engine. Currently working on a way to make this more general so I don't need to change them ever.
void ACustomFighter::EnableLightHitbox()
{
	
	LeftArm->SetCanDamage(true);
	LeftHand->SetCanDamage(true);
}

void ACustomFighter::DisableLightHitbox()
{
	LeftArm->SetCanDamage(false);
	LeftHand->SetCanDamage(false);
}

//Just code for making a light attack play and replicate on the server..

void ACustomFighter::LightAtk()
{
    /*All this code will be done on the client.*/

    //Check the sate machine to pick the right animation and change to the attacking state
	if (SM->GetCurrentState() == ECharacterStates::GROUND)
	{
		SM->SetCurrentState(ECharacterStates::ATK_GROUND);
		bLightAtk = true;
	}
	else if (SM->GetCurrentState() == ECharacterStates::AIR)
	{
		SM->SetCurrentState(ECharacterStates::ATK_AIR);
		bLightAtkAir = true;
	}
    //Stop movement input. Creates a nice slide into the attack
	Client_StopMovement();
    //Setting the animation to play on the server via a server side toggle command.
	Server_LightToggle(SM->GetCurrentState(), true);
    
    //Finally the animation time. This call the OnLightAtkEnd function to set everything back to default.
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
        //all timer values (lightAnim,Length) set up so that designers/animators can adjust the length of the played animation in code
		World->GetTimerManager().SetTimer(TimerHandle_LightPunch, this, &ADemarkBase::Client_OnLightAttackEnd, LightAnimLength);
	}
    //add to attacks thrown for aggression
	AttacksThrown++;
    //Let me know im trying to hit things.
	bIsHitting = true;

}

/*This is called at the end of the light attack animations*/
void ACustomFighter::Client_OnLightAttackEnd_Implementation()
{
    //Resetting bools
	bLightAtk = false;
	bLightAtkAir = false;
	bIsHitting = false;
	bHasHit = false;
    //Resetting statemachine to the proper state
	if (SM->GetCurrentState() == ECharacterStates::ATK_GROUND)
	{
		SM->SetCurrentState(ECharacterStates::GROUND);
        //resetting values on the server side
		Server_LightToggle(ECharacterStates::ATK_GROUND, false);
	}
	else
	{
		SM->SetCurrentState(ECharacterStates::AIR);
        //resetting values on the server side
		Server_LightToggle(ECharacterStates::ATK_AIR, false);
	}
}

/*This is the server side code for the replication of animations*/

void ACustomFighter::Server_LightToggle_Implementation(ECharacterStates atkType, bool bAtkIsActive)
{
	//This checks what attack is being thrown (air or ground)
	switch (atkType)
	{
	case ECharacterStates::ATK_GROUND:
        //sets the attack as active on the server
		bLightAtk = bAtkIsActive;
		if (bAtkIsActive == true)
		{
            //States must also be set on the server so it knows whats going on with attacking
			SM->SetCurrentState(ECharacterStates::ATK_GROUND);
		}
		else
		{
			SM->SetCurrentState(ECharacterStates::GROUND);
		}
		break;
	case ECharacterStates::ATK_AIR:
        //sets the attack as active on the server
		bLightAtkAir = bAtkIsActive;
		if (bAtkIsActive == true)
		{
			SM->SetCurrentState(ECharacterStates::ATK_AIR);
		}
		else
		{
			SM->SetCurrentState(ECharacterStates::AIR);
		}
		break;
	}
	
}