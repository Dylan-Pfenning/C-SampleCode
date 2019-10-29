/*
    @Copywrite: C Dylan Pfenning 2019
    @Author: Dylan Pfenning
    All of the following code was written for Unreal Engine 4.21 
    Header Files are available if you'd like to see them however they're pretty standard.
    Assume all #includes are present and correct.
*/

/* 
    This files is specific to combat code. IE; throwing out moves, adjusting hitboxes and registering hits.
    All code is up to date for the project as of September 1st 2019
	Link to the project as a whole can be found on my website @ dylanpfenning.com
*/

//All the following is done in it's own component titled "Hit Component." The component is then attached to the actor that can be hit. Following is the code that runs....
void UHitComponent::Hit_Implementation(UPrimitiveComponent *HittingComp, UPrimitiveComponent *HitComp, AActor *HitBy, EHitPower power, float DamageToDo, float KBTime, FVector_NetQuantize KBVec)
{
	//Make sure the character is valid
	if (IsValid(MyCharacter))
	{
		//make sure the character can be hit and whatever is hitting the character is a valid entity
		if (MyCharacter->CanBeHit())
			return;
		if (!IsValid(HitBy))
			return;

		//Change the state machine to hit in the air as grounded hitstun doesn't exist for this game.. This can be edited if grounded hitstun is desired
		MyCharacter->SM->SetCurrentState(ECharacterStates::HIT_AIR);

		//Checking to see what "getting hit" animation to play based off of where the character getting hit is facing in relation to the character doing the damage
		if (HitBy->GetActorForwardVector() == MyCharacter->GetActorForwardVector())
		{
			if (MyCharacter->GetHitAirFront())
			{
				MyCharacter->SetHitAirFront(false);
			}
			MyCharacter->SetHitAirBack(true);
		}
		else
		{
			if (MyCharacter->GetHitAirBack())
			{
				MyCharacter->SetHitAirBack(false);
			}
			MyCharacter->SetHitAirFront(true);
		}

		/*The launching of the characer*/

		//Each move has its own KB power based off what move was thrown out. This can be edited to "stale" out moves if need be in the case 1 ability is too strong. All values are editable in engine.
		LaunchVector = KBVec * HitBy->GetActorForwardVector();
		LaunchVector.Z = KBVec.Z;

		//Making sure the player can't be moving while being knocked back (hitstun)
		MyCharacter->bIsMovementEnabled = false;
		//Spawning the flash indicating a valid hit
		MyCharacter->SpawnHitSplat(HitComp, power);
		//Do some damage
		MyCharacter->SetHealth(MyCharacter->GetHealth() - DamageToDo);
		//Lastly we use the UE4 function of launch the character
		MyCharacter->LaunchCharacter(LaunchVector, true, true);

		UWorld *World = GetWorld();
		if (IsValid(World))
		{
			World->GetTimerManager().SetTimer(TimerHandle_KB, MyCharacter, &AFighterBase::Client_KBEnd, KBTime);
		}
		//Check to see if the health is 0 (dead)
		MyCharacter->Trigger_HealthZero(MyCharacter);
	}
}

//The following is all the calls for Light/Medium/Special attacks when the button is pressed

/*All of the code for activating hitboxes is called VIA animation timings in engine. The part that gets called by the animation notifiers will be at the end of this file*/

void ACustomFighter::LightAtk()
{
	//Make sure you're allowed the do the action
	if (bIsMovementEnabled == false)
		return;
	//Standard unreal validation checks
	UWorld *World = GetWorld();
	if (World != nullptr)
	{
		//Check if the fighter is on the grounf
		if (SM->GetCurrentState() == ECharacterStates::GROUND)
		{
			//If yes we execute the grounded routine for throwing out a light attack
			SM->SetCurrentState(ECharacterStates::ATK_GROUND);
			Client_StopMovement();
			World->GetTimerManager().SetTimer(TimerHandle_LightPunch, this, &ACustomFighter::Client_OnLightAttackEnd, LightAtkTime);
			bLightAtk = true;
			//Call the server toggle function that lets the server know the character is executing an attack.
			Server_LightToggle(SM->GetCurrentState(), true);
		}
		else if (SM->GetCurrentState() == ECharacterStates::AIR)
		{
			//If we're in the air we do an air attack
			SM->SetCurrentState(ECharacterStates::ATK_AIR);
			World->GetTimerManager().SetTimer(TimerHandle_LightAir, this, &ACustomFighter::Client_OnLightAttackEnd, LightAirTime);
			bLightAtkAir = true;
			Server_LightToggle(SM->GetCurrentState(), true);
		}
		//Let the client know that it is in attack mode.
		bIsHitting = true;
	}
}
//Medium attacks are a bit more complex as stick position matters with that move is thrown out (Think smash brothers)
void ACustomFighter::MedAtk()
{
	if (bIsMovementEnabled == false)
		return;
	UWorld *World = GetWorld();
	if (IsValid(World))
	{
		//Get the direction the control stick is facing
		float axisVal = InputComponent->GetAxisValue("DirectionInput");
		if (SM->GetCurrentState() == ECharacterStates::GROUND || SM->GetCurrentState() == ECharacterStates::ATK_GROUND)
		{
			//make sure the user doesnt want an anti air
			if (axisVal >= -.30f)
			{
				//Do netural medium attack

				//Toggle the med animation
				bMedAtk = true;
				bIsMovementEnabled = false;

				//Tell the server what we're trying to do
				Server_MedToggle(true, EMoveType::NEUTRAL);

				World->GetTimerManager().SetTimer(TimerHandle_MedGround, this, &ACustomFighter::Client_OnMediumAttackEnd, MedAtkTime);
				//This characters grounded medium attack pushes them forward 200 units so we launch the character the direction he is hitting
				FVector launchVec = FVector(0.0f, 1500.f, 0.0f) * this->GetActorForwardVector();
				this->LaunchCharacter(launchVec, true, true);
				Server_LaunchChar(launchVec);
				Client_StopMovement();
			}
			//the stick is pulled down so the player wants to anti-air
			else if (axisVal < -.30f)
			{
				//Do up medium anti air
				bMedLauncher = true;
				Server_MedToggle(true, EMoveType::ANTI_AIR);
				World->GetTimerManager().SetTimer(TimerHandle_MedLauncher, this, &ACustomFighter::Client_OnMediumAttackEnd, MedLauncherTime);
			}
		}
		//if we're in the air we have more to check for
		else if (SM->GetCurrentState() == ECharacterStates::AIR || SM->GetCurrentState() == ECharacterStates::ATK_AIR)
		{
			//In air neutral
			if (axisVal <= .30f && axisVal >= -.30f)
			{
				//DO Neutral med air
				bMedAtkAir = true;
				Server_MedToggle(true, EMoveType::AIR_NEUTRAL);
				World->GetTimerManager().SetTimer(TimerHandle_MedAirNeutral, this, &ACustomFighter::Client_OnMediumAttackEnd, MedAirNeutralTime);
			}

			//in air upwards attack
			else if (axisVal <= -.30f)
			{
				//Do med up atk
				bMedAirUp = true;
				Server_MedToggle(true, EMoveType::UP);
				World->GetTimerManager().SetTimer(TimerHandle_MedAirUp, this, &ACustomFighter::Client_OnMediumAttackEnd, MedAirUpTime);
			}

			//in air downwards attack
			else if (axisVal >= .30f)
			{
				//do med down atk
				bMedAirDown = true;
				Server_MedToggle(true, EMoveType::DOWN);
				World->GetTimerManager().SetTimer(TimerHandle_MedAirDown, this, &ACustomFighter::Client_OnMediumAttackEnd, MedAirDownTime);
			}
		}
		bIsHitting = true;
	}
}
//Lastly special attacks
void ACustomFighter::SpecAtk()
{
	//making sure we can move and we aren't currently in a state that doesnt allow for a special attack
	if (bIsMovementEnabled == false)
		return;
	if (SM->GetCurrentState() == ECharacterStates::AIR || SM->GetCurrentState() == ECharacterStates::ATK_AIR || SM->GetCurrentState() == ECharacterStates::HIT_AIR)
		return;

	//Making sure we have enough meter to use a special attack. (Meter is gained by successful attacks)
	if (Aggression < 100.f && BarsBuilt < 1)
		return;
	if (Aggression != 100)
	{
		Aggression += 100;
		BarsBuilt -= 1;
		Aggression -= 100;
	}

	UWorld *World = GetWorld();
	if (World != nullptr)
	{
		Client_StopMovement();
		bIsMovementEnabled = false;
		//set timers for spawning the shot as well as ending the animation
		World->GetTimerManager().SetTimer(TimerHandle_SpecialAttack, this, &ACustomFighter::Client_OnSpecialAttackEnd, SpecialTime);
		World->GetTimerManager().SetTimer(TimerHandle_ShootProj, this, &ACustomFighter::SpawnSpecActor, SpecialTime - .35f);
	}
	Server_SpecToggle(true);
}

void ACustomFighter::SpawnSpecActor_Implementation()
{
	ENetRole myRole = Role;
	//Spawning Projectile
	FVector loc = GetMesh()->GetSocketLocation(FName("RHand"));
	loc.Y += (75 * this->GetActorForwardVector().Y);
	UFighterMovementComponent *movement = Cast<UFighterMovementComponent>(this->GetMovementComponent());
	//Rotation of the projectile
	if (movement != nullptr)
	{
		if (movement->bIsFacingRight)
		{
			ADemark_Special *MySpec = this->GetWorld()->SpawnActor<ADemark_Special>(ProjectileRight, loc, this->GetActorRotation());
		}
		else
		{
			ADemark_Special *MySpec = this->GetWorld()->SpawnActor<ADemark_Special>(ProjectileLeft, loc, this->GetActorRotation());
		}
	}
	PlaySpSound();
}

/*Sample of the toggling on/off of hitboxes*/
void AFighterBase::Arms(bool isRightSide, bool isActive)
{
	if (isRightSide)
	{
		//Activates the custom hitboxes on the right side
		RightShoulder->SetCanDamage(isActive);
		RightArm->SetCanDamage(isActive);
		RightHand->SetCanDamage(isActive);
	}
	else
	{
		//Activates the custom hitboxes on the right side
		LeftShoulder->SetCanDamage(isActive);
		LeftArm->SetCanDamage(isActive);
		LeftHand->SetCanDamage(isActive);
	}
}

/*Execution code for when a hitbox connects with another character*/

void ACustomFighter::DoOverlap_Implementation(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &sweepResult)
{
	//making sure we aren't recalling a hit that already happened on a previous frame
	if (bHasHit == false)
	{
		UHitboxComponent *MyHittingComp = Cast<UHitboxComponent>(OverlappedComp);

		if (IsValid(MyHittingComp) && IsValid(OtherComp))
		{
			if (MyHittingComp->GetCanDamage())
			{

				AFighterBase *OtherFighter = Cast<AFighterBase>(OtherActor);
				if (!IsValid(OtherFighter))
					return;

				/*
					The below code does have cases for every type of attack as they all do give different damage values + different meter gain
					I saved the trouble of scrolling through it all as it looks very similar to the sample below
				*/
				if (bLightAtk) /*Light Ground Attack*/
				{
					//Play the hitting sound
					PlayLightHitSound();
					//Spawn hit splat
					SpawnHitSplat(OtherComp, EHitPower::LIGHT);
					//If you have over 500 aggression do more damage
					if (BarsBuilt >= 5)
					{
						//Hit the character (HitChar just calls the characters hit component to execute what is at the top of the file)
						OtherFighter->HitChar(MyHittingComp, OtherComp, this, EHitPower::LIGHT, Damage + .75, LightGroundKBTime, LightGroundKB);
						if (Aggression >= 7)
						{
							Aggression -= 7;
						}
						else
						{
							BarsBuilt -= 1;
							Aggression += 100;
							Aggression -= 7;
						}
					}
					else
					{
						//Hit the character (HitChar just calls the characters hit component to execute what is at the top of the file)

						OtherFighter->HitChar(MyHittingComp, OtherComp, this, EHitPower::LIGHT, Damage, LightGroundKBTime, LightGroundKB);
					}
					//add aggression
					Aggression += 10;
				}
			}
		}
	}
}