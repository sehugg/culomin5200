/* Curse of the lost miner for Atari 5200. 32KB cartridge.
 * Created by BAKTRA Software in 2015
 * 
 * I have placed this work in the Public Domain, thereby relinquishing
 * all copyrights. Everyone is free to use, modify, republish, sell or give away
 * this work without prior consent from anybody.
 * 
 * History:
 * Version 1.0
 * - Dealt with ATTRACT mode
 * - Controls temporarily locked when miner lands
 * - Pause and Reset buttons
 * - Minor optimizations
 * 
 * Release candidate 01
 * - Adjusted for NTSC
 * - Sound effects and music
 * - "Controls cleanup" restricted to the FIRE button 
 * - Better color palette adjusted for NTSC
 * 
 * Prototype 003.
 * - Fixed keypad controls
 * - Fixed initialization of POTs.
 */

/*Memory usage :
 * 
 * Pure read only data in the cartridge:
 * -------------------------------------
 * Program and read only data                    : 16384
 * Second character set (1024 bytes)             : 
 * Third character set (1024) bytes              : 
 * Cave display list (190 bytes,k boudnary)      : 
 * Cave elements - 13+1 caves(3108 bytes)        : 
 * 
 * 
 * Read/Write display areas:
 * -------------------------
 * PMG one-line resolution (2k)                  : 4096 - 6143 PAGE:16 OFFSET:  0
 * Cave display memory (22x40=880 bytes)         : 6144 - 7023 PAGE:24 OFFSET:  0
 * Cave status bar (40 bytes)                    : 7024 - 7083 PAGE:27 OFFSET:112
 * Menu display memory(960 bytes)                : 15872 -16352 
 * 
 * Read/Write sound effects and music
 * ----------------------------------
 * All relocated from cartridge ROM
 * Raster Music Tracker Aux1                     : 7554
 * Raster Music Tracker Aux2                     : 7680
 * Raster Music Tracker Main                     : 7936, Entry point 8192 
 * RMT File                                      : 9216 (0x2400)
 */

#pragma codesize(100)

//#link "rmt_sup.s"
//#link "data.s"
//#resource "clmfont1.fnt"
//#resource "clmfont2.fnt"
//#resource "rmt_aux1.bin"
//#resource "rmt_aux2.bin"
//#resource "levels.dat"
//#resource "rmt_main.bin"
//#resource "rmt_music.bin"

/*Memory layout constants*/
#define MA_CAVDMEM 6144U
#define MA_PMGPAGE 16U
#define MA_PMGSTART 4096U
#define MA_PMGEND 6143U
#define MA_SBMEM 7024U
#define MA_RMT_AUX1 7554U
#define MA_RMT_AUX2 7680U
#define MA_RMT_MAIN 7936U
#define MA_RMT_MUSIC 9216U

extern unsigned char CLM_DATA_CHSET1;
extern unsigned char CLM_DATA_CHSET2;
extern unsigned char CLM_DATA_CAVES;
extern unsigned char CLM_DATA_DL_CAVE;
extern unsigned char CLM_RMT_AUX1;
extern unsigned char CLM_RMT_AUX2;
extern unsigned char CLM_RMT_MAIN;
extern unsigned char CLM_RMT_MUSIC;
extern unsigned char CLM_RMT_MUSIC_END;


/*Caves*/
#define MAX_CAVE_INDEX (12)
#define NUMBER_OF_CAVES (13)
#define TRAINING_CAVE_INDEX (13)
#define CAVESIZE (222)

/*Control speed*/
#define CTRL_DELAY (5)


/*Control threshold*/
#define JS_LEFT (76)
#define JS_RIGHT (152)
#define JS_UP (76)
#define JS_DOWN (152)

/*Keypad*/
#define KPAD_NONE (0xFF)
#define KPAD_0    (0x00)
#define KPAD_ASTERISK (0x0A)
#define KPAD_PAUSE (0x0D)
#define KPAD_RESET (0x0E)

/*Logical joystick position*/
#define JS_LOG_CENTER (0)
#define JS_LOG_LEFT (1)
#define JS_LOG_RIGHT (2)
#define JS_LOG_UP (4)
#define JS_LOG_DOWN (8)

#define JS_LOG_UP_LEFT  (5)
#define JS_LOG_UP_RIGHT (6)

#define POT_HORIZONTAL (0x11)
#define POT_VERTICAL (0x12)

/*Movement when miner is falling*/
#define FALL_FLAG_NONE (0)
#define FALL_FLAG_LEFT (1)
#define FALL_FLAG_RIGHT (2)
#define FALL_FLAG_LEFT_AND_RIGHT (3)
#define FALL_FLAG_FALLING (4)

/*Game over type*/
#define GAME_OVER_NONE  (0)
#define GAME_OVER_DEATH  (1)
#define GAME_OVER_SUCCESS  (2)
#define GAME_OVER_USER_QUIT (3)

/*Game type*/
#define GAME_TYPE_NORMAL (0)
#define GAME_TYPE_TRAINING (1)

/*Game speed*/
#define GAME_SPEED_NORMAL (0)
#define GAME_SPEED_SLOW (1)


#include <stdio.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdlib.h>
#include <string.h>
#include <atari5200.h>
#include <6502.h>


/*Main game routine*/
void doGame(void);
void relocateRmt(void);


/*Caves and cave elements*/
void paintElement(unsigned char x, unsigned char y, unsigned char elem);
void paintCave(void);
void rebuildCaveElementArray(unsigned char cv);

/*Miner - PMG*/
void pmgInit(void);
void setMinerPos(unsigned char x, unsigned char y);
void repaintMiner(void);

/*Time and timing*/
void delay(unsigned int w);
void adjustGameSpeed(unsigned char speed);


/*Text mode displays*/
void displayCongratulations(void);
void displayGameOver(void);
void displayReturnToMenuScreen(void);
void setTextModeSettings(void);
void displayMainMenu(void);
void displayTrainingText(void);

/*Display game speed and starting cave*/
void displayGameSpeed(void);
void displayStartingCave(void);

/*SFX and music routines*/
void rmtInitMenuMusic(void);
void rmtInitGameMusic(void);
void rmtInitGameOverMusic(void);
void rmtInitDummyMusic(void);
void rmtPlayDiamond(void);
void rmtPlayPicked(void);
void rmtPlayDeath(void);
void rmtPlayUpdate(void);
void rmtPlayGratulation(void);
void rmtPlayJump(void);
void rmtAllStop(void);
void rmtSuspend(void);
void rmtResume(void);
void rmtSetVBI(void);
void rmtRestoreVBI(void);

/*Reboot*/
void asmReboot();

/*Miner movement*/
unsigned char moveLeft(void);
unsigned char moveRight(void);
void moveUp(void);
void moveDown(void);
unsigned char jumpUp(void); /*Return 0 if OK, 1 if death*/
void fallDown(void);
void handleHighJump(void);
void updateStatusBar(void);
unsigned char checkTreasure(void);
void checkDeath(void);

/*Pause*/
void handlePause(void);

unsigned char maxCaveReached; /*Max. warp*/
unsigned char startingCave; /*Warp*/
unsigned char dmactlStore; /*DMA CTL shadow Store*/
unsigned char lives; /*Lives*/

unsigned char currentCave; /*Current cave*/
unsigned char diamondsInCave; /*How many diamonds in cave*/
unsigned char diamondsCollected; /*How many diamonds collected*/
unsigned char caveDeath; /*1 for death*/
unsigned char caveAllPicked; /*1 for all picked*/
unsigned char stayHere; /*Stay in cave*/
unsigned char gameOverType;
unsigned char gameSpeed; /*Game speed*/
unsigned char gameType; /*Game type, normal or training*/

/*Menu displaylist adress*/
unsigned char menuDl1;
unsigned char menuDl2;

/*Temporary variables for general use*/
unsigned char x1;
unsigned char y1;
unsigned int i1;
unsigned int i2;
unsigned char z1;

/*Current cave status*/
unsigned char caveElements[20][22];
unsigned char caveBroken [20][22];

/*Miner location*/
unsigned char minerX, minerY;
int p0x, p0y;

/*Miner - PMG P0. Normal miner and jumping miner*/
const unsigned char minerDataNormal[] = {60, 126, 90, 219, 255, 195, 102, 60};
const unsigned char minerDataJump[] = {60, 126, 90, 219, 255, 195, 126, 0};

/*"Training" and "PAUSED" literals*/
const unsigned char trainingLiteral[] = {52, 50, 33, 41, 46, 41, 46, 39};
const unsigned char pausedLiteral[] = {48, 33, 53, 51, 37, 36};

/*High jump*/
unsigned char hijs;
unsigned char hiJump;

/*Movement control - allocated in asm source*/
extern unsigned char mvDelay;
extern unsigned char dliHandler;

/*Colors changed in DLI*/
extern unsigned char colorStore1;
extern unsigned char colorStore2;

/*Keypad*/
extern unsigned char keypadCont;
extern unsigned char keypadKey;
extern unsigned char keypadDisable;

extern unsigned char secondFire;
extern unsigned char breakHandler;

/*Pointer to current 'look and feel' of the miner*/
const unsigned char* minerData = minerDataNormal;

/*Movement speed setup. The higher the number, the slower the movement*/
unsigned char brokenSpeed;
unsigned char hijumpSpeedA;
unsigned char hijumpSpeedB;
unsigned char controlDelay;
unsigned char fallSpeed;


/*Cave elements*/

/*These elements do not require translation*/
#define E_BLANK (0)
#define E_ROCK_FULL (1)
#define E_ROCK_TL (2)
#define E_ROCK_TR (3)
#define E_ROCK_BL (4)
#define E_ROCK_BR (5)
#define E_ROCK_UNSTABLE (6)
#define E_LADDER (7)
#define E_DEATH_BOTTOM_TOP (8)
#define E_DEATH_TOP_BOTTOM (9)

/*Diamonds. In external representation, there is one code. 
 *Internally, we have three types of diamonds
 */
#define EXT_E_DIAM (14)
#define E_DIAM_F (10)
#define E_DIAM_L (12)

/*Broken rock has one external code and 8 internals for different
 states of decay
 */
#define EXT_E_ROCK_BROKEN (15)
#define E_ROCK_BROKEN_F (13)
#define E_ROCK_BROKEN_L (20)

/*The skull is a display-only element*/
#define E_SKULL (21)
#define E_SKULL_2 (22)

/*Element attribute masks*/

/*                          0  0  0  0  0  0  0  0  0  0  1  1  1  1  1  1  1  1  1  1  2  
                            0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9  0 */
const unsigned char passable[] = {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
const unsigned char notJump [] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
const unsigned char broken[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
/*                          B  R           R  U  L  D  D  D  D  D  B
 *                          L  O           O  N  A  E  E  I  I  I  R
 *                          A  C           C  S  D  A  A  A  A  A  O
 *                          N  K           K  T  D  D  D  M  M  M  K
 *                          K                       BT TB          1  2  3  4  5  6  7  8
 */

/*Map cave elements to character pairs*/
const unsigned char elem2CharMap[] = {

    0, /*BLANK*/

    64 + 128, /*ROCK FULL*/

    72 + 128, /*ROCK TL*/
    74 + 128, /*ROCK TR*/
    70 + 128, /*ROCK BL*/
    68 + 128, /*ROCK BR*/

    90 + 128, /*ROCK UNSTABLE*/

    78, /*LADDER*/

    66 + 128, /*DEATH BOTTOM TOP*/
    76 + 128, /*DEATH TOP BOTTOM*/

    84, /*DIAM 1*/
    86, /*DIAM 2*/
    88, /*DIAM 3*/

    97 + 128, /*BROKEN 1*/
    99 + 128, /*BROKEN 2*/
    101 + 128, /*BROKEN 3*/
    103 + 128, /*BROKEN 4*/
    105 + 128, /*BROKEN 5*/
    107 + 128, /*BROKEN 6*/
    109 + 128, /*BROKEN 7*/
    111 + 128, /*BROKEN 8*/

    80, /*SKULL*/
    82 /*SKULL 2*/
};

int main() {

    /*General game status*/
    gameOverType = GAME_OVER_NONE;
    gameSpeed = GAME_SPEED_NORMAL;
    maxCaveReached = 0;

    /*Clear screen*/
    clrscr();

    /*Relocate music and sound effects*/
    relocateRmt();

    /*Prepare to read keypad*/
    POKE(0x00, 64 + 32+ 128);
    POKEY_WRITE.irqen = 64 + 32+ 128;
    POKEY_WRITE.skctl = 0x02;
    POKE(0x20A, ((unsigned int) &keypadCont) % 256);
    POKE(0x20B, ((unsigned int) &keypadCont) / 256);
     
    POKE(0x20C,((unsigned int) &breakHandler) % 256);
    POKE(0x20D,((unsigned int) &breakHandler) / 256);
    
    CLI();

    delay(5);

    /*Cheat/debug helper.*/
    if (keypadKey == KPAD_ASTERISK) {
        maxCaveReached = MAX_CAVE_INDEX;
        keypadKey = KPAD_NONE;
    }

    /*Initialize our VBI routine*/
    rmtSuspend();
    ANTIC.nmien = 0;
    rmtSetVBI();
    ANTIC.nmien = 64;

    menuDl1 = PEEK(0x05);
    menuDl2 = PEEK(0x06);

    cursor(0);

    /*Init the PMG*/
    pmgInit();

    /*Enclosing loop*/
    while (1) {

        /*How the game ended before?*/
        switch (gameOverType) {

            case GAME_OVER_DEATH:
            {
                rmtSuspend();
                rmtInitGameOverMusic();
                rmtResume();
                displayGameOver();
                break;
            }

            case GAME_OVER_SUCCESS:
            {
                rmtSuspend();
                rmtInitDummyMusic();
                rmtResume();
                displayCongratulations();
                break;
            }
            case GAME_OVER_USER_QUIT:
            {
                rmtSuspend();
                rmtInitDummyMusic();
                rmtResume();
                displayReturnToMenuScreen();
                break;
            }

        }


        /*Menu music*/
        rmtSuspend();
        rmtInitMenuMusic();
        rmtResume();



        /*Draw main menu*/
        setMinerPos(1, 6);
        displayMainMenu();


        /*Select controller 1 and enable POTs*/
        GTIA_WRITE.consol = 0x04;

        /*Clear any controls*/
        while (GTIA_READ.trig0 == 0/* ||
                PEEK(POT_HORIZONTAL) <JS_LEFT ||
                PEEK(POT_HORIZONTAL) >JS_RIGHT ||
                PEEK(POT_VERTICAL)<JS_UP ||
                PEEK(POT_VERTICAL)>JS_DOWN*/
                );

        /*The menu loop*/
        while (1) {

            /*Start game ?*/
            if (GTIA_READ.trig0 == 0) {
                gameType = GAME_TYPE_NORMAL;
                break;
            }

            /*Starting cave - increase*/
            if (PEEK(POT_HORIZONTAL) > JS_RIGHT) {
                if (startingCave < maxCaveReached) {
                    startingCave++;
                    displayStartingCave();
                    delay(15);
                    continue;
                }
            }

            /*Starting cave - decrease*/
            if (PEEK(POT_HORIZONTAL) < JS_LEFT) {
                if (startingCave > 0) {
                    startingCave--;
                    displayStartingCave();
                    delay(15);
                    continue;
                }
            }

            /*Game speed toggle*/
            if (PEEK(POT_VERTICAL) > JS_DOWN) {
                gameSpeed = 1 - gameSpeed;
                displayGameSpeed();
                delay(20);
                continue;
            }

            /*Training*/
            if (PEEK(POT_VERTICAL) < JS_UP) {
                displayTrainingText();
                gameType = GAME_TYPE_TRAINING;
                break;
            }

        }

        /*Set gameover*/
        gameOverType = GAME_OVER_NONE;

        /*Set game speed*/
        adjustGameSpeed(gameSpeed);


        /*Setup the music*/
        rmtSuspend();



        rmtAllStop();
        rmtInitGameMusic();
        rmtResume();

        /*Game routine*/
        doGame();

    }/*Enclosing loop*/

    return 0;
}

void doGame() {

    /*Joystick*/
    unsigned char js;
    unsigned char strig;

    /*Elements near miner*/
    unsigned char probeBelow;
    unsigned char probeMiner;

    /*Gravity*/
    unsigned char fallTimer;
    unsigned char fallCounter;
    unsigned char fallLength;
    unsigned char fallMovementFlags;
    unsigned char landLock;


    /*Broken rock timer*/
    unsigned char breakTimer;

    /*Quit flag*/
    unsigned char caveQuit;

    /*High jump in progress flag*/
    unsigned char hijump;

    /*DLI interrupt address*/
    unsigned char* dliadr;


    /*Set display list and character set*/
    POKE(0x05, ((unsigned int) &CLM_DATA_DL_CAVE) % 256);
    POKE(0x06, ((unsigned int) &CLM_DATA_DL_CAVE) / 256);


    /*Set current cave and number of lives*/
    if (gameType == GAME_TYPE_NORMAL) {
        currentCave = startingCave;
    } else {
        currentCave = TRAINING_CAVE_INDEX;
    }

    lives = 4;

    /*Set DLI and enable it*/
    dliadr = &dliHandler;
    ANTIC.nmien = 96;
    POKE(0x206, (unsigned int) dliadr % 256);
    POKE(0x207, (unsigned int) dliadr / 256);
    ANTIC.nmien = 192;

    /*Cave loop*/
    while (1) {

        /*Do not show anything*/
        dmactlStore = PEEK(0x07);
        POKE(0x07, 0);

        /*Rebuild cave aray*/
        rebuildCaveElementArray(currentCave);
        diamondsCollected = 0;

        /*Paint the cave and update status bar*/
        if ((currentCave & 0x03) < 2) {
            POKE(0x0D, 12); /*White*/
            POKE(0x0E, 0x96); /*Blue*/
            POKE(0x0C, 0x32); /*Dark brown*/
            POKE(0x0F, 0x34); /*Lighter brown*/
            POKE(0x10, 0); /*Background*/
        } else {
            POKE(0x0D, 12); /*White*/
            POKE(0x0E, 0xD8); /*Green*/
            POKE(0x0C, 0x54); /*Dark purple*/
            POKE(0x0F, 0x56); /*Lighter purple*/
            POKE(0x10, 0); /*Background*/
        }

        /*Store the colors for DLI routine*/
        colorStore1 = PEEK(0x0D);
        colorStore2 = PEEK(0x0C);


        paintCave();
        updateStatusBar();

        /*Place the miner*/
        minerData = minerDataNormal;
        setMinerPos(minerX, minerY);

        /*Initialize game status variables*/
        stayHere = 1;
        fallCounter = 0;
        caveDeath = 0;
        caveAllPicked = 0;
        hijump = 0;
        fallLength = 0;
        fallMovementFlags = FALL_FLAG_NONE;
        landLock = 0;
        caveQuit = 0;
        probeBelow = E_ROCK_FULL;
        keypadKey = KPAD_NONE;
        secondFire = 0;

        /*Show the cave*/
        POKE(0x07, dmactlStore);

        /*Controls and physics loop*/
        while (stayHere) {

            /* Read keypad*/
            if (keypadKey != KPAD_NONE) {

                /*Keypad * or RESET - Return to menu*/
                if (keypadKey == KPAD_ASTERISK || keypadKey == KPAD_RESET) {
                    keypadKey = KPAD_NONE;
                    stayHere = 0;
                    caveQuit = 1;
                    break;
                }

                /*Keypad 0 - Commit Suicide*/
                if (keypadKey == KPAD_0) {
                    keypadKey = KPAD_NONE;
                    caveDeath = 1;
                    stayHere = 0;
                    break;
                }

                /*Keypad PAUSE - Pause game*/
                if (keypadKey == KPAD_PAUSE) {
                    handlePause();
                }


            }

            /*Whats is behind the miner a what is below the miner?*/
            probeMiner = caveElements[minerX][minerY];
            probeBelow = caveElements[minerX][minerY + 1];

            /*Gravity - If there is nothing below the miner and the miner is not on a ladder, he falls down.*/
            if (passable[probeBelow] == 1 && probeMiner != E_LADDER && probeBelow != E_LADDER) {
                if (fallTimer != PEEK(0x02)) {
                    fallCounter++;
                    fallTimer = PEEK(0x02);
                    if (fallCounter == fallSpeed) {
                        fallDown();
                        fallLength++;
                        checkDeath();
                        fallCounter = 0;
                        if (fallLength > 6) {
                            stayHere = 0;
                            caveDeath = 1;
                            continue;
                        }
                    }
                }
                fallMovementFlags |= FALL_FLAG_FALLING;
                landLock = 0;
            } else {
                if ((fallMovementFlags & (FALL_FLAG_LEFT_AND_RIGHT)) != 0) {
                    landLock = 2;
                }
                fallCounter = 0;
                fallLength = 0;
                fallMovementFlags = FALL_FLAG_NONE;
            }

            /*There is a broken rock under the miner. It decays*/
            if (broken[probeBelow] == 1) {
                y1 = minerY + 1;
                if (breakTimer != PEEK(0x02)) {
                    caveBroken[minerX][y1]++;
                    breakTimer = PEEK(0x02);
                }
                if (caveBroken[minerX][y1] == brokenSpeed) {
                    caveBroken[minerX][y1] = 0;
                    if (probeBelow < E_ROCK_BROKEN_L) {
                        caveElements[minerX][y1]++;
                        paintElement(minerX, y1, caveElements[minerX][y1]);
                    } else {
                        caveElements[minerX][y1] = E_BLANK;
                        paintElement(minerX, y1, caveElements[minerX][y1]);
                    }
                }

            }

            /*Unstable rock under the miner*/
            if (probeBelow == E_ROCK_UNSTABLE) {
                caveElements[minerX][minerY + 1] = E_BLANK;
                paintElement(minerX, minerY + 1, E_BLANK);
            }

            /*Controls*/
            if (mvDelay == 0) {

                js = JS_LOG_CENTER;

                if (PEEK(POT_HORIZONTAL) < JS_LEFT) {
                    js += JS_LOG_LEFT;
                } else if (PEEK(POT_HORIZONTAL) > JS_RIGHT) {
                    js += JS_LOG_RIGHT;
                }

                if (PEEK(POT_VERTICAL) < JS_UP) {
                    js += JS_LOG_UP;
                } else if (PEEK(POT_VERTICAL) > JS_DOWN) {
                    js += JS_LOG_DOWN;
                }
                strig = GTIA_READ.trig0;


                switch (js) {

                        /*Joystick right*/
                    case JS_LOG_UP_RIGHT:
                    case JS_LOG_RIGHT:
                    {

                        /*With trigger - Medium jump to right*/
                        if (strig == 0 && !(notJump[probeBelow])) {
                            rmtPlayJump();
                            /*Display miner as jumping one*/
                            minerData = minerDataJump;
                            fallLength = 0;
                            if (jumpUp()) break;
                            delay(CTRL_DELAY);
                            if (jumpUp()) break;
                            delay(CTRL_DELAY);
                            moveRight();
                            delay(CTRL_DELAY);
                            moveRight();
                            delay(CTRL_DELAY);
                            moveRight();
                            delay(CTRL_DELAY);
                            checkDeath();
                            /*Display miner as normal*/
                            minerData = minerDataNormal;
                            repaintMiner();
                            break;
                        }
                        /*Without trigger - Move to the right*/

                        /*When falling - allow move to the right just once*/
                        if ((fallMovementFlags & FALL_FLAG_FALLING) == FALL_FLAG_FALLING) {
                            if ((fallMovementFlags & FALL_FLAG_RIGHT) == 0) {
                                if (moveRight()) fallMovementFlags |= FALL_FLAG_RIGHT;
                            }
                        }/*Otherwise just move to the right*/
                        else {
                            if (landLock == 0) {
                                moveRight();
                            } else {
                                --landLock;
                                mvDelay = controlDelay;
                            }
                        }


                        checkDeath();
                        break;
                    }

                        /*Joystick left*/
                    case JS_LOG_UP_LEFT:
                    case JS_LOG_LEFT:
                    {
                        /*With trigger - Medium jump to the left*/
                        if (strig == 0 && !(notJump[probeBelow])) {
                            /*Medium jump to left*/
                            rmtPlayJump();

                            /*Display miner as jumping one*/
                            minerData = minerDataJump;

                            fallLength = 0;
                            if (jumpUp()) break;
                            delay(CTRL_DELAY);
                            if (jumpUp()) break;
                            delay(CTRL_DELAY);
                            moveLeft();
                            delay(CTRL_DELAY);
                            moveLeft();
                            delay(CTRL_DELAY);
                            moveLeft();
                            delay(CTRL_DELAY);
                            checkDeath();

                            /*Display miner as normal one*/
                            minerData = minerDataNormal;
                            repaintMiner();
                            break;
                        }
                        /*Without trigger - Simply movet to the left*/

                        /*When falling - allow move to the left just once*/
                        if ((fallMovementFlags & FALL_FLAG_FALLING) == FALL_FLAG_FALLING) {
                            if ((fallMovementFlags & FALL_FLAG_LEFT) == 0) {
                                if (moveLeft()) fallMovementFlags |= FALL_FLAG_LEFT;
                            }
                        }/*Otherwise just move to the left*/
                        else {
                            if (landLock == 0) {
                                moveLeft();
                            } else {
                                --landLock;
                                mvDelay = controlDelay;
                            }

                        }
                        checkDeath();
                        break;
                    }


                        /*Joystick down, move down (ladder only)*/
                    case JS_LOG_DOWN:
                    {
                        moveDown();
                        checkDeath();
                        break;
                    }

                        /*Joystick up, move up (ladder only) or jump high*/
                    case JS_LOG_UP:
                    {
                        if (strig == 0) {
                            if (notJump[probeBelow]) break;
                            fallLength = 0;
                            rmtPlayJump();
                            handleHighJump();
                            break;
                        }
                        moveUp();
                        checkDeath();
                        break;
                    }

                    default:
                    {
                        mvDelay = 0;
                        landLock = 0;
                    }

                }/*End switch js*/
            }

        }/*End of controls and physics loop*/

        /* Return to main menu by user request*/
        if (caveQuit) {
            /*Hide the miner*/
            setMinerPos(-8, 32);
            gameOverType = GAME_OVER_USER_QUIT;
            break;
        }

        /* Treasure collected. Advance to next cave or complete the game*/
        if (caveAllPicked) {

            /*If training, then return to the main menu*/
            if (gameType == GAME_TYPE_TRAINING) {
                setMinerPos(-8, 32);
                gameOverType = GAME_OVER_USER_QUIT;
                break;
            }

            currentCave++;
            if (currentCave == NUMBER_OF_CAVES) {
                /*Hide the miner*/
                setMinerPos(-8, 32);
                /*Show congratulations screen before menu*/
                gameOverType = GAME_OVER_SUCCESS;
                break;
            } else {
                if (currentCave > maxCaveReached) maxCaveReached = currentCave;
            }
            /*Play positive sound*/
            rmtPlayPicked();
            continue;
        }
        if (caveDeath) {

            /*Let the miner fall to the ground if possible*/
            if ((fallMovementFlags & FALL_FLAG_FALLING) == FALL_FLAG_FALLING) {
                while (minerY < 22 && (caveElements[minerX][minerY + 1] == E_BLANK)) {
                    minerY++;
                    setMinerPos(minerX, minerY);
                    delay(3);
                }
            }

            /*Let the miner dissapear*/
            setMinerPos(-8, 32);

            /*Display a skull with blinking eye*/
            paintElement(minerX, minerY, E_SKULL);
            rmtPlayDeath();
            delay(15);
            paintElement(minerX, minerY, E_SKULL_2);
            delay(15);
            paintElement(minerX, minerY, E_SKULL);
            delay(5);

            /*Check remaining lives lives*/
            if (lives == 0) {
                gameOverType = GAME_OVER_DEATH;
                break;
            }
            lives--;

            continue;
        }



    }/*End of outer loop*/

    /*Inhibit DLI*/
    ANTIC.nmien = 96;
}

/*Paint element at specific location*/
void paintElement(unsigned char x, unsigned char y, unsigned char elem) {

    /*Target memory*/
    i2 = MA_CAVDMEM + (y * 40)+(x << 1);

    /*Mapping for element*/
    z1 = elem2CharMap[elem];

    POKE(i2, z1);
    POKE(i2 + 1, z1 + 1);

}

/*Paint whole cave*/
void paintCave() {

    for (y1 = 0; y1 < 22; ++y1) {
        for (x1 = 0; x1 < 20; ++x1) {
            paintElement(x1, y1, caveElements[x1][y1]);
        }
    }
}

/*Rebuild cave*/
void rebuildCaveElementArray(unsigned char cv) {

    /*Elements*/
    unsigned char elems[2];
    int ec = 0;


    /*Element Pointer*/
    unsigned char* p;

    /*Coordinates*/
    unsigned char x, y;

    /*Determine character set (even cave or odd cave)*/
    if ((cv & 0x01U) != 0) {
        ANTIC.chbase = (((unsigned int) &CLM_DATA_CHSET2) >> 8);
    } else {
        ANTIC.chbase = (((unsigned int) &CLM_DATA_CHSET1) >> 8);
    }


    /*Point to the cave beginning*/
    p = (unsigned char*) (&CLM_DATA_CAVES);
    p += cv*CAVESIZE;

    /*Reset number of diamonds in the cave*/
    diamondsInCave = 0;

    /*Determine miner position*/
    minerY = *p;
    p++;
    minerX = *p;
    p++;

    for (y = 0; y < 22; y++) {
        for (x = 0; x < 20; x += 2) {

            elems[0] = (*(p) >> 4);
            elems[1] = (*(p)&0x0F);

            for (ec = 0; ec < 2; ec++) {

                /*Translate special elements*/
                switch (elems[ec]) {
                    case EXT_E_DIAM:
                    {
                        diamondsInCave++;
                        elems[ec] = E_DIAM_F + (diamondsInCave % 3);
                        break;
                    }
                    case EXT_E_ROCK_BROKEN:
                    {
                        elems[ec] = E_ROCK_BROKEN_F;
                        break;
                    }
                }
            }

            caveElements[x][y] = elems[0];
            caveElements[x + 1][y] = elems[1];
            p++;

        }
    }


    /*Clear the broken array*/
    memset(caveBroken, 0, 440);
}

/*Player missile graphics*/
void pmgInit() {

    /*Set PMG memory start*/
    ANTIC.pmbase = MA_PMGPAGE;

    /*PMG single line resolution*/
    POKE(0x07, 62);

    /*Clear all PMG memory*/
    memset((void*) MA_PMGSTART, 0, (MA_PMGEND - MA_PMGSTART + 1));

    /*Enable PMG*/
    GTIA_WRITE.gractl = 2;

    /*Player 0 will be green*/
    POKE(0x08, 0xC8);

    /*Initial coordinates*/
    p0x = 128;
    GTIA_WRITE.hposp0 = p0x;
    p0y = 0;

}

/*Place miner at given coordinates*/
void setMinerPos(unsigned char x, unsigned char y) {
    p0x = 48 + (x << 3);
    memset(((unsigned char*) p0y + MA_PMGSTART + 1024), 0, 8);
    p0y = 32 + (y << 3);
    GTIA_WRITE.hposp0 = p0x;
    memcpy((unsigned char*) p0y + MA_PMGSTART + 1024, minerData, 8);
}

/*Just repaint the miner*/
void repaintMiner() {
    memcpy(((unsigned char*) (32 + (minerY << 3) + MA_PMGSTART + 1024)), minerData, 8);
}

/*Wait for some time*/
void delay(unsigned int w) {
    unsigned int i = 0;
    for (i = 0; i < w; i++) {
        unsigned char a = PEEK(0x02);
        while (PEEK(0x02) == a) {
        }
    }
}

/*Move commands with range and pass checking*/
unsigned char moveLeft() {
    if (minerX == 0 || passable[caveElements[minerX - 1][minerY]] == 0) return 0;
    minerX--;
    setMinerPos(minerX, minerY);
    checkTreasure();
    mvDelay = controlDelay;
    return 1;
}

unsigned char moveRight() {
    if (minerX == 19 || passable[caveElements[minerX + 1][minerY]] == 0) return 0;
    minerX++;
    setMinerPos(minerX, minerY);
    checkTreasure();
    mvDelay = controlDelay;
    return 1;
}

void moveDown() {
    if (minerY == 21) return;
    x1 = caveElements[minerX][minerY + 1];
    if (passable[x1] == 1) {
        minerY++;
        setMinerPos(minerX, minerY);
        checkTreasure();
        mvDelay = controlDelay;
    }
}

void fallDown() {
    if (minerY == 21) return;
    x1 = caveElements[minerX][minerY + 1];
    if (passable[x1] == 1) {
        minerY++;
        setMinerPos(minerX, minerY);
        checkTreasure();
    }
}

void moveUp() {
    if (minerY == 0) return;
    x1 = caveElements[minerX][minerY - 1];
    /*Not free*/
    if (passable[x1] == 0) return;

    /*We can move up only when we are on the ladder a passable element is above*/
    if (caveElements[minerX][minerY] == E_LADDER) {
        /*Into death*/
        if (x1 == E_DEATH_TOP_BOTTOM) {
            stayHere = 0;
            caveDeath = 1;
            return;
        }
        minerY--;
        setMinerPos(minerX, minerY);
        checkTreasure();
        mvDelay = controlDelay;

    }

}

unsigned char jumpUp() {
    if (minerY == 0) return 0;
    x1 = caveElements[minerX][minerY - 1];
    /*Into death*/
    if (x1 == E_DEATH_TOP_BOTTOM) {
        stayHere = 0;
        caveDeath = 1;
        return 1;
    }
    /*Not free*/
    if (passable[x1] == 0) return 0;

    minerY--;
    setMinerPos(minerX, minerY);
    checkTreasure();
    return 0;
}

/*This is special function to handle high jump.*/
void handleHighJump() {

    unsigned char hjMaxTicks = hijumpSpeedA;
    unsigned char hjTicks = 0;
    unsigned char hjFlipFlop;

    /*Jumping miner*/
    minerData = minerDataJump;


    y1 = 0; /*Side move flag*/
    hiJump = 3; /*Jump power - 3 steps*/
    mvDelay = 0; /*Reset Movement delay*/

    /*When there is still jump power*/
    while (hiJump > 0) {

        /*If the miner is blocked - jump is complete*/
        if (jumpUp()) break;

        /*Time window for side movement*/
        while (hjTicks < hjMaxTicks) {

            /*Time to allow controls*/
            if (mvDelay == 0) {

                hijs = JS_LOG_CENTER;
                if (PEEK(POT_HORIZONTAL) < JS_LEFT) {
                    hijs = JS_LOG_LEFT;
                } else if (PEEK(POT_HORIZONTAL) > JS_RIGHT) {
                    hijs = JS_LOG_RIGHT;
                }

                /*Allow only single left or right move during the jump*/
                switch (hijs) {
                    case (JS_LOG_LEFT):
                    {
                        if (y1) break;
                        if (moveLeft()) y1 = 1;
                        break;
                    }
                    case (JS_LOG_RIGHT):
                    {
                        if (y1) break;
                        if (moveRight()) y1 = 1;
                        break;
                    }
                    default:
                    {
                        mvDelay = 0;
                        break;
                    }
                }/*End of switch hiJS*/

            }
            if (hjFlipFlop != PEEK(0x02)) {
                hjFlipFlop = PEEK(0x02);
                hjTicks++;
            }
        }
        if (hiJump == 2) hjMaxTicks = hijumpSpeedB;
        hjTicks = 0;
        hiJump--;
    }
    /*Jump is ended*/
    /*Check for death*/
    checkDeath();

    /*Normal miner*/
    minerData = minerDataNormal;
    repaintMiner();

}

void checkDeath() {
    x1 = caveElements[minerX][minerY + 1];
    if (x1 == E_DEATH_BOTTOM_TOP) {
        stayHere = 0;
        caveDeath = 1;
    }
}

unsigned char checkTreasure() {
    x1 = caveElements[minerX][minerY];
    if (x1 >= E_DIAM_F && x1 <= E_DIAM_L) {
        diamondsCollected++;
        caveElements[minerX][minerY] = E_BLANK;
        paintElement(minerX, minerY, E_BLANK);
        rmtPlayDiamond();
        if (diamondsCollected == diamondsInCave) {
            stayHere = 0;
            caveAllPicked = 1;
        }
        return 1;
    }
    return 0;
}

void updateStatusBar() {

    /*Clear*/
    memset((unsigned char*) MA_SBMEM, 0, 40);

    /*Lives*/
    for (y1 = 0; y1 < lives; y1++) {
        POKE(MA_SBMEM + y1, 123);
    }

    /*Current cave*/
    if (gameType == GAME_TYPE_TRAINING) {
        memcpy((char*) (MA_SBMEM + 32), trainingLiteral, 8);
    } else {
        x1 = (40 - NUMBER_OF_CAVES) + currentCave + 1;
        for (y1 = 40 - NUMBER_OF_CAVES; y1 < x1; y1++) {
            POKE(MA_SBMEM + y1, 96);
        }
    }

}

/*Display main menu*/
void displayMainMenu() {

    /*Disable DMA and setup display list*/
    dmactlStore = PEEK(0x07);
    POKE(0x07, 0);
    POKE(0x05, menuDl1);
    POKE(0x06, menuDl2);

    /*Set colors*/
    setTextModeSettings();
    clrscr();
    cursor(0);

    /*Game title*/

    /*             12345678901234567890 */
    cputsxy(0, 0, "    CURSE OF THE");
    cputsxy(0, 1, "     LOST MINER");
    cputsxy(0, 3, "BAKTRA SOFTWARE 2015");

    /*Description of menu controls*/
    cputsxy(1, 8, "FIRE    start game");
    cputsxy(1, 10, "JS L/R  cave");
    cputsxy(1, 11, "JS DOWN speed");
    cputsxy(1, 13, "JS UP   training");

    /*Instructions*/
    cputsxy(1, 20, "guide the miner");
    cputsxy(1, 21, "through dangerous");
    cputsxy(1, 22, "caves and collect");
    cputsxy(1, 23, "all diamonds");

    /*Variable portions*/
    displayStartingCave();
    displayGameSpeed();

    /*Reenable DMA*/
    POKE(0x07, dmactlStore);
}

/*Display game speed*/
void displayGameSpeed() {
    if (gameSpeed == GAME_SPEED_NORMAL) {
        cputsxy(15, 11, "NORM");
    } else {
        cputsxy(15, 11, "SLOW");
    }
}

void displayStartingCave() {
    gotoxy(15, 10);
    cprintf("%02d", startingCave + 1);
}

void adjustGameSpeed(unsigned char speed) {
    /*Normal game speed*/
    if (speed == GAME_SPEED_NORMAL) {

        brokenSpeed = 17;
        hijumpSpeedA = 6;
        hijumpSpeedB = 20;
        controlDelay = 8;
        fallSpeed = 4;

    }/*Slower game speed*/
    else {
        brokenSpeed = 25;
        hijumpSpeedA = 8;
        hijumpSpeedB = 26;
        controlDelay = 8;
        fallSpeed = 5;
    }

}

/*Show Congratulations*/
void displayCongratulations() {

    /*Use the menu DL*/
    POKE(0x07, 0);
    POKE(0x05, menuDl1);
    POKE(0x06, menuDl2);
    clrscr(); /*1234567890123456789012345678901234567890*/
    cputsxy(2, 4, "CONGRATULATIONS");
    cputsxy(2, 6, "the curse of the");
    cputsxy(2, 7, "lost miner");
    cputsxy(2, 8, "has been broken");
    setTextModeSettings();
    POKE(0x07, dmactlStore);
    delay(50);

    for (x1 = 0; x1 < 3; x1++) {
        rmtPlayGratulation();
        delay(50);
    }

    delay(100);
    cputsxy(2, 13, "press FIRE");
    while (GTIA_READ.trig0 != 0);


}

/*Show game over*/
void displayGameOver() {

    /*Use the menu DL*/
    POKE(0x07, 0);
    POKE(0x05, menuDl1);
    POKE(0x06, menuDl2);

    clrscr(); /*1234567890123456789012345678901234567890*/
    cputsxy(4, 11, "game is over");

    /*Show the screen again*/
    setTextModeSettings();
    POKE(0x07, dmactlStore);
    delay(150);

    cputsxy(5, 13, "press FIRE");

    while (GTIA_READ.trig0 != 0);

}

/*Display "Returning to the main menu ...*/
void displayReturnToMenuScreen() {

    /*Use the menu DL*/

    POKE(0x07, 0);
    POKE(0x05, menuDl1);
    POKE(0x06, menuDl2);

    /*Just blank screen*/
    clrscr();
    cputsxy(0, 10, "returning to menu...");
    /*Show the screen again*/
    setTextModeSettings();
    POKE(0x07, dmactlStore);

    /*Wait for a while*/
    delay(125);

}

/*Display instructions*/
void displayTrainingText() {

    /*Clear screen*/
    clrscr();
    setMinerPos(-8, 32);
    /*12345678901234567890*/
    cputsxy(0, 0, "use joystick to");
    cputsxy(0, 1, "control the miner");

    /*Movement*/
    cputsxy(0, 3, "MOVEMENT");
    cputsxy(0, 4, "js left  left");
    cputsxy(0, 5, "js right right");
    cputsxy(0, 6, "js up   up(ladder)");
    cputsxy(0, 7, "js down down(ladder)");

    /*Jumps*/
    cputsxy(0, 9, "JUMPING");
    cputsxy(0, 10, "fire+up");
    cputsxy(0, 11, "high jump");

    cputsxy(0, 12, "fire+left or right");
    cputsxy(0, 13, "long jumps");

    cputsxy(0, 15, "miner can jump only");
    cputsxy(0, 16, "when there is a rock");
    cputsxy(0, 17, "or a ladder below");

    cputsxy(0, 19, "avoid spikes");
    cputsxy(0, 20, "press 0 for suicide");
    cputsxy(0, 21, "press * for menu");


    cputsxy(0, 23, "press FIRE");

    /*Wait for FIRE button*/
    while (GTIA_READ.trig0 != 0);

}

/*Colors and character set for text mode*/
void setTextModeSettings() {
    POKE(0x0C, 255); /*Capitals - emphasize*/
    POKE(0x0D, 14); /*Minuscules - normal*/
    POKE(0x10, 50); /*Background*/
    ANTIC.chbase = 0xF8;
}

void handlePause() {

    unsigned char colors[5];

    /*Disable keypad*/
    keypadDisable = 1;
    while (keypadKey == KPAD_PAUSE);

    /*Dummy music*/
    rmtSuspend();
    rmtInitDummyMusic();
    rmtResume();

    /*Backup colors and make everything dark*/
    memcpy(colors, (unsigned char*) 0x0C, 5);
    memset((unsigned char*) 0x0C, 0, 5);
    POKE(0x08, 0x00);

    /*Show "PAUSED text*/
    memset((unsigned char*) MA_SBMEM, 0, 40);
    memcpy((unsigned char*) MA_SBMEM, pausedLiteral, 6);

    /*Reenable keypad*/
    delay(30);
    keypadKey = KPAD_NONE;
    keypadDisable = 0;

    /*Wait for pause*/
    while (keypadKey != KPAD_PAUSE);

    /*Disable keypad*/
    keypadDisable = 1;
    delay(30);
    keypadKey = KPAD_NONE;

    /*Game music*/
    rmtSuspend();
    rmtInitGameMusic();
    rmtResume();



    /*Restore colors and normal status bar*/
    memcpy((unsigned char*) 0x0C, colors, 5);
    POKE(0x08, 0xC8);
    updateStatusBar();

    /*Enable keypad*/
    keypadDisable = 0;

}

/*Relocate RMT routines and music file*/
void relocateRmt() {

    /*RMT Player*/
    memcpy((unsigned char*) MA_RMT_AUX1,
            &(CLM_RMT_AUX1),
            (unsigned char*) &CLM_RMT_AUX2 - (unsigned char*) &CLM_RMT_AUX1);

    memcpy((unsigned char*) MA_RMT_AUX2,
            &(CLM_RMT_AUX2),
            (unsigned char*) &CLM_RMT_MAIN - (unsigned char*) &CLM_RMT_AUX2);

    memcpy((unsigned char*) MA_RMT_MAIN,
            &(CLM_RMT_MAIN),
            (unsigned char*) &CLM_RMT_MUSIC - (unsigned char*) &CLM_RMT_MAIN);

    /*RMT music file*/
    memcpy((unsigned char*) MA_RMT_MUSIC,
            &(CLM_RMT_MUSIC),
            (unsigned char*) &CLM_RMT_MUSIC_END - (unsigned char*) &CLM_RMT_MUSIC);

}