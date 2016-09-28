/*
 * controller.h
 *
 *  Created on: 20/05/2015
 *      Author: s142203
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "applications/application.h"
#include "protocol/protocol.h"
#include "drivers/light.h"
#include "drivers/timer.h"
#include "drivers/sensors.h"
#include "applications/game.h"

#define CLICK_INTERVAL 450

//----------------------- GAME SETTINGS functions and variables ---------------------------

static uint8_t detectClicks(void);
static void reactionToClick (void);
void GameSettings(void);

static uint8_t isClicked, possTripClick, firstTime , nrClicks;
static uint8_t reset;
static uint8_t lastClick, mode, gameParam[3];
static struct timer_t click_timer;

extern uint8_t idx, playerID, nrOfPlayers, diffLevel, gameType, roundNr;
//----------------------- GAME SETTINGS functions and variables ---------------------------

#endif /* CONTROLLER_H_ */
