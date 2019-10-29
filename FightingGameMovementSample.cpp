/*
    @Copywrite: C Dylan Pfenning 2019
    @Author: Dylan Pfenning
    All of the following code was written for Unreal Engine 4.21 
    Header Files are available if you'd like to see them however they're pretty standard.
    Assume all #includes are present and correct.
*/

/*
    The code below is all related to character movement in unreal engine. Samples will be taken from a few different files
*/

void AFighterBase::DoMoveCharacter_Implementation(float val)
{
	//Moving the character using the custom movement component that was created the combat the issue that we ran into with regards to rotation on the network.
	//Move Right
	if (val > 0.0f)
	{
        //check direction
		if (bIsFacingRight == false)
		{
			bIsFacingRight = true;
            //Spawn the dash particle
			if (DashLeftParticleComponent->bIsActive)
			{
				DashLeftParticleComponent->Deactivate();
			}

			UFighterMovementComponent* FighterCharacterMovement = Cast<UFighterMovementComponent>(GetCharacterMovement());
            //Set the direction in the movement comp
			if (FighterCharacterMovement != nullptr)
			{
				FighterCharacterMovement->bIsFacingRight = true;
			}
		}
	}
	//Move Left (same as above but to to the left)
	else if (val < 0.0f)
	{
		if (bIsFacingRight == true)
		{
			if (DashRightParticleComponent->bIsActive)
			{
				DashRightParticleComponent->Deactivate();
			}
			bIsFacingRight = false;
			UFighterMovementComponent* FighterCharacterMovement = Cast<UFighterMovementComponent>(GetCharacterMovement());

			if (FighterCharacterMovement != nullptr)
			{
				FighterCharacterMovement->bIsFacingRight = false;
			}
		}
	}
	else
	{
        //Deactivate all the particles if movement has stopped
		if (DashRightParticleComponent->bIsActive)
		{
			DashRightParticleComponent->Deactivate();
		}
		if (DashLeftParticleComponent->bIsActive)
		{
			DashLeftParticleComponent->Deactivate();
		}
	}
    //Check if the character is in the air or not
	if (bInAir == false)
	{
		if (val > 0.0f && DashRightParticleComponent->IsActive() == false)
		{
			if (GetVelocity().Size() != 0)
			{
				DashRightParticleComponent->Activate();
			}
		}
		else if (val < 0.0f && DashLeftParticleComponent->IsActive() == false)
		{
			if (GetVelocity().Size() != 0)
			{
				DashLeftParticleComponent->Activate();
			}
		}
	}
    //lastly move the character based on the forward direction and the absolute values of the sticks left/right positioning
	AddMovementInput(GetActorForwardVector(), FMath::Abs(val), false);
}


//Below is a section of the movement component that had to be re written due to characters bouncing off of eachothers heads
void UFighterMovementComponent::JumpOff(AActor* MovementBaseActor)
{
	if (!bPerformingJumpOff)
	{
		bPerformingJumpOff = true;
		if (CharacterOwner)
		{
            //Just shifting the position left/right  allowing the character to "slide" off of the head rather than bounce on it.
			FVector MyPos = CharacterOwner->GetActorLocation();
			FVector OtherPos = MovementBaseActor->GetActorLocation();
			if (MyPos.Y >= OtherPos.Y)
			{
				Velocity.Y = 500.f;
			}
			else if (MyPos.Y < OtherPos.Y)
			{
				Velocity.Y = -500.f;
			}
			SetMovementMode(MOVE_Falling);
		}
		bPerformingJumpOff = false;
	}
}