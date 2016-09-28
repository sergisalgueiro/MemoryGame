/*
 * controller.c
 *
 *  Created on: 20/05/2015
 *      Author: s142203
 */

#include "applications/controller.h"

//----------------------- GAME SETTINGS ---------------------------

void GameSettings(void)
{
	  if (is_master()){

		  set_one_led(LIGHT_BLUE, 0);
		  set_one_led(LIGHT_RED, 4);

		  //parameters for the clicking recognition

		  timer_reset(&click_timer);
		  isClicked = 0;
		  possTripClick = 0;
		  firstTime = 1;
		  lastClick = 0;
		  reset = 1;

		  // other parameters

		  mode = 1;
		  for(int i = 0; i<3; i++) {gameParam[i]=1;}

		  //parameter selection

		  while(reset){

				//------------------begining of clicking recognition part------------------------//
				//function returns variable lastClick = 1,2 or 3, depending on the click type//
				//lastClick has to be reseted after every 'use'

				nrClicks = detectClicks();

			// reaction to clicking

				reactionToClick ();


		  }
	  }
}

static uint8_t detectClicks() {

	uint8_t temp;

	if(is_master() && fsr_update()){

		//clickCount(&isClicked);

		if(isClicked < 3) (isClicked)++;

		if(isClicked==1) timer_reset(&click_timer);
	}


	if((!timer_compare(&click_timer, CLICK_INTERVAL) && isClicked == 2) || possTripClick == 1){

		if(firstTime){
			timer_reset(&click_timer);
			possTripClick=1;
			firstTime=0;
		}

		if(!timer_compare(&click_timer, CLICK_INTERVAL) && isClicked == 3){		//triple click
			isClicked = 0;
			possTripClick = 0;
			firstTime=1;

			lastClick = 3;
		}

		if(timer_compare(&click_timer, CLICK_INTERVAL) && isClicked == 2){		// double click
			isClicked = 0;
			possTripClick = 0;
			firstTime=1;

			lastClick=2;
		}
	}


	if(timer_compare(&click_timer, CLICK_INTERVAL) && isClicked == 1){		//single click
		isClicked = 0;

		lastClick=1;
	}

	temp = lastClick;
	lastClick = 0;

	return temp;

}

static void reactionToClick () {

	//triple click use

	if(nrClicks == 3){
		set_all_leds(LIGHT_CYAN);
		delay_ms(1000);
		reset = 0;
	}

	// double click use, mode changing

	if(nrClicks == 2){
		mode++;
		if(mode>3) mode = 1;
	}

	// inside modes, single click used

	switch(mode){

		case 1:

		set_all_leds(LIGHT_OFF);

		for(int i = 0; i<mode; i++) {set_one_led(LIGHT_GREEN, i);}

		for(int i = 0; i<gameParam[mode-1]; i++) {set_one_led(LIGHT_RED, i+3);}

		if(nrClicks == 1){

					gameParam[mode-1]++;
					if(gameParam[mode-1]>5) gameParam[mode-1] = 1;
		}

		break;

		case 2:

		set_all_leds(LIGHT_OFF);

		for(int i = 0; i<mode; i++) {set_one_led(LIGHT_GREEN, i);}

		for(int i = 0; i<gameParam[mode-1]; i++)
		{
			if(i == 0) set_one_led(LIGHT_BLUE, 3);
			else if (i == 1) set_one_led(LIGHT_PURPLE, 4);
			else if (i == 2) set_one_led(LIGHT_CYAN, 5);
			else if (i == 3) set_one_led(LIGHT_YELLOW, 6);
		}

		if(nrClicks == 1){

					gameParam[mode-1]++;
					if(gameParam[mode-1]>4) gameParam[mode-1] = 1;
		}

		break;

		case 3:

		set_all_leds(LIGHT_OFF);

		for(int i = 0; i<mode; i++) {set_one_led(LIGHT_GREEN, i);}

		for(int i = 0; i<gameParam[mode-1]; i++) {set_one_led(LIGHT_RED, i+3);}

		if(nrClicks == 1){

						gameParam[mode-1]++;
						if(gameParam[mode-1]>5) gameParam[mode-1] = 1;
		}

		break;



	}
	gameType = gameParam[0];
	nrOfPlayers = gameParam[1];
	diffLevel = gameParam[2];
}
