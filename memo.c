/*
 *	REMEMBER TO ADD THIS C FILE IN THE MAKEFILE,
 *	AND ADD THE APPLICATION STRUCT IN THE APPLICATION HEADER FILE (application.h)
*/

#include "applications/application.h"
#include "protocol/protocol.h"
#include "drivers/light.h"
#include "drivers/timer.h"
#include "drivers/sensors.h"
#include "applications/game.h"
#include "applications/controller.h"

#define ALIVE_INTERVAL 100
#define LOST_INTERVAL 1500
#define BROADCAST_INTERVAL 400
#define INIT_INTERVAL 1000
#define CLICK_INTERVAL 450
#define RESULTS_TIME 1000
#define MAX_ROUNDS 4
#define MAX_TILES 6
#define MAX_PLAYERS 4
#define TIE_COLOR 4

static void GenerateSequenceMemo(uint8_t n);
static void DisplayTileMemo(uint8_t i);
static void TilePressedMemo(void);
static void CheckAnswerMemo(uint16_t pressedTileAddress);
static void ProcessAddress(uint16_t addrToProcess);
static void Blink(uint8_t col, uint16_t t);
static uint8_t NrOfActivePlayers(void);
static uint16_t CalcBlinkInterval(void);

static void SetNLedsCol(uint8_t cnt, uint8_t col);
static void SetOneLedCol(unsigned char nr, uint8_t col);

/* Create the application struct*/
static void Init(void);
static void Update(void);
static void PacketListener(struct frame_t *frame);
struct application_t memo = {Init, Update, PacketListener};


struct packetFormat {
  enum { TOALL_MYADDR , TOALL_RESTART , TOALL_GAMEEND , TOSLAVE_BLINK ,
	     TOALL_CHANGESTATE , TOMASTER_TILEPRESSED} cmd;
  uint8_t color;
  uint16_t time;
};

struct tile_tag {
  uint16_t address;
  struct timer_t timer;
};

static enum { GAME_INIT, GAME_START, CONFIRMATION, GENERATE, DISPLAY, REPEAT,
	          PLAYER_START, CONFIRM_START, PLAYER_END, CONFIRM_END, ROUND_END, GAME_END } gameState;

static struct tile_tag tileArray[MAX_TILES]; // Assume a max number of 6 tiles
static struct packetFormat packet;
static uint8_t nrOfTiles, nrOfTilesInRound, nrOfPressedTiles, nrOfDisplayedTiles;
static struct timer_t broadcastTimer, sequenceTimer, blinkTimer, initTimer;
static uint16_t randAddrSeq[MAX_ROUNDS+5], blinkTime, BLINK_INTERVAL;
uint8_t idx, playerID, nrOfPlayers, diffLevel, gameType, roundNr;
static bool playerStatus[MAX_PLAYERS], ONE_PLAYER;

/*
 * Initialize application, this function is called every time
 * the application is started or restarted
 */
static void Init(void) {
	game_init(sizeof(struct packetFormat));
	set_all_leds(LIGHT_OFF);

	nrOfTiles = 0;

	for (uint8_t i = 0; i < MAX_TILES; i++) {
		tileArray[i].address = BROADCAST; // Initialize the addresses
		timer_reset(&tileArray[i].timer);
	}

	timer_reset(&initTimer);
	set_all_leds(LIGHT_RED);
	delay_ms(300);
	set_all_leds(LIGHT_OFF);

	gameState = GAME_START;

	for (uint8_t i = 0; i < MAX_PLAYERS; i++)
	{
		playerStatus[i] = true;
	}
}

/*
 * Update application, this is the main function of the application.
 * This function is called
 */
static void Update(void) {

	if(gameState == GAME_START)
	{
		if(is_master() && timer_compare(&initTimer, INIT_INTERVAL))
		{
			playerID = 0;
			roundNr = 1;

			GameSettings();
			if(nrOfPlayers == 1) ONE_PLAYER = true;
			else ONE_PLAYER = false;

			BLINK_INTERVAL = CalcBlinkInterval();
			blinkTime = BLINK_INTERVAL;

			gameState = PLAYER_START;
		}
	}

	if(gameState == PLAYER_START)
	{
		for (uint8_t i = 0; i < 8; i++) {
			if(i < roundNr) SetOneLedCol(i,playerID);
			else SetOneLedCol(i , 4);
		}

		gameState = CONFIRM_START;
	}

	if(gameState == CONFIRM_START)
	{
		if (fsr_update())
		{
			set_all_leds(LIGHT_OFF);

			gameState = GENERATE;
		}
	}

	if(gameState == GENERATE)
	{
		nrOfTilesInRound = 2 + roundNr;
		nrOfPressedTiles = 0;
		nrOfDisplayedTiles = 0;


		GenerateSequenceMemo(nrOfTilesInRound);
		gameState = DISPLAY;
	}

	if(gameState == DISPLAY)
	{
		if(timer_compare(&sequenceTimer, BLINK_INTERVAL*2) && nrOfDisplayedTiles < nrOfTilesInRound)
		{
			DisplayTileMemo(nrOfDisplayedTiles);
			nrOfDisplayedTiles++;

			if (nrOfDisplayedTiles == nrOfTilesInRound)
			{
				gameState = REPEAT;

				packet.cmd = TOALL_CHANGESTATE;
				send_command_all(&packet);
			}
		}
	}

	if (gameState == REPEAT)
	{
		if(fsr_update())
		{
			TilePressedMemo();
		}

		if (is_master() && nrOfPressedTiles == nrOfTilesInRound)
		{
			if(timer_compare(&blinkTimer, blinkTime))
			{
				gameState = PLAYER_END;
			}
		}
	}

	if (gameState == PLAYER_END)
	{
		packet.cmd = TOALL_CHANGESTATE;
		send_command_all(&packet);

		if(playerStatus[playerID])
		{
			set_all_leds(LIGHT_GREEN);
		}
		else
		{
			set_all_leds(LIGHT_RED);
		}

		gameState = CONFIRM_END;
	}

	if(gameState == CONFIRM_END)
	{
		if (fsr_update())
		{
			set_all_leds(LIGHT_OFF);

			do   //find next active player
			{
				playerID++;
			} while(playerID < nrOfPlayers && !playerStatus[playerID]);

			if (playerID == nrOfPlayers)
			{
				gameState = ROUND_END;
			}
			else
			{
				gameState = PLAYER_START;
			}
		}
	}

	if (gameState == ROUND_END)
	{
		if(NrOfActivePlayers() == 0)
		{
			gameState = GAME_END;
		}
		else if(NrOfActivePlayers() == 1 && !ONE_PLAYER)
		{
			gameState = GAME_END;
		}
		else
		{
			if (roundNr == MAX_ROUNDS)
			{
				gameState = GAME_END;
			}
			else
			{
				roundNr++;
				playerID = 0;
				while(playerID < nrOfPlayers && !playerStatus[playerID]) playerID++;
				gameState = PLAYER_START;
			}
		}
	}

	if (gameState == GAME_END)
	{
		packet.cmd = TOALL_GAMEEND;

		if (NrOfActivePlayers() == 1)
		{
			for (playerID = 0; playerID < nrOfPlayers; playerID++)
			{
				if (playerStatus[playerID] == true) break;
			}

			packet.color = playerID;
		}
		else
		{
			packet.color = TIE_COLOR;
		}

		send_command_all(&packet);
	}


    if (timer_compare(&blinkTimer, blinkTime) && gameState != CONFIRM_START && gameState != CONFIRM_END)
    {
    	set_all_leds(LIGHT_OFF);
    }

    if(timer_compare(&broadcastTimer, BROADCAST_INTERVAL + rand_int(100)))
    {
    	packet.cmd = TOALL_MYADDR;
    	send_command_all(&packet);
    	timer_reset(&broadcastTimer);
    }
  
    for (uint8_t i = 0; i < MAX_TILES; i++)
    {
    	if (timer_compare(&tileArray[i].timer, LOST_INTERVAL))
    	{
    		tileArray[i].address = BROADCAST;
    		timer_reset(&tileArray[i].timer);
    	}
    }
}

/*
 * Packet listener, this function is called every time a packet is received
 * for the application on this device. The argument is the packet itself.
 * See protocol/frame.h for meta data and data in the packet.
 */
static void PacketListener(struct frame_t *frame) {
	struct packetFormat *receivedPacket;
	receivedPacket = (struct packetFormat *) frame->info;

	if ( receivedPacket->cmd == TOMASTER_TILEPRESSED )
	{
		CheckAnswerMemo(frame->address_from);
	}

	if ( receivedPacket->cmd == TOSLAVE_BLINK )
	{
		Blink(receivedPacket->color, receivedPacket->time);
	}

	if (receivedPacket->cmd == TOALL_CHANGESTATE)
	{
		if (!is_master())
		{
			if (gameState == REPEAT)
			{
				gameState = GAME_START;
			}
			else if (gameState == GAME_START)
			{
				gameState = REPEAT;
			}
		}
	}

	if (receivedPacket->cmd == TOALL_RESTART)
	{
		set_all_leds(LIGHT_RED);
		delay_ms(1000);
		restart_active_application();
	}

	if (receivedPacket->cmd == TOALL_GAMEEND)
	{
		for (uint8_t i = 0; i < 10; i++)
		{
			SetNLedsCol(8,receivedPacket->color);
			delay_ms(500);
			set_all_leds(LIGHT_OFF);
			delay_ms(500);
		}

		restart_active_application();
	}

	if( receivedPacket->cmd == TOALL_MYADDR )
	{
		ProcessAddress(frame->address_from);
	}
}

void GenerateSequenceMemo(uint8_t n)
{
	uint8_t idx = 0, i = 0;

	while(i < n)
	{
		idx = rand_int(nrOfTiles);
		if (tileArray[idx].address != BROADCAST)
		{
			randAddrSeq[i] = tileArray[idx].address;
			i++;
		}
	}
}

void DisplayTileMemo(uint8_t i)
{
	if (randAddrSeq[i] == my_master_addr())
	{
		Blink(playerID, BLINK_INTERVAL);
	}
	else
	{
		packet.cmd = TOSLAVE_BLINK;
		packet.color = playerID;
		packet.time = BLINK_INTERVAL;

		send_command(randAddrSeq[i] , &packet);
	}
	timer_reset(&sequenceTimer);
}

void TilePressedMemo(void)
{
	if (is_master())
	{
		CheckAnswerMemo(my_master_addr());
	}
	else
	{
		packet.cmd = TOMASTER_TILEPRESSED;
		send_command(my_master_addr(), &packet);
	}
}

void CheckAnswerMemo(uint16_t pressedTileAddress)
{
	if (pressedTileAddress != randAddrSeq[nrOfPressedTiles])
	{
		playerStatus[playerID] = false;
		gameState = PLAYER_END;
	}
	else
	{
		nrOfPressedTiles++;

		if(pressedTileAddress == my_master_addr())
		{
			Blink(playerID, BLINK_INTERVAL);
		}
		else
		{
			packet.cmd = TOSLAVE_BLINK;
			packet.color = playerID;
			packet.time = BLINK_INTERVAL;

			send_command(pressedTileAddress, &packet);
			timer_reset(&blinkTimer);    // another tile is blinking,
										 // wait with e.g. displaying round result
		}
	}
}

void ProcessAddress(uint16_t addrToProcess)
{
	uint8_t newAddressIdx = 255;
	bool addressKnown = false;

	//check if address is already known
	for (uint8_t i = 0; i < MAX_TILES; i++)
	{
		if (tileArray[i].address == addrToProcess)
		{
			addressKnown = true;
			timer_reset(&tileArray[i].timer);
			break;
		}
	}

	//if address is unknown find unused index and save
	if(!addressKnown && nrOfTiles < MAX_TILES)
	{
		for (uint8_t i = 0; i < MAX_TILES; i++)
		{
			if(tileArray[i].address == BROADCAST)
			{
				newAddressIdx = i;
				break;
			}
		}

		tileArray[newAddressIdx].address= addrToProcess;
		timer_reset(&tileArray[newAddressIdx].timer);
	}

	nrOfTiles = 0;
	for (uint8_t i = 0; i < MAX_TILES; i++)
	{
		if ( tileArray[i].address != BROADCAST )
		{
			nrOfTiles++;
		}
	}
}

void Blink(uint8_t col, uint16_t t)
{
	blinkTime = t;
	SetNLedsCol(8, col);
	timer_reset(&blinkTimer);
}

uint8_t NrOfActivePlayers(void)
{
	uint8_t cnt = 0;

	for (uint8_t i = 0; i < nrOfPlayers; i++)
	{
		if (playerStatus[i] == true)
			cnt++;
	}

	return cnt;
}

static void SetNLedsCol(uint8_t cnt, uint8_t col)
{
	set_all_leds(LIGHT_OFF);

	for(int i = 0; i < 8 && i < cnt; i++)
	{
		SetOneLedCol(i,col);
	}
}

static void SetOneLedCol(unsigned char nr, uint8_t col)
{
	if(col == 0) set_one_led(LIGHT_BLUE, nr);
	else if(col == 1) set_one_led(LIGHT_PURPLE, nr);
	else if(col == 2) set_one_led(LIGHT_CYAN, nr);
	else if(col == 3) set_one_led(LIGHT_YELLOW, nr);
	else if(col == 4) set_one_led(LIGHT_WHITE, nr);
	else if(col == 5) set_one_led(LIGHT_RED, nr);
	else if(col == 6) set_one_led(LIGHT_GREEN, nr);
	else if(col == 7) set_one_led(LIGHT_OFF, nr);
}

uint16_t CalcBlinkInterval(void) {

	return 1200/diffLevel + 20*nrOfTiles;
}
