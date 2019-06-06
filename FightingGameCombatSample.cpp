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



//In unreal you must validate before implementation. The validate method for TakeHit returned true by default.

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