/********************************************************************************************************************
Author: Chad Cromwell
Date: February 26th, 2017
Assignment: 4
Program: SpriteGuy.cpp
Description: A game made with Allegro, and C++ with added AI, threading, and a .dat file
Copyright info:
Graphics: Chad Cromwell
Sounds: bg.wav is "Chibi Ninja" by Eric Skiff retrieved from http://ericskiff.com/music/ Creative Commons Attribution License: http://ericskiff.com/music/
Fonts: titleFont is Clementine Sketch by teagan_white retrieved from http://www.1001fonts.com/clementine-sketch-font.html under Donationware(Freeware) license: http://www.1001fonts.com/clementine-sketch-font.html#license
bodyFont is Arial font under Windows EULA
No TTfonts are distributed with this game, only .pcx glyphs
********************************************************************************************************************/

#define HAVE_STRUCT_TIMESPEC

#include <pthread.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include "allegro.h"

#define BLACK makecol(0,0,0)
#define WHITE makecol(255,255,255)

DATAFILE *datFile = NULL;
//**Timing***
volatile long counter = 0;
void Increment() {
	counter++;
}
END_OF_FUNCTION(Increment)

//Timing variables
int ticks; //Keep track of frames
int seconds; //Keep track of seconds
int mins; //Keep track of minutes
		  //************

		  //***Variables***
		  //Booleans
bool goLeft = false; //Determine if Sprite Guy is going left
bool goRight = false; //Determine if Sprite Guy is going right
bool canJump = true; //Determine if Sprite Guy is allowed to jump
bool jumping = false; //Determine if Sprite Guy is jumping
bool facingRight = false; //Determine if Sprite Guy is facing right or not
bool facingLeft = false; //Determine if Sprite Guy is facing left or not
bool facingFront = true; //Determine if Sprite Guy is facing front (screen) or not
bool rightRelease = false; //Determine if right key has been released
bool leftRelease = false; //Determine if left key has been released
bool collision = false; //Determine if collision has happened
bool loseHP = false; //Determine if HP is lost
bool turnRight = false;  //Determine if Sprite Guy is turning right
bool turnLeft = false;  //Determine if Sprite Guy is turning left
bool set = false;  //Determine if Sprite Guy needs to be set up for his new body form
bool dead = false;  //Determine if player dies
bool menu = false;  //Determine if in menu
bool game = true;  //Determine if in game
bool gameOver = false; //Determine if game loop should exit
bool mute = false;  //Determine if music is muted or not
bool help = false; //Determine if help is toggled or not
bool lockedOn = false; //Determine if the smart egg needs to find a new x position for AI

					   //Integers
int randomX = rand() % 1024; //Initialize randomX
int randomY = -(rand() % 300); //Initialize randomY
int groundHeight = 0; //Initialize, to hold the height of the ground, taken from sand.bmp->h
int spriteGuyGroundHeight = 0; //Initialize, to hold the y height needed for Sprite Guy to be on the ground: SCREEN_H - sand.bmp->h - Sprite Guy's height
int hp = 3; //Initialize hit points
int lastHP = 0; //Initialize lastHP to keep track of hp will invincible
int timer = 0; //Initialize timer to keep track of time passed
int transferFrame = 0; //Initialize transferFrame, used to hold the animation frame between transformations
int lastState = 0;  //Initialize lastSTate, used to hold what the previous transformation state was
int splatSpotX = 0; //Initialize splatSpotX, used to hold where splat BITMAP should be drawn
int splatSpotY = 0; //Initialize splatSpotY, used to hold where splat BITMAP should be drawn
int state = 0; //0 normal, 1 shrink, 2 large, 3 invincible
int volume; //Volume
int homingX; //To hold the live x position for the homing egg AI
int homingSpeed = 1; //The speed at which the homing egg can traverse X towards the player. 1 is a good speed to keep it at.

BITMAP *buffer; //For screen buffer
BITMAP *splatBuffer; //Buffer for egg splats
BITMAP *background; //For the background
BITMAP *hpHeart; //For HP hearts
BITMAP *splat; //For splat animation
BITMAP *sgCollisionMask; //Masked bitmap, for making collision area smaller
BITMAP *tempBuf; //Buffer to hold the threads

				 //Physics variables
double gravity = 1; //Amount of gravity, good setting is 1
double jumpVelocity = -15; //Initial velocity of jump, good setting is -15
double sgAcceleration = .7; //Acceleration of Sprite Guy while running, increase this and he accelerates faster. If set below 0 he will move backwards. Keep above 0.
double sgDecceleration = .5; //Decceleration of Sprite Guy when stopping running, increase this and he slows down faster. Keep above 0.
double maxHVelocity = 18; //Maximum velocity that Sprite Guy can run, the higher the faster, the lower the slower. Keep above 0.
double drag = .5; //Air resistance, how fast Sprite Guy slows down when running, best set to 50% of sgAcceleration
double maxDropVelocity = 3; //Egg terminal velocity
double eggSpeed = 0.1; //Egg's general speed increments
double potionSpeed = 0.025; //Potions drop speed
double maxPotionDropVelocity = 3; //Potions terminal velocity
double heartSpeed = 0.025; //Heart general speed increments
double maxHeartDropVelocity = 3; //Heart terminal velocty
								 //Some here for options, not all used

								 //SAMPLES/Sound variables
SAMPLE *bg;
SAMPLE *hurt;
SAMPLE *healed;
SAMPLE *grow;
SAMPLE *shrink;
SAMPLE *invincible;

//FONTS
FONT *bodyFont;
FONT *titleFont;

//Thread MUTEX to protect variables in use
pthread_mutex_t threadsafe = PTHREAD_MUTEX_INITIALIZER;

//******************************************************************************************************************************

//Sprite structure
typedef struct SPRITE
{
	double x, y, xVelocity, yVelocity; //Hold xy position and velocities
	int width, height; //Hold width and height sprite
	int xdelay, ydelay; //Hold x and y delays, slows down movement in these directions if needed
	int xcount, ycount; //Count the current x and y, used to compare against delay, if delay is reached, then movement will take place
	int curframe, maxframe, animdir; //Holds current frame, max frame of animation, and the direction of animation
	int framecount, framedelay; //Holds which frame to display, delay is used like x and y delay, slows animation frames down if desired

}SPRITE;

//***Sprites***
//Sprite Guy
COMPILED_SPRITE *spriteGuyImg[120]; //Array to hold compiled sprite frames for animation
SPRITE spriteGuy1; //Sprite
SPRITE *spriteGuy = &spriteGuy1; //Pointer

								 //Egg
COMPILED_SPRITE *eggImg[1];
SPRITE egg1;
SPRITE *egg = &egg1;

//Ground
COMPILED_SPRITE *groundImg[1];
SPRITE ground1;
SPRITE *ground = &ground1;

//Other eggs
COMPILED_SPRITE *egg2Img[1];
SPRITE egg21;
SPRITE *egg2 = &egg21;

COMPILED_SPRITE *egg3Img[1];
SPRITE egg31;
SPRITE *egg3 = &egg31;

COMPILED_SPRITE *egg4Img[1];
SPRITE egg41;
SPRITE *egg4 = &egg41;

COMPILED_SPRITE *egg5Img[1];
SPRITE egg51;
SPRITE *egg5 = &egg51;

//Potion
COMPILED_SPRITE *potionImg[1];
SPRITE potion1;
SPRITE *potion = &potion1;

//Hearts
COMPILED_SPRITE *heartImg[1];
SPRITE heart1;
SPRITE *heart = &heart1;

COMPILED_SPRITE *heartImg2[1];
SPRITE heart21;
SPRITE *heart2 = &heart21;

COMPILED_SPRITE *heartImg3[1];
SPRITE heart31;
SPRITE *heart3 = &heart31;

COMPILED_SPRITE *heartImg4[1];
SPRITE heart41;
SPRITE *heart4 = &heart41;
//*************

//collisionTest(SPRITE *spr1, SPRITE *spr2) - Test if two sprites collide with one another
bool collisionTest(SPRITE *spr1, SPRITE *spr2) {

	if (spr1->x + spr1->width < spr2->x || spr1->x > spr2->x + spr2->width || spr1->y > spr2->y + spr2->height || spr1->y + spr1->height < spr2->y) { //If their edges do not collide or overlap
		return false; //No collision
	}
	else {
		return true; //Collision
	}
}

//maskSGCollisionTest(SPRITE *spr1) - Test to see if objects collide with Sprite Guy with mask, more closely bounds to his body instead of BITMAP box
bool maskSGCollisionTest(SPRITE *spr1) {
	if (spr1->x + spr1->width < spriteGuy->x + 50 || spr1->x > spriteGuy->x + spriteGuy->width - 50 || spr1->y > spriteGuy->y + spriteGuy->height || spr1->y + spr1->height < spriteGuy->y) {

		return false;
	}
	else {
		return true;
	}
}

//drawLevel(BITMAP *dest) - *dest = destination Bitmap. Draws background and ground
void drawLevel(BITMAP *dest) {
	blit(background, buffer, 0, 0, 0, 0, 1024, 768);
}


//physics(SPRITE *spr) - *spr = sprite. Accepts SPRITE and applies physics to it
void physics(SPRITE *spr) {

	//If in NORMAL state or INVINCIBLE state
	if (state == 0 || state == 3) {
		//Update his size
		spriteGuy->height = 128;
		spriteGuy->width = 128;

		//If he needs to be set up
		if (set == true) {

			//If he's coming from being SHRUNKEN
			if (lastState == 1) {
				spr->curframe = transferFrame - 40; //Change the curframe so it now draws normal sized Sprite Guy
				if (collisionTest(spr, ground) == true) { //If he's on the ground
					spr->y = SCREEN_H - ground->height - spriteGuy->height; //Set his height so he's not below the ground
				}
			}

			//If he's coming from being GROWN
			if (lastState == 2) {
				spr->curframe = transferFrame - 80; //Change the curframe so it now draws normal sized Sprite Guy
				if (collisionTest(spr, ground) == true) { //If he's on the ground
					spr->y = SCREEN_H - ground->height - spriteGuy->height; //Set his height so he's not below the ground
					spr->yVelocity = 0; //Set his velocity to 0 so he doesn't fall through the ground due to his large size and growing will send him under the ground
				}
			}

			//Apply gravit to Sprite Guy to send him towards the ground after he transforms back
			if (++spr->ycount > spr->ydelay) {
				spr->ycount;
				spr->yVelocity = spr->yVelocity + gravity;
				spr->y += spr->yVelocity;
			}

			//If he collides with the ground
			if (collisionTest(spr, ground) == true) {
				spr->yVelocity = 0; //Set his velocity to 0
				set = false; //He no longer needs to be set
			}

		}

		//update y position
		if (++spr->ycount > spr->ydelay) {
			spr->ycount = 0;

			//If something has upward velocity, apply gravity to it
			if (jumping == true) {
				spr->yVelocity = spr->yVelocity + gravity;
				spr->y += spr->yVelocity;
			}

		}

	}

	//If he's SHRUNKEN - Same logic as above
	if (state == 1) {

		spriteGuy->height = 64;
		spriteGuy->width = 64;

		if (set == true) {

			if (++spr->ycount > spr->ydelay) {
				spr->ycount = 0;
				spr->yVelocity = spr->yVelocity + gravity;
				spr->y += spr->yVelocity;
			}

			if (collisionTest(spr, ground) == true) {
				spr->yVelocity = 0;
				set = false;
			}

		}

		//update y position
		if (++spr->ycount > spr->ydelay) {
			spr->ycount = 0;

			//If something has upward velocity, apply gravity to it
			if (jumping == true) {
				spr->yVelocity = spr->yVelocity + gravity;
				spr->y += spr->yVelocity;
			}

		}

	}

	//If he's GROWN - Same logic as above
	if (state == 2) {

		spriteGuy->height = 192;
		spriteGuy->width = 192;

		if (set == true) {
			spr->curframe = transferFrame + 80;

			if (collisionTest(spr, ground) == true) {
				spr->y = SCREEN_H - ground->height - spriteGuy->height;
			}

			if (++spr->ycount > spr->ydelay) {
				spr->ycount = 0;
				spr->yVelocity = spr->yVelocity + gravity;
				spr->y += spr->yVelocity;
			}

			if (collisionTest(spr, ground) == true) {
				spr->yVelocity = 0;
				set = false;
			}

		}

		//update y position
		if (++spr->ycount > spr->ydelay) {
			spr->ycount = 0;

			//If something has upward velocity, apply gravity to it
			if (jumping == true) {
				spr->yVelocity = spr->yVelocity + gravity;
				spr->y += spr->yVelocity;
			}

		}

	}

	//Update x position
	if (++spr->xcount > spr->xdelay) { //Increment the sprite's xcount and if it is bigger than xdelay

		spr->xcount = 0; //Set xcount to 0

						 //If xVelocity is at maxVelocity
		if (spr->xVelocity > maxHVelocity) {
			spr->xVelocity = maxHVelocity; //Set velocity as max, stops him from going faster
		}

		//Same as above but for moving left
		if (spr->xVelocity < -maxHVelocity) {
			spr->xVelocity = -maxHVelocity;
		}

		//If he's moving right
		if (spr->xVelocity > 0) {
			spr->xVelocity = spr->xVelocity - drag; //Apply drag
			spr->x += spr->xVelocity; //Update velocity
			if (spr->xVelocity <= 0) { //When he stops moving due to drag
				spr->xVelocity = 0; //Stop his velocity
			}
		}

		//If he's moving left, same logic as above
		if (spr->xVelocity < 0) {
			spr->xVelocity = spr->xVelocity + drag;
			spr->x += spr->xVelocity;
			if (spr->xVelocity >= 0) {
				spr->xVelocity = 0;
			}
		}
	}

}

//eggPhysics() - Handles physics for the eggs - Same logic as above
void eggPhysics() {

	//update y frame
	if (++egg->ycount > egg->ydelay) {
		egg->ycount = 0;

		egg->yVelocity = egg->yVelocity + eggSpeed; //Apply gravity
		if (collisionTest(egg, ground) == false) { //If it is above the ground
			egg->y += egg->yVelocity; //Increase velocity linearly
		}
		if (collisionTest(egg, ground) == true) { //If it is on the ground
			egg->y = ground->y - egg->height; //Set it's y height so it is on the ground
		}
		if (egg->yVelocity >= maxDropVelocity) { //If egg reaches terminal velocity
			egg->yVelocity = maxDropVelocity; //Limit velocity
		}
	}

	//Apply physics to the rest of the eggs - Individualized if you'd like to adjust their individual speeds
	if (++egg2->ycount > egg2->ydelay) {
		egg2->ycount = 0;

		egg2->yVelocity = egg2->yVelocity + eggSpeed; //Apply gravity
		if (collisionTest(egg2, ground) == false) { //If it is above the ground
			egg2->y += egg2->yVelocity; //Increase velocity linearly
		}
		if (collisionTest(egg2, ground) == true) { //If it is on the ground
			egg2->y = ground->y - egg2->height; //Set it's y height so it is on the ground
		}
		if (egg2->yVelocity >= maxDropVelocity) { //If egg reaches terminal velocity
			egg2->yVelocity = maxDropVelocity; //Limit velocity
		}
	}

	if (++egg3->ycount > egg3->ydelay) {
		egg3->ycount = 0;

		egg3->yVelocity = egg3->yVelocity + eggSpeed; //Apply gravity
		if (collisionTest(egg3, ground) == false) { //If it is above the ground
			egg3->y += egg3->yVelocity; //Increase velocity linearly
		}
		if (collisionTest(egg3, ground) == true) { //If it is on the ground
			egg3->y = ground->y - egg3->height; //Set it's y height so it is on the ground
		}
		if (egg3->yVelocity >= maxDropVelocity) { //If egg reaches terminal velocity
			egg3->yVelocity = maxDropVelocity; //Limit velocity
		}
	}

	if (++egg4->ycount > egg4->ydelay) {
		egg4->ycount = 0;

		egg4->yVelocity = egg4->yVelocity + eggSpeed; //Apply gravity
		if (collisionTest(egg4, ground) == false) { //If it is above the ground
			egg4->y += egg4->yVelocity; //Increase velocity linearly
		}
		if (collisionTest(egg4, ground) == true) { //If it is on the ground
			egg4->y = ground->y - egg4->height; //Set it's y height so it is on the ground
		}
		if (egg4->yVelocity >= maxDropVelocity) {  //If egg reaches terminal velocity
			egg4->yVelocity = maxDropVelocity; //Limit velocity
		}
	}

	if (++egg5->ycount > egg5->ydelay) {
		egg5->ycount = 0;

		egg5->yVelocity = egg5->yVelocity + eggSpeed; //Apply gravity
		if (collisionTest(egg5, ground) == false) { //If it is above the ground
			egg5->y += egg5->yVelocity; //Increase velocity linearly
		}
		if (collisionTest(egg5, ground) == true) { //If it is on the ground
			egg5->y = ground->y - egg5->height; //Set it's y height so it is on the ground
		}
		if (egg5->yVelocity >= maxDropVelocity) {  //If egg reaches terminal velocity
			egg5->yVelocity = maxDropVelocity; //Limit velocity
		}
	}
}

//potionPhysics()  - Apply physics to the potions -Saame logic as above
void potionPhysics() {

	//update y frame
	if (++potion->ycount > potion->ydelay) {
		potion->ycount = 0;

		potion->yVelocity = potion->yVelocity + potionSpeed; //Apply gravity
		if (collisionTest(potion, ground) == false) { //If it is above the ground
			potion->y += potion->yVelocity; //Increase velocity linearly
		}
		if (collisionTest(potion, ground) == true) { //If it is on the ground
			potion->y = ground->y - potion->height; //Set it's y height so it is on the ground
		}
		if (potion->yVelocity >= maxPotionDropVelocity) { //If potion reaches terminal velocity
			potion->yVelocity = maxPotionDropVelocity; //Limit velocity
		}
	}
}

//heartPhysics() - Apply physics to the hearts - Same logic as above
void heartPhysics() {

	//update y frame
	if (++heart->ycount > heart->ydelay) {
		heart->ycount = 0;

		heart->yVelocity = heart->yVelocity + heartSpeed; //Apply gravity
		if (collisionTest(heart, ground) == false) { //If it is above the ground
			heart->y += heart->yVelocity; //Increase velocity linearly
		}
		if (collisionTest(heart, ground) == true) { //If it is on the ground
			heart->y = ground->y - heart->height; //Set it's y height so it is on the ground
		}
		if (heart->yVelocity >= maxHeartDropVelocity) { //If heart reaches terminal velocity
			heart->yVelocity = maxHeartDropVelocity; //Limit velocity
		}
	}
}


//warpSprite(SPRITE *spr) - *spr = sprite. Accepts a sprite and determines if sprite goes off screen and warps it to come in from other side of the screen
void warpsprite(SPRITE *spr)
{
	//If he goes half off, wrap the other half to opposite side - For each side
	if (spr->x > SCREEN_W - (spr->width / 2))
	{
		spr->x = 0 - (spr->width / 2);
	}

	else if (spr->x < 0 - (spr->width / 2))
	{
		spr->x = SCREEN_W - (spr->width / 2);
	}

}

//compiled_grabframe(BITMAP *source, int width, int height, int startx, int starty, int colums, int frame) - Source bitmap, it's width, height, where it starts x, and y, how many colums, and current frame
//Accepts a bitmap and parses it to divide it into individual frames for animation
COMPILED_SPRITE *compiled_grabframe(BITMAP *source,
	int width, int height,
	int startx, int starty,
	int columns, int frame)
{
	COMPILED_SPRITE *sprite; //Hold compiled sprite
	BITMAP *temp = create_bitmap(width, height); //Temporary Bitmap to copy to

	int x = startx + (frame % columns) * width; //End x
	int y = starty + (frame / columns) * height; //End y of where to cut to

	blit(source, temp, x, y, 0, 0, width, height); //Blit the cropped frame to temp bitmap

												   //Set compiled sprite from temp to sprite
	sprite = get_compiled_sprite(temp, FALSE);

	//Destroy temp bitmap to be reused again
	destroy_bitmap(temp);

	//Return the cropped frame as sprite
	return sprite;
}


//draw(BITMAP *s, COMPILED_SPRITE *cs[], SPRITE *c) - *s destination bitmap, source compiled sprite frame, sprite for x and y location - Accepts these parameters and draws it to the screen
void draw(BITMAP *s, COMPILED_SPRITE *cs[], SPRITE *c) {
	draw_compiled_sprite(s, cs[c->curframe], c->x, c->y);
}

//animate(BITMAP *dest, SPRITE *spr) - Animates sprite to destination bitmap
void animate(BITMAP *dest, SPRITE *spr) {

	//************************************************************************************************************************
	//If in NORMAL or INVINCIBLE form
	if (state == 0 || state == 3) {

		//If going right
		if (goRight == true) {
			//If the frame hits 20, go back to 0 animation frame (0-20 is right walking animation)
			if (spr->curframe > 20) {
				spr->curframe = 0;
			}
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (++spr->curframe > 19) { //Increment the sprite's curframe, if it is greater than 19
					spr->curframe = 10; //Set sprite's curframe to 10, to keep animating through his running right animation
				}
			}
			draw(buffer, spriteGuyImg, spr); //Draw him to the buffer

		}

		//If going left
		if (goLeft == true) {

			//If the frame is less than 20, set to 20 (20-40 is left walking animation)
			if (spr->curframe < 20) {
				spr->curframe = 20;
			}
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (++spr->curframe > 39) { //Increment the sprite's curframe, if it is greater than 39
					spr->curframe = 30; //Set sprite's curframe to 30, to keep animating him through his running left animation
				}
			}
			draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
		}

		//If stopping going right, animate him back to facing the screen
		if (goRight == false && goLeft == false && facingRight == true) { //If not moving and facing right
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (--spr->curframe < 0) { //Decrement curframe until it reaches 0, which is him facing the screen
					spr->curframe = 0; //Set sprite's curframe to 0
					facingFront = true; //Now facing front
					facingRight = false; //No longer facing right
				}
			}
			draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
		}

		//If stopping going left, animate him back to facing the screen
		if (goRight == false && goLeft == false && facingLeft == true) { //If not moving and facing left
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (--spr->curframe < 20) { //Decrement curframe until frame reaches 20, which is him facing the screen
					spr->curframe = 20; //Set sprite's curframe to 20
					facingFront = true; //Now facing front
					facingLeft = false; //No longer facing right
				}
			}
			draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
		}

		//If facing the front of the screen
		if (goRight == false && goLeft == false && facingFront == true) { //If not moving and facing front
			if (spr->curframe == 19) { //If the curframe is 19
				if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
					spr->framecount = 0; //Set framecount to 0
					if (--spr->curframe < 0) { //Decrement curframe until he's facing the screen
						spr->curframe = 0; //Set sprite's curframe to 0
					}
				}
			}
			if (spr->curframe == 39) { //If the curframe is 39
				if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
					spr->framecount = 0; //Set framecount to 0
					if (--spr->curframe < 20) { //Decrement curframe until he's facing the screen
						spr->curframe = 20; //Set sprite's curframe to 0
					}
				}
			}
			draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
		}
	}

	//************************************************************************************************************************

	//************************************************************************************************************************

	//If SHRUNKEN - Same logic as above, escept curframes get shifted up by 40 so they are at the proper animation frame for small Sprite Guy
	if (state == 1) {
		if (set == true) {
			spr->curframe = transferFrame + 40;
		}
		if (goRight == true) {
			if (spr->curframe > 60 || spr->curframe < 40) {
				spr->curframe = 40;
			}
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (++spr->curframe > 59) { //Increment the sprite's curframe, if it is greater than maxframe
					spr->curframe = 50; //Set sprite's curframe to 0
				}
			}
			draw(buffer, spriteGuyImg, spr);

		}

		if (goLeft == true) {
			if (spr->curframe < 60) {
				spr->curframe = 60;
			}
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (++spr->curframe > 79) { //Increment the sprite's curframe, if it is greater than maxframe
					spr->curframe = 70; //Set sprite's curframe to 0
				}
			}
			draw(buffer, spriteGuyImg, spr);
		}

		if (goRight == false && goLeft == false && facingRight == true) { //If not moving and facing right
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (--spr->curframe < 40) { //Increment the sprite's curframe, if it is greater than maxframe
					spr->curframe = 40; //Set sprite's curframe to 0
					facingFront = true;
					facingRight = false;
				}
			}
			draw(buffer, spriteGuyImg, spr);
		}

		if (goRight == false && goLeft == false && facingLeft == true) { //If not moving and facing left
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (--spr->curframe < 60) { //Increment the sprite's curframe, if it is greater than maxframe
					spr->curframe = 60; //Set sprite's curframe to 0
					facingFront = true;
					facingLeft = false;
				}
			}
			draw(buffer, spriteGuyImg, spr);
		}
		if (goRight == false && goLeft == false && facingFront == true) { //If not moving and facing front
			if (spr->curframe == 59) {
				if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
					spr->framecount = 0; //Set framecount to 0
					if (--spr->curframe < 40) {
						spr->curframe = 40; //Set sprite's curframe to 0
					}
				}
			}
			if (spr->curframe == 79) {
				if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
					spr->framecount = 0; //Set framecount to 0
					if (--spr->curframe < 60) {
						spr->curframe = 60; //Set sprite's curframe to 0
					}
				}
			}
			draw(buffer, spriteGuyImg, spr);
		}
	}
	//************************************************************************************************************************

	//************************************************************************************************************************
	//If GROWN - Same logic as above but curframe increment of 80
	if (state == 2) {
		if (set == true) {
			spr->curframe = transferFrame + 80;
		}
		if (goRight == true) {
			if (spr->curframe > 100 || spr->curframe < 80) {
				spr->curframe = 80;
			}
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (++spr->curframe > 99) { //Increment the sprite's curframe, if it is greater than maxframe
					spr->curframe = 90; //Set sprite's curframe to 0
				}
			}
			draw(buffer, spriteGuyImg, spr);

		}

		if (goLeft == true) {
			if (spr->curframe < 100) {
				spr->curframe = 100;
			}
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (++spr->curframe > 119) { //Increment the sprite's curframe, if it is greater than maxframe
					spr->curframe = 110; //Set sprite's curframe to 0
				}
			}
			draw(buffer, spriteGuyImg, spr);
		}

		if (goRight == false && goLeft == false && facingRight == true) { //If not moving and facing right
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (--spr->curframe < 80) { //Increment the sprite's curframe, if it is greater than maxframe
					spr->curframe = 80; //Set sprite's curframe to 0
					facingFront = true;
					facingRight = false;
				}
			}
			draw(buffer, spriteGuyImg, spr);
		}

		if (goRight == false && goLeft == false && facingLeft == true) { //If not moving and facing left
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (--spr->curframe < 100) { //Increment the sprite's curframe, if it is greater than maxframe
					spr->curframe = 100; //Set sprite's curframe to 0
					facingFront = true;
					facingLeft = false;
				}
			}
			draw(buffer, spriteGuyImg, spr);
		}
		if (goRight == false && goLeft == false && facingFront == true) { //If not moving and facing front
			if (spr->curframe == 99) {
				if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
					spr->framecount = 0; //Set framecount to 0
					if (--spr->curframe < 80) {
						spr->curframe = 80; //Set sprite's curframe to 0
					}
				}
			}
			if (spr->curframe == 119) {
				if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
					spr->framecount = 0; //Set framecount to 0
					if (--spr->curframe < 100) {
						spr->curframe = 100; //Set sprite's curframe to 0
					}
				}
			}
			draw(buffer, spriteGuyImg, spr);
		}
	}
	//************************************************************************************************************************
}

//drawSplat(int x, int y) - Draws masked blit of the egg splat to the given x y coords, to be used in dropEggs()
void drawSplat(int x, int y) {
	masked_blit(splat, splatBuffer, 0, 0, x, y, 32, 32);
}

//dropEggs(BITMAP *dest, SPRITE *spr) - Drops eggs on the destination bitmap
void dropEggs(BITMAP *dest, SPRITE *spr) {
	if (++spr->framecount > spr->framedelay) { //Increment framecount and if it is greater than delay
		spr->framecount = 0;  //Set framecount to 0

							  //Animate
		if (++spr->curframe > spr->maxframe) { //Increment current frame and if it is greater than maxframe
			spr->curframe = 0; //Set current frame to 0
		}

	}

	draw_compiled_sprite(dest, eggImg[0], spr->x, spr->y); //Draw the compiled sprite of the egg

	if (maskSGCollisionTest(spr) == true) { //If egg collides with Sprite Guy
		play_sample(hurt, 255, 127, 1000, FALSE); //Play hurt sound effect
		if (hp > 0) { //If health is over 0
			hp--; //Lower the players health by 1
		}
	}
	if (collisionTest(spr, ground) == true || maskSGCollisionTest(spr)) { //If egg hits the ground or Sprite Guy
		drawSplat(spr->x, spr->y); //Draw splat bitmap in it's place
		spr->x = rand() % 993; //Assign randomly generated x coord
		spr->y = -(rand() % 300); //Assign randomly generated y coord, the bigger the number, the longer it takes to enter screen
	}

	if (state == 1) {
		if (collisionTest(spr, spriteGuy) == true) { //If egg collides with Sprite Guy
			play_sample(hurt, 255, 127, 1000, FALSE); //Play hurt sound effect
			if (hp > 0) { //If health is over 0
				hp--; //Lower the players health by 1
			}
		}
		if (collisionTest(spr, ground) == true || collisionTest(spr, spriteGuy)) { //If egg hits the ground or Sprite Guy
			drawSplat(spr->x, spr->y); //Draw splat bitmap in it's place
			spr->x = rand() % 993; //Assign randomly generated x coord
			spr->y = -(rand() % 300); //Assign randomly generated y coord, the bigger the number, the longer it takes to enter screen
		}
	}
}

//***AI IMPLEMENTATION***
//dropSmartEgg(BITMAP *dest, SPRITE *spr) - Drops an egg with some AI that makes it drop in the same x position as the user
void dropSmartEgg(BITMAP *dest, SPRITE *spr) {
	if (++spr->framecount > spr->framedelay) { //Increment framecount and if it is greater than delay
		spr->framecount = 0;  //Set framecount to 0

							  //Animate
		if (++spr->curframe > spr->maxframe) { //Increment current frame and if it is greater than maxframe
			spr->curframe = 0; //Set current frame to 0
		}

	}

	if (lockedOn == false) {
		spr->x = (spriteGuy->x + (spriteGuy->width / 2)); //Assign egg's x position to be in line with spriteGuy's x position
		spr->x = spr->x + (rand() % 21 + (-10)); //Vary this by 10 pixels so it has some variation
		lockedOn = true;
	}

	draw_compiled_sprite(dest, eggImg[0], spr->x, spr->y); //Draw the compiled sprite of the egg

	if (maskSGCollisionTest(spr) == true) { //If egg collides with Sprite Guy
		play_sample(hurt, 255, 127, 1000, FALSE); //Play hurt sound effect
		if (hp > 0) { //If health is over 0
			hp--; //Lower the players health by 1
		}
		lockedOn = false;
	}
	if (collisionTest(spr, ground) == true || maskSGCollisionTest(spr)) { //If egg hits the ground or Sprite Guy
		drawSplat(spr->x, spr->y); //Draw splat bitmap in it's place
		spr->x = rand() % 993; //Assign randomly generated x coord
		spr->y = -(rand() % 300); //Assign randomly generated y coord, the bigger the number, the longer it takes to enter screen
		lockedOn = false;
	}

	if (state == 1) {
		if (collisionTest(spr, spriteGuy) == true) { //If egg collides with Sprite Guy
			play_sample(hurt, 255, 127, 1000, FALSE); //Play hurt sound effect
			if (hp > 0) { //If health is over 0
				hp--; //Lower the players health by 1
			}
			lockedOn = false;
		}
		if (collisionTest(spr, ground) == true || collisionTest(spr, spriteGuy)) { //If egg hits the ground or Sprite Guy
			drawSplat(spr->x, spr->y); //Draw splat bitmap in it's place
			spr->x = rand() % 993; //Assign randomly generated x coord
			spr->y = -(rand() % 300); //Assign randomly generated y coord, the bigger the number, the longer it takes to enter screen
			lockedOn = false;
		}
	}
}

//***AI IMPLEMENTATION***
//dropHomingEgg(BITMAP *dest, SPRITE *spr) - Drops an egg with that homes in on the user
void dropHomingEgg(BITMAP *dest, SPRITE *spr) {
	if (++spr->framecount > spr->framedelay) { //Increment framecount and if it is greater than delay
		spr->framecount = 0;  //Set framecount to 0

							  //Animate
		if (++spr->curframe > spr->maxframe) { //Increment current frame and if it is greater than maxframe
			spr->curframe = 0; //Set current frame to 0
		}

	}

	//If the egg is to the left of the user, move it towards the right at homingSpeed
	if (spr->x < (spriteGuy->x + spriteGuy->width / 2)) {
		spr->x = spr->x + homingSpeed;
	}

	//If the egg is to the right of the user, move it towards the left at homingSpeed
	if (spr->x >(spriteGuy->x + spriteGuy->width / 2)) {
		spr->x = spr->x - homingSpeed;
	}

	draw_compiled_sprite(dest, eggImg[0], spr->x, spr->y); //Draw the compiled sprite of the egg

	if (maskSGCollisionTest(spr) == true) { //If egg collides with Sprite Guy
		play_sample(hurt, 255, 127, 1000, FALSE); //Play hurt sound effect
		if (hp > 0) { //If health is over 0
			hp--; //Lower the players health by 1
		}
	}
	if (collisionTest(spr, ground) == true || maskSGCollisionTest(spr)) { //If egg hits the ground or Sprite Guy
		drawSplat(spr->x, spr->y); //Draw splat bitmap in it's place
		spr->x = rand() % 993; //Assign randomly generated x coord
		spr->y = -(rand() % 300); //Assign randomly generated y coord, the bigger the number, the longer it takes to enter screen
	}

	if (state == 1) {
		if (collisionTest(spr, spriteGuy) == true) { //If egg collides with Sprite Guy
			play_sample(hurt, 255, 127, 1000, FALSE); //Play hurt sound effect
			if (hp > 0) { //If health is over 0
				hp--; //Lower the players health by 1
			}
		}
		if (collisionTest(spr, ground) == true || collisionTest(spr, spriteGuy)) { //If egg hits the ground or Sprite Guy
			drawSplat(spr->x, spr->y); //Draw splat bitmap in it's place
			spr->x = rand() % 993; //Assign randomly generated x coord
			spr->y = -(rand() % 300); //Assign randomly generated y coord, the bigger the number, the longer it takes to enter screen
		}
	}
}

//All drop functions below follow the same logic as above (dropEggs) - refer to it for comments on logic if needed
void dropPotions(BITMAP *dest, SPRITE *spr) {
	if (++spr->framecount > spr->framedelay) {
		spr->framecount = 0;

		//Animate
		if (++spr->curframe > spr->maxframe) {
			spr->curframe = 0;
		}

	}

	draw_compiled_sprite(dest, potionImg[0], spr->x, spr->y); //Draw potion

															  //If potion hits Sprite Guy
	if (maskSGCollisionTest(spr) == true) {
		int randomized = rand() % 3; //Generate random number for random effect

									 //GROW!
		if (randomized == 0) {
			play_sample(grow, 255, 127, 1000, FALSE); //Play grow sound effect
			transferFrame = spriteGuy->curframe; //Set the current frame to transferFrame so we can animate Sprite Guy at the same frame after he transforms
			set = true; //We need
			state = 2; //Set state to 2 to reflect GROW!
			lastState = 2; //Set lastState to keep track of what state he's coming from
		}

		//SHRINK - same logic as above
		if (randomized == 1) {
			play_sample(shrink, 255, 127, 1000, FALSE); //Play shrink sound effect
			transferFrame = spriteGuy->curframe;
			set = true;
			state = 1;
			lastState = 1;
		}

		//INVINCIBLE!
		if (randomized == 2) {
			play_sample(invincible, 255, 127, 1000, FALSE); //Play invincible sound effect
			lastHP = hp; //Store the HP so we can go back to it after chaning HP
			hp = 5000; //Set HP to very high number, mimics invincibility
			state = 3;
		}

	}

	//If potion hits the ground or Sprite Guy
	if (collisionTest(spr, ground) == true || maskSGCollisionTest(spr)) {
		spr->x = rand() % 993;
		spr->y = -(rand() % 6000 + 3000);
	}
	if (state != 0) { //If Sprite Guy is transformed, start a timer
		timer++;
	}
	if (timer == 600) { //Aster 10 seconds (60FPS*60s)
		transferFrame = spriteGuy->curframe; //Update the transferFrame so he gets animated with proper frame
		if (state == 1) { //If coming form state 1, we need to set him up for his new body size
			play_sample(grow, 255, 127, 1000, FALSE); //Play grow sound effect
			set = true;
		}
		if (state == 2) { //If coming from state 2, we need to set him up for his new body size
			play_sample(shrink, 255, 127, 1000, FALSE); //Play shrink sound effect
			set = true;
		}
		if (state == 3) { //If 3, invincible
			hp = lastHP; //Restore old HP to player
		}
		lastHP = 0; //Reset lastHP for next instance
		state = 0; //He's now back to normal
		timer = 0; //Reset the timer for next instance
	}
}

//dropHearts(BITMAP *dest, SPRITE *spr) - Drops hearts, follows same logic
void dropHearts(BITMAP *dest, SPRITE *spr) {

	if (++spr->framecount > spr->framedelay) {
		spr->framecount = 0;

		//Animate
		if (++spr->curframe > spr->maxframe) {
			spr->curframe = 0;
		}

	}

	draw_compiled_sprite(dest, heartImg[0], spr->x, spr->y); //Draw the heart

	if (maskSGCollisionTest(spr) == true) { //If collided with Sprite Guy
		play_sample(healed, 255, 127, 1000, FALSE); //Play healed sound effect

		if (hp < 3) { //If less than HP

			if (state == 3) { //If he's INVINCIBLE
				lastHP++; //Add these hearts to his lastHP so he gets them once he's out of invincibility
			}

			hp++; //Increase HP

		}

	}

	if (collisionTest(spr, ground) == true || maskSGCollisionTest(spr)) { //If it hits the ground, send it back up to come down later
		spr->x = rand() % 993;
		spr->y = -(rand() % 12000 + 10000);
	}

}

//drawGround(BITMAP *dest, SPRITE *spr) - Draws the ground.
void drawGround(BITMAP *dest, SPRITE *spr) {

	if (++spr->framecount > spr->framedelay) {
		spr->framecount = 0;

		//Animate
		if (++spr->curframe > spr->maxframe) {
			spr->curframe = 0;
		}

	}

	draw(dest, groundImg, spr); //Draw the ground
}

//controller(SPRITE *spr) - *spr = SPRITE. A controller that moves Sprite Guy, if left or right is pressed it moves accordingly, if space is pressed Sprite Guy jumps
void controller(SPRITE *spr) {

	//When a key is pressed
	if (keypressed()) {

		//When left is pressed
		if (key[KEY_LEFT] && !key[KEY_RIGHT]) { //Only if right key isn't pressed
			facingRight = false; //Update direction bools
			facingFront = false;
			facingLeft = true;
			turnLeft = true;
			goRight = false; //Set to tell he is no longer going right
			goLeft = true; //He is now going left

			spr->xVelocity = spr->xVelocity - sgAcceleration; //Apply acceleration
			leftRelease = true; //Set leftRelease to true so that we can apply decceleration when left is released

		}

		//When right is pressed
		if (key[KEY_RIGHT] && !key[KEY_LEFT]) { //Only if left key isn't pressed
			facingRight = true; //Update direction bools
			facingFront = false;
			facingLeft = false;
			turnRight = true;
			goLeft = false; //Set to tell he is no longer going left
			goRight = true; //He is now going right

			spr->xVelocity = spr->xVelocity + sgAcceleration; //Apply acceleration
			rightRelease = true; //Set rightRelease to true so that we can apply decceleration when right is released
		}

		if (!key[KEY_LEFT] && !key[KEY_RIGHT] || key[KEY_LEFT] && key[KEY_RIGHT]) { //If no key is being pressed, or both keys are being pressed, we want to stop Sprite Guy
			goRight = false; //No longer moving right
			facingFront = true; //Facting screen
			goLeft = false; //No longer moving left
							//If right key has been released
			if (rightRelease == true) {
				spr->xVelocity = spr->xVelocity - sgDecceleration; //Apply decceleration
				if (spr->xVelocity <= 0) { //When velocity is 0
					rightRelease = false; //No longer apply decceleration
				}
			}
			//If left key has been released
			if (leftRelease == true) {
				spr->xVelocity = spr->xVelocity + sgDecceleration; //Apply decceleration
				if (spr->xVelocity >= 0) { //When velocity is 0
					leftRelease = false; //No longer apply decceleration
				}
			}

		}

		//When space bar is pressed
		if (key[KEY_SPACE]) {

			//If he's allowed to jump
			if (canJump == true) {
				spr->yVelocity = jumpVelocity; //Apply velocity
				jumping = true;
			}
		}

		//When ESC is pressed
		if (key[KEY_ESC]) {
			gameOver = true;
		}

		//When CTRL+M is pressed - Unmute music
		if (mute == false && volume < 155 && key[KEY_LCONTROL] && key[KEY_M] || mute == false && volume < 155 && key[KEY_RCONTROL] && key[KEY_M]) {
			volume = 155;
			adjust_sample(bg, volume, 128, 1000, TRUE); //Unmute
			mute = true; //For key unpress check to allow it to be pressed again
		}

		//If unpress CTRL+M, allow it to be pressed again
		if (mute == true && !key[KEY_LCONTROL] && !key[KEY_M] || mute == true && !key[KEY_RCONTROL] && !key[KEY_M]) {
			mute = false;
		}

		//When CTRL+M is pressed - Mute music
		if (mute == false && volume > 0 && key[KEY_LCONTROL] && key[KEY_M] || mute == false && volume > 0 && key[KEY_RCONTROL] && key[KEY_M]) {
			volume = 0;
			adjust_sample(bg, volume, 128, 1000, TRUE); //Mute
			mute = true;
		}

	}

	//If Sprite Guy is not on the ground due to jumping
	if (collisionTest(spr, ground) == false && jumping == true) {
		canJump = false; //He's not allowed to jump
	}

	//When Sprite Guy reaches the ground
	if (collisionTest(spr, ground) == true && jumping == true && canJump == false) {
		spr->yVelocity = 0;
		spr->y = SCREEN_H - ground->height - spr->height;
		jumping = false; //He's no longer jumping
		canJump = true; //He's allowed to jump
	}

}

//score(int s) - Accepts int (counter) to determine how much time has passed
void score(BITMAP *dest, int s) {

	if (s == 60) { //If s is 60, then 1 second has passed
		seconds++; //Increment seconds
		ticks = 0; //Set ticks back to 0
		maxDropVelocity = maxDropVelocity + .05;
	}
	if (seconds == 60) { //If seconds is 60, a minute has passed
		mins++; //Increment minutes
		seconds = 0; //Set seconds to 0
	}
	textprintf_ex(dest, bodyFont, 0, 0, WHITE, -1, "Time %d:%d", mins, seconds); //Print to screen how much time has passed
}

void health(BITMAP *dest) {
	if (hp > 3) { //If player has over 3 HP then display INVINCIBLE!
		textprintf_ex(dest, bodyFont, SCREEN_W - text_length(bodyFont, "INVINCIBLE!"), 0, WHITE, -1, "INVINCIBLE!");
	}
	if (hp == 3) { //If player has 3 HP show 3 hearts
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 128, 5, 32, 32);
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 86, 5, 32, 32);
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 44, 5, 32, 32);
	}
	if (hp == 2) { //If player has 2 HP show 2 hearts
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 86, 5, 32, 32);
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 44, 5, 32, 32);
	}
	if (hp == 1) { //If player has 1 HP show 1 heart
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 44, 5, 32, 32);

	}
	if (hp == 0) { //If player has 0 HP, go to death screen
		dead = true;
		textprintf_ex(dest, bodyFont, SCREEN_W - text_length(bodyFont, "DEAD"), 0, WHITE, -1, "DEAD");
	}
}

//Thread0 - Threads the score and health to a new buffer to prevent flickering
void* thread0(void* data) {
	int thread_id = *((int*)data);

	while (gameOver == false && dead == false) {

		if (pthread_mutex_lock(&threadsafe)) {
			allegro_message("Error: thread mutex was locked");
		}

		acquire_bitmap(tempBuf);
		score(tempBuf, ticks);
		health(tempBuf);
		release_bitmap(tempBuf);

		if (pthread_mutex_unlock(&threadsafe)) {
			allegro_message("ERROR: thread mutex unlock error");
		}

	}
	pthread_exit(NULL);
	return NULL;
}

//***Main***
int main(void)
{
	//initialize
	allegro_init();
	set_color_depth(24);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, 1024, 768, 0, 0);
	install_timer();
	install_keyboard();
	srand(time(NULL));

	datFile = load_datafile("SpriteGuy.dat");

	int id;
	pthread_t pthread0;
	pthread_t pthread1;
	int threadid0 = 0;
	int threadid1 = 1;

	//Install digital sound driver
	if (install_sound(DIGI_AUTODETECT, MIDI_NONE, "") != 0) {
		allegro_message("Error initializing sound system"); //Tell user that sound couldn't be initialized
		return 1;
	}
	rectfill(screen, 0, 0, SCREEN_W, SCREEN_H, WHITE); //Colour the screen white

													   //***Declare/Define BITMAPS***
	buffer = create_bitmap(1024, 768); //Buffer screen
	splatBuffer = create_bitmap(1024, 768); //Splay buffer screen
	rectfill(splatBuffer, 0, 0, 1024, 768, makecol(255, 0, 255));

	BITMAP *temp; //To hold incoming sprites
	BITMAP *groundTemp = create_bitmap(1024, 64);
	BITMAP *smallTemp = create_bitmap(1280, 128);
	BITMAP *largeTemp = create_bitmap(3840, 384);
	splat = create_bitmap(32, 32);
	background = create_bitmap(1024, 768);
	hpHeart = create_bitmap(32, 32);
	tempBuf = create_bitmap(1024, 768);
	rectfill(tempBuf, 0, 0, 1024, 768, makecol(255, 0, 255));
	//****************************

	//***AUDIO***
	//Background song
	volume = 155;
	bg = (SAMPLE*)datFile[0].dat; //Load the bg.wav file, assign to intro
	if (!bg) { //If bg.wav can't be found
		allegro_message("Couldn't find the bg.wav file"); //Tell user. If this happens, make sure files are in proper folder
		return 1;
	}

	//Hurt sound - The rest follow the smae logic
	hurt = (SAMPLE*)datFile[8].dat;
	if (!hurt) {
		allegro_message("Couldn't find the hurt.wav file");
		return 1;
	}
	//Healed sound
	healed = (SAMPLE*)datFile[6].dat;
	if (!healed) {
		allegro_message("Couldn't find the healed.wav file");
		return 1;
	}

	//Grow sound
	grow = (SAMPLE*)datFile[5].dat;
	if (!grow) {
		allegro_message("Couldn't find the grow.wav file");
		return 1;
	}

	//Shrink sound
	shrink = (SAMPLE*)datFile[13].dat;
	if (!shrink) {
		allegro_message("Couldn't find the shrink.wav file");
		return 1;
	}

	//Invincible sound
	invincible = (SAMPLE*)datFile[9].dat;
	if (!invincible) {
		allegro_message("Couldn't find the invincible.wav file");
		return 1;
	}

	//Allocating bg.wav to soundcard voice
	play_sample(bg, volume, 128, 1000, TRUE);
	//***********

	//***Load Fonts***
	bodyFont = (FONT*)datFile[2].dat;//load_font("Fonts/body.pcx", NULL, NULL); //Font for body text
	titleFont = (FONT*)datFile[14].dat;//load_font("Fonts/title.pcx", NULL, NULL); //Font for title


									   //Menu loop
	while (true) {
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Sprite^ Guy^!") / 2, SCREEN_H / 2 - 130, BLACK, -1, "Sprite^ Guy^!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "watch out for the eggs") / 2, SCREEN_H / 2, BLACK, -1, "watch out for the eggs");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "...and maybe the potions too") / 2, SCREEN_H / 2 + 20, BLACK, -1, "...and maybe the potions too");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Try to survive for as long as you can") / 2, SCREEN_H / 2 + 40, BLACK, -1, "Try to survive for as long as you can");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(6000);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		release_bitmap(buffer);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Sprite^ Guy^!") / 2, SCREEN_H / 2 - 130, BLACK, -1, "Sprite^ Guy^!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, BLACK, -1, "CONTROLS");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1200);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Sprite^ Guy^!") / 2, SCREEN_H / 2 - 130, BLACK, -1, "Sprite^ Guy^!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, BLACK, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, BLACK, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump") / 2, SCREEN_H / 2, BLACK, -1, "spacebar to jump");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1200);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Sprite^ Guy^!") / 2, SCREEN_H / 2 - 130, BLACK, -1, "Sprite^ Guy^!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, BLACK, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, BLACK, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump") / 2, SCREEN_H / 2, BLACK, -1, "spacebar to jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, BLACK, -1, "ctrl+m to mute/unmute music");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1200);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Sprite^ Guy^!") / 2, SCREEN_H / 2 - 130, BLACK, -1, "Sprite^ Guy^!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, BLACK, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, BLACK, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump") / 2, SCREEN_H / 2, BLACK, -1, "spacebar to jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, BLACK, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+h to see controls again") / 2, SCREEN_H / 2 + 40, BLACK, -1, "ctrl+h to see controls again");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1200);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Sprite^ Guy^!") / 2, SCREEN_H / 2 - 130, BLACK, -1, "Sprite^ Guy^!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, BLACK, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, BLACK, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump") / 2, SCREEN_H / 2 + 0, BLACK, -1, "spacebar to jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, BLACK, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+h to see controls again") / 2, SCREEN_H / 2 + 40, BLACK, -1, "ctrl+h to see controls again");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ESC to quit at any time") / 2, SCREEN_H / 2 + 60, BLACK, -1, "ESC to quit at any time");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(2000);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Sprite^ Guy^!") / 2, SCREEN_H / 2 - 130, BLACK, -1, "Sprite^ Guy^!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, BLACK, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, BLACK, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump") / 2, SCREEN_H / 2, BLACK, -1, "spacebar to jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, BLACK, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+h to see controls again") / 2, SCREEN_H / 2 + 40, BLACK, -1, "ctrl+h to see controls again");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ESC to quit at any time") / 2, SCREEN_H / 2 + 60, BLACK, -1, "ESC to quit at any time");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Are you ready?") / 2, SCREEN_H / 2 + 120, BLACK, -1, "Are you ready?");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1500);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, WHITE);
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Sprite^ Guy^!") / 2, SCREEN_H / 2 - 130, BLACK, -1, "Sprite^ Guy^!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, BLACK, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, BLACK, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump") / 2, SCREEN_H / 2, BLACK, -1, "spacebar to jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, BLACK, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+h to see controls again") / 2, SCREEN_H / 2 + 40, BLACK, -1, "ctrl+h to see controls again");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ESC to quit at any time") / 2, SCREEN_H / 2 + 60, BLACK, -1, "ESC to quit at any time");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Are you ready?") / 2, SCREEN_H / 2 + 120, BLACK, -1, "Are you ready?");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "press any key to start...") / 2, SCREEN_H / 2 + 140, BLACK, -1, "press any key to start...");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		clear_bitmap(buffer);
		clear_keybuf();
		readkey();
		game = true;
		break;
	}


	//***Load BITMAPS***
	//Sand bitmap for ground
	temp = (BITMAP*)datFile[11].dat; //Load sand.bmp to temp bitmap
	for (int x = 0; x < 16; x++) {
		blit(temp, groundTemp, 0, 0, x * 64, 0, 64, 64);
	}

	groundImg[0] = compiled_grabframe(groundTemp, 1024, 64, 0, 0, 1, 0);
	destroy_bitmap(groundTemp); //Destory groundTemp, no longer needed
	destroy_bitmap(temp);

	temp = (BITMAP*)datFile[10].dat;
	potionImg[0] = compiled_grabframe(temp, 32, 32, 0, 0, 1, 0);
	destroy_bitmap(temp);

	temp = (BITMAP*)datFile[3].dat;
	//Compiled Grabforms for the eggs
	eggImg[0] = compiled_grabframe((BITMAP*)datFile[3].dat, 32, 32, 0, 0, 1, 0);
	egg2Img[0] = compiled_grabframe(temp, 32, 32, 0, 0, 1, 0);
	egg3Img[0] = compiled_grabframe(temp, 32, 32, 0, 0, 1, 0);
	egg4Img[0] = compiled_grabframe(temp, 32, 32, 0, 0, 1, 0);
	egg5Img[0] = compiled_grabframe(temp, 32, 32, 0, 0, 1, 0);

	temp = (BITMAP*)datFile[7].dat;
	heartImg[0] = compiled_grabframe(temp, 32, 32, 0, 0, 1, 0);
	blit(temp, hpHeart, 0, 0, 0, 0, 32, 32);

	//draw_sprite(buffer, groundTemp, 50, 50);
	groundHeight = temp->h; //Set the groundHeight to the height of the sand bitmap
	destroy_bitmap(temp); //Destroy temp bitmap for reuse

	temp = (BITMAP*)datFile[4].dat;
	blit(temp, splat, 0, 0, 0, 0, 32, 32);
	destroy_bitmap(temp);

	background = (BITMAP*)datFile[1].dat;


	//Load compiled sprite of Sprite Guy
	temp = (BITMAP*)datFile[12].dat;
	for (int n = 0; n < 20; n++) { //Iterate through 20 frames
		spriteGuyImg[n] = compiled_grabframe(temp, 128, 128, 0, 0, 20, n); //Grab each frame and new position and assign to element
	}
	for (int n = 20; n < 40; n++) { //Iterate through 20 frames
		spriteGuyImg[n] = compiled_grabframe(temp, 128, 128, 0, 0, 20, n); //Grab each frame and new position and assign to element
	}

	//Shrink and add to compiled Sprite Guy
	stretch_blit(temp, smallTemp, 0, 0, temp->w, temp->h, 0, 0, 1280, 128);
	for (int n = 0; n < 20; n++) { //Iterate through 20 frames
		spriteGuyImg[n + 40] = compiled_grabframe(smallTemp, 64, 64, 0, 0, 20, n); //Grab each frame and new position and assign to element
	}
	for (int n = 20; n < 40; n++) { //Iterate through 20 frames
		spriteGuyImg[n + 40] = compiled_grabframe(smallTemp, 64, 64, 0, 0, 20, n); //Grab each frame and new position and assign to element
	}

	//Enlarge and add to compiled Sprite Guy
	stretch_blit(temp, largeTemp, 0, 0, temp->w, temp->h, 0, 0, largeTemp->w, largeTemp->h);
	for (int n = 0; n < 20; n++) { //Iterate through 20 frames
		spriteGuyImg[n + 80] = compiled_grabframe(largeTemp, 192, 192, 0, 0, 20, n); //Grab each frame and new position and assign to element
	}
	for (int n = 20; n < 40; n++) { //Iterate through 20 frames
		spriteGuyImg[n + 80] = compiled_grabframe(largeTemp, 192, 192, 0, 0, 20, n); //Grab each frame and new position and assign to element
	}
	destroy_bitmap(smallTemp);
	destroy_bitmap(largeTemp);
	destroy_bitmap(temp); //Destroy bitmap for reuse
						  //******************

	spriteGuyGroundHeight = SCREEN_H - groundHeight - spriteGuyImg[0]->h; //Set the ground y height for Sprite Guy

																		  //Initialize Sprite Guy
	spriteGuy->x = ((SCREEN_W / 2) - (spriteGuyImg[0]->w / 2));
	spriteGuy->y = SCREEN_H - spriteGuyImg[0]->h - groundImg[0]->h;
	spriteGuy->width = 128;
	spriteGuy->height = 128;
	spriteGuy->xdelay = 1;
	spriteGuy->ydelay = 0;
	spriteGuy->xcount = 0;
	spriteGuy->ycount = 0;
	spriteGuy->xVelocity = 0;
	spriteGuy->yVelocity = 0;
	spriteGuy->curframe = 0;
	spriteGuy->maxframe = 119;
	spriteGuy->framecount = 0;
	spriteGuy->framedelay = 2;
	spriteGuy->animdir = 1;

	//Initialize Eggs
	egg->x = randomX;
	egg->y = randomY;
	egg->width = eggImg[0]->w;
	egg->height = eggImg[0]->h;
	egg->xdelay = 0;
	egg->ydelay = 0;
	egg->xcount = 0;
	egg->ycount = 0;
	egg->xVelocity = 0;
	egg->yVelocity = 0;
	egg->curframe = 0;
	egg->maxframe = 0;
	egg->framecount = 0;
	egg->framedelay = 1;
	egg->animdir = 1;

	randomX = rand() % 998;
	randomY = -(rand() % 300);
	egg2->x = randomX;
	egg2->y = randomY;
	egg2->width = eggImg[0]->w;
	egg2->height = eggImg[0]->h;
	egg2->xdelay = 0;
	egg2->ydelay = 0;
	egg2->xcount = 0;
	egg2->ycount = 0;
	egg2->xVelocity = 0;
	egg2->yVelocity = 0;
	egg2->curframe = 0;
	egg2->maxframe = 0;
	egg2->framecount = 0;
	egg2->framedelay = 1;
	egg2->animdir = 1;

	randomX = rand() % 998;
	randomY = -(rand() % 300);
	egg3->x = randomX;
	egg3->y = randomY;
	egg3->width = eggImg[0]->w;
	egg3->height = eggImg[0]->h;
	egg3->xdelay = 0;
	egg3->ydelay = 0;
	egg3->xcount = 0;
	egg3->ycount = 0;
	egg3->xVelocity = 0;
	egg3->yVelocity = 0;
	egg3->curframe = 0;
	egg3->maxframe = 0;
	egg3->framecount = 0;
	egg3->framedelay = 1;
	egg3->animdir = 1;

	randomX = rand() % 998;
	randomY = -(rand() % 300);
	egg4->x = randomX;
	egg4->y = randomY;
	egg4->width = eggImg[0]->w;
	egg4->height = eggImg[0]->h;
	egg4->xdelay = 0;
	egg4->ydelay = 0;
	egg4->xcount = 0;
	egg4->ycount = 0;
	egg4->xVelocity = 0;
	egg4->yVelocity = 0;
	egg4->curframe = 0;
	egg4->maxframe = 0;
	egg4->framecount = 0;
	egg4->framedelay = 1;
	egg4->animdir = 1;

	randomX = rand() % 998;
	randomY = -(rand() % 300);
	egg5->x = randomX;
	egg5->y = randomY;
	egg5->width = eggImg[0]->w;
	egg5->height = eggImg[0]->h;
	egg5->xdelay = 0;
	egg5->ydelay = 0;
	egg5->xcount = 0;
	egg5->ycount = 0;
	egg5->xVelocity = 0;
	egg5->yVelocity = 0;
	egg5->curframe = 0;
	egg5->maxframe = 0;
	egg5->framecount = 0;
	egg5->framedelay = 1;
	egg5->animdir = 1;

	//Initialize potion
	randomX = rand() % 998;
	randomY = -(rand() % 3000 + 1500);
	potion->x = randomX;
	potion->y = randomY;
	potion->width = eggImg[0]->w;
	potion->height = eggImg[0]->h;
	potion->xdelay = 0;
	potion->ydelay = 0;
	potion->xcount = 0;
	potion->ycount = 0;
	potion->xVelocity = 0;
	potion->yVelocity = 0;
	potion->curframe = 0;
	potion->maxframe = 0;
	potion->framecount = 0;
	potion->framedelay = 1;
	potion->animdir = 1;

	//Initialize heart
	randomX = rand() % 998;
	randomY = -(rand() % 12000 + 10000);
	heart->x = randomX;
	heart->y = randomY;
	heart->width = heartImg[0]->w;
	heart->height = heartImg[0]->h;
	heart->xdelay = 0;
	heart->ydelay = 0;
	heart->xcount = 0;
	heart->ycount = 0;
	heart->xVelocity = 0;
	heart->yVelocity = 0;
	heart->curframe = 0;
	heart->maxframe = 0;
	heart->framecount = 0;
	heart->framedelay = 1;
	heart->animdir = 1;

	//Initialize ground
	ground->x = 0;
	ground->y = SCREEN_H - groundImg[0]->h;
	ground->width = groundImg[0]->w;
	ground->height = groundImg[0]->h;
	ground->xdelay = 0;
	ground->ydelay = 0;
	ground->xcount = 0;
	ground->ycount = 0;
	ground->xVelocity = 0;
	ground->yVelocity = 0;
	ground->curframe = 0;
	ground->maxframe = 0;
	ground->framecount = 0;
	ground->framedelay = 1;
	ground->animdir = 1;


	//***Set Framerate***
	LOCK_VARIABLE(counter);
	LOCK_FUNCTION(Increment);
	install_int_ex(Increment, BPS_TO_TIMER(60)); //60FPS
												 //*******************

	id = pthread_create(&pthread0, NULL, thread0, (void*)&threadid0);

	//***Game Loop***
	while (gameOver == false && game == true) {
		while (dead == true) {
			if (keypressed()) {
				acquire_bitmap(buffer);
				rectfill(buffer, 0, 0, 1024, 768, WHITE);
				textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "You Died!") / 2, SCREEN_H / 2 - 80, BLACK, -1, "You^ Died^!");
				textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "...a horribly runny death") / 2, SCREEN_H / 2, BLACK, -1, "...a horribly runny death");
				textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Your total time was ##minutes and ## seconds") / 2, SCREEN_H / 2 + 20, BLACK, -1, "Your total time was %d minutes and %d seconds", mins, seconds); //Print to screen how much time has passed
				textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Press ESC to quit") / 2, SCREEN_H / 2 + 40, BLACK, -1, "Press ESC to quit");
				release_bitmap(buffer);
				blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
				if (key[KEY_ESC]) {
					dead = false;
					game = false;
					gameOver = true;
				}
			}
		}

		//When counter reaches 0, wait for one count
		while (counter == 0) {
			rest(1);
		}

		//While counter is greater than 0
		while (counter > 0) {
			pthread_mutex_lock(&threadsafe);
			//Assign the counter number to old_counter
			int old_counter = counter;

			//Draw the background/ground
			drawLevel(buffer);
			drawGround(buffer, ground);

			//Apply physics
			physics(spriteGuy);
			eggPhysics();
			potionPhysics();
			heartPhysics();

			//Warp if needed
			warpsprite(spriteGuy);

			//Move Sprite Guy
			controller(spriteGuy);

			//Draw Sprite Guy to buffer
			acquire_bitmap(buffer);
			//If moving right
			drawGround(buffer, ground);
			animate(buffer, spriteGuy);
			dropEggs(buffer, egg);
			dropEggs(buffer, egg2);
			dropEggs(buffer, egg3);
			dropHomingEgg(buffer, egg4);
			dropSmartEgg(buffer, egg5);
			dropPotions(buffer, potion);
			dropHearts(buffer, heart);
			pthread_mutex_unlock(&threadsafe);
			//When CTRL+H is pressed - Display controls
			if (key[KEY_LCONTROL] && key[KEY_H] || key[KEY_RCONTROL] && key[KEY_H]) {
				rectfill(buffer, 0, SCREEN_H - 20, text_length(font, "<- -> LEFT and RIGHT ARROW KEYS to MOVE. SPACEBAR to JUMP. CTRL-M to MUTE music. CTRL-H to show this HELP window. ESC to quit."), SCREEN_H, WHITE);
				textprintf_ex(buffer, font, 0, SCREEN_H - 15, BLACK, WHITE, "<- -> LEFT and RIGHT ARROW KEYS to MOVE. SPACEBAR to JUMP. CTRL-M to MUTE music. CTRL-H to show this HELP window. ESC to quit.");
			}
			release_bitmap(buffer);
			//Increment ticks for time count
			ticks++;

			if (ticks < 10) {
				rectfill(splatBuffer, 0, 0, 1024, 768, makecol(255, 0, 255));
			}

			//Display score
			//score(buffer, ticks);

			//Decrement counter
			counter--;

			//If frame is taking too long to compute, break out of the frame and just draw
			if (old_counter <= counter) {
				break;
			}
		}

		masked_blit(splatBuffer, buffer, 0, 0, 0, 0, 1024, 768);
		masked_blit(tempBuf, buffer, 0, 0, 0, 0, 1024, 768);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768); //Blit the buffer to the screen
		clear_bitmap(buffer); //Clear the buffer for next frame
		rectfill(tempBuf, 0, 0, 1024, 768, makecol(255, 0, 255));
	}
	//***************

	//Destroy compiled sprites
	for (int n = 0; n < 120; n++) {
		destroy_compiled_sprite(spriteGuyImg[n]);
	}
	pthread_mutex_destroy(&threadsafe);
	destroy_sample(bg);
	destroy_sample(healed);
	destroy_sample(hurt);
	destroy_sample(grow);
	destroy_sample(shrink);
	destroy_sample(invincible);
	return 0;
}

END_OF_MAIN();