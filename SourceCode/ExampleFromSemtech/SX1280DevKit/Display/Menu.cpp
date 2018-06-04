/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Display demo menus and manage touch sensor.

Maintainer: Gregory Cristian & Gilbert Menth
*/

#include "mbed.h"
#include "Menu.h"
#include "DisplayDriver.h"
#include "Eeprom.h"
#include "GpsMax7.h"
#include "DemoApplication.h"
#include "SX9306.h"
#include "main.h"


/*!
 * \brief Maximum character count on the same line with function DrawText
 * This include needed space for the outline (at the begin and the end of line).
 * MAX_CHAR_PER_BTN is the same, but for a button.
 */
#define MAX_CHAR_PER_LINE       28

#define SCALE_LINE_COUNT        5
#define RANGING_FULLSCALE_MIN   10
#define RANGING_FULLSCALE_MAX   30000

/*!
 * \brief Bitmap of Semtech Logo stored here (copied from a C file).
 */
extern uint8_t SemtechLogo[];


/*!
 * \brief This ticker give the rythme to check X and Y coordinates of activated
 * touched region (if pressed). CheckScreenFlag keep the status if Touchscreen
 * has been checked or not, until checked.
 */
Ticker CheckScreenTouch;
volatile bool CheckScreenFlag = false;

/*!
 * \brief This ticker give the rythme to refresh a page when continuous info
 * has to be printed in the same page.
 */
Ticker CheckPageRefresh;
volatile bool PageRefresh = false;

/*!
 * \brief This ticker give the rythme to avoid quick menu change
 */
Ticker DebouncedScreenTouch;
volatile bool ScreenBeenDebounced = false;

/*!
 * \brief List og graphical object in the application.
 */
enum MenuObjectsId
{
    PAGE_BORDER = 0,
    TITLE_LINE,
    TITLE_TEXT,
    FIRM_LOGO,
    BTN0_BORDER,       // to accord with DrawActiveButton( ), each button should
    BTN0_MENU_TEXT,    // have consecutively MENU_TEXT (full large text),
    BTN0_LABEL_TEXT,   // LABEL_TEXT (left side of the button) and VALUE_TEXT
    BTN0_VALUE_TEXT,   // (right side of the button).
    BTN1_BORDER,
    BTN1_MENU_TEXT,
    BTN1_LABEL_TEXT,
    BTN1_VALUE_TEXT,
    BTN2_BORDER,
    BTN2_MENU_TEXT,
    BTN2_LABEL_TEXT,
    BTN2_VALUE_TEXT,
    BTN3_BORDER,
    BTN3_MENU_TEXT,
    BTN3_LABEL_TEXT,
    BTN3_VALUE_TEXT,
    BTN4_BORDER,
    BTN4_MENU_TEXT,
    BTN4_LABEL_TEXT,
    BTN4_VALUE_TEXT,
    BTN5_BORDER,
    BTN5_MENU_TEXT,
    BTN5_LABEL_TEXT,
    BTN5_VALUE_TEXT,
    BTN6_BORDER,
    BTN6_MENU_TEXT,
    BTN6_LABEL_TEXT,
    BTN6_VALUE_TEXT,
    BTN7_BORDER,
    BTN7_MENU_TEXT,
    BTN8_BORDER,
    BTN8_MENU_TEXT,
    BTN9_BORDER,
    BTN9_MENU_TEXT,
    LINE0_TEXT,
    LINE1_TEXT,
    LINE2_TEXT,
    LINE2_COL0_TEXT,
    LINE2_COL1_TEXT,
    LINE3_TEXT,
    LINE3_COL0_TEXT,
    LINE3_COL1_TEXT,
    LINE3_COL2_TEXT,
    LINE4_TEXT,         // to accord with DrawText( ), each text line should
    LINE4_COL0_TEXT,    // have consecutively TEXT (full large text),
    LINE4_COL1_TEXT,    // COL0_TEXT (left side of the line), COL1_TEXT (center
    LINE4_COL2_TEXT,    // region of the line) and COL2_TEXT (right side of the
    LINE5_COL0_TEXT,    // line). COL0..2 are optionals if doesn't exist.
    LINE5_COL1_TEXT,
    LINE5_COL2_TEXT,
    LINE6_COL0_TEXT,
    LINE6_COL1_TEXT,
    LINE6_COL2_TEXT,
    LINE7_COL0_TEXT,
    LINE7_COL1_TEXT,
    LINE7_COL2_TEXT,
    LINE8_COL0_TEXT,
    LINE8_COL1_TEXT,
    LINE9_COL0_TEXT,
    LINE9_COL1_TEXT,
    RNG_DIST_TEXT,
    RNG_DIST_CIRCLE,
    RNG_DS1_TEXT,
    RNG_DS2_TEXT,
    RNG_DS3_TEXT,
    RNG_DS4_TEXT,
    RNG_DS5_TEXT,
    RNG_DS0_LINE,
    RNG_DS1_LINE,
    RNG_DS2_LINE,
    RNG_DS3_LINE,
    RNG_DS4_LINE,
    RNG_DS5_LINE,
    RNG_MST_SCR,
};
#define GRAPH_OBJECTS_COUNT     78
GraphObject_t MenuObjects[GRAPH_OBJECTS_COUNT] =
{
    // { Id,           Type,      Xpos, Ypos, Height, Width, LineW, BackColor, FrontColor, DoFill, FillCol, Source, TouchActive }
    { PAGE_BORDER,     GO_RECTANGLE, 0,   0,   320,    240,   1, BACK_COLOR, PAGE_COLOR,    false, NULL, NULL,        false },
    { TITLE_LINE,      GO_LINE,      0,   30,  1,      240,   1, BACK_COLOR, PAGE_COLOR,    false, NULL, NULL,        false },
    { TITLE_TEXT,      GO_TEXT,      15,  9,   NULL,   208,   1, BACK_COLOR, WHITE,         false, NULL, NULL,        false },
    { FIRM_LOGO,       GO_IMAGE,     20,  37,  95,     215,   1, BACK_COLOR, WHITE,         false, NULL, SemtechLogo, false },
    { BTN0_BORDER,     GO_RECTANGLE, 10,  40,  30,     106,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN0_MENU_TEXT,  GO_TEXT,      15,  48,  NULL,   96,    1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN0_LABEL_TEXT, GO_TEXT,      15,  48,  NULL,   32,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { BTN0_VALUE_TEXT, GO_TEXT,      47,  48,  NULL,   64,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { BTN1_BORDER,     GO_RECTANGLE, 10,  75,  30,     222,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN1_MENU_TEXT,  GO_TEXT,      15,  83,  NULL,   200,   1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN1_LABEL_TEXT, GO_TEXT,      15,  83,  NULL,   64,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { BTN1_VALUE_TEXT, GO_TEXT,      79,  83,  NULL,   152,   1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { BTN2_BORDER,     GO_RECTANGLE, 10,  110, 30,     222,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN2_MENU_TEXT,  GO_TEXT,      15,  118, NULL,   200,   1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN2_LABEL_TEXT, GO_TEXT,      15,  118, NULL,   64,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { BTN2_VALUE_TEXT, GO_TEXT,      79,  118, NULL,   152,   1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { BTN3_BORDER,     GO_RECTANGLE, 10,  145, 30,     222,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN3_MENU_TEXT,  GO_TEXT,      15,  153, NULL,   200,   1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN3_LABEL_TEXT, GO_TEXT,      15,  153, NULL,   64,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { BTN3_VALUE_TEXT, GO_TEXT,      79,  153, NULL,   152,   1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { BTN4_BORDER,     GO_RECTANGLE, 10,  180, 30,     222,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN4_MENU_TEXT,  GO_TEXT,      15,  188, NULL,   200,   1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN4_LABEL_TEXT, GO_TEXT,      15,  188, NULL,   64,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { BTN4_VALUE_TEXT, GO_TEXT,      79,  188, NULL,   152,   1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { BTN5_BORDER,     GO_RECTANGLE, 10,  215, 30,     222,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN5_MENU_TEXT,  GO_TEXT,      15,  223, NULL,   200,   1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN5_LABEL_TEXT, GO_TEXT,      15,  223, NULL,   64,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { BTN5_VALUE_TEXT, GO_TEXT,      79,  223, NULL,   152,   1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { BTN6_BORDER,     GO_RECTANGLE, 10,  250, 30,     222,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN6_MENU_TEXT,  GO_TEXT,      15,  258, NULL,   200,   1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN6_LABEL_TEXT, GO_TEXT,      15,  258, NULL,   64,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { BTN6_VALUE_TEXT, GO_TEXT,      79,  258, NULL,   152,   1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { BTN7_BORDER,     GO_RECTANGLE, 10,  285, 30,     106,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN7_MENU_TEXT,  GO_TEXT,      15,  293, NULL,   96,    1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN8_BORDER,     GO_RECTANGLE, 125, 285, 30,     107,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN8_MENU_TEXT,  GO_TEXT,      130, 293, NULL,   96,    1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { BTN9_BORDER,     GO_RECTANGLE, 125, 40,  30,     107,   1, BACK_COLOR, BUTTON_BORDER, false, NULL, NULL,        false },
    { BTN9_MENU_TEXT,  GO_TEXT,      130, 48,  NULL,   96,    1, BACK_COLOR, MENU_TEXT,     false, NULL, NULL,        false },
    { LINE0_TEXT,      GO_TEXT,      5,   80,  NULL,   232,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE1_TEXT,      GO_TEXT,      5,   100, NULL,   232,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE2_TEXT,      GO_TEXT,      5,   120, NULL,   232,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE2_COL0_TEXT, GO_TEXT,      5,   120, NULL,   104,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE2_COL1_TEXT, GO_TEXT,      110, 120, NULL,   56,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE3_TEXT,      GO_TEXT,      5,   140, NULL,   232,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE3_COL0_TEXT, GO_TEXT,      5,   140, NULL,   104,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE3_COL1_TEXT, GO_TEXT,      110, 140, NULL,   56,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE3_COL2_TEXT, GO_TEXT,      170, 140, NULL,   64,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE4_TEXT,      GO_TEXT,      5,   160, NULL,   232,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE4_COL0_TEXT, GO_TEXT,      5,   160, NULL,   104,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE4_COL1_TEXT, GO_TEXT,      110, 160, NULL,   56,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE4_COL2_TEXT, GO_TEXT,      170, 160, NULL,   64,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE5_COL0_TEXT, GO_TEXT,      5,   180, NULL,   104,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE5_COL1_TEXT, GO_TEXT,      110, 180, NULL,   56,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE5_COL2_TEXT, GO_TEXT,      170, 180, NULL,   64,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE6_COL0_TEXT, GO_TEXT,      5,   200, NULL,   104,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE6_COL1_TEXT, GO_TEXT,      110, 200, NULL,   56,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE6_COL2_TEXT, GO_TEXT,      170, 200, NULL,   64,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE7_COL0_TEXT, GO_TEXT,      5,   220, NULL,   104,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE7_COL1_TEXT, GO_TEXT,      110, 220, NULL,   56,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE7_COL2_TEXT, GO_TEXT,      170, 220, NULL,   64,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE8_COL0_TEXT, GO_TEXT,      5,   240, NULL,   104,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE8_COL1_TEXT, GO_TEXT,      110, 240, NULL,   56,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { LINE9_COL0_TEXT, GO_TEXT,      5,   260, NULL,   104,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { LINE9_COL1_TEXT, GO_TEXT,      110, 260, NULL,   56,    1, BACK_COLOR, TEXT_VALUE,    false, NULL, NULL,        false },
    { RNG_DIST_TEXT,   GO_TEXT,      63,  75,  NULL,   120,   1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { RNG_DIST_CIRCLE, GO_CIRCLE,    175, 270, 10,     10,    1, BACK_COLOR, GRAY1,         true, GRAY1, NULL,        false },
    { RNG_DS1_TEXT,    GO_TEXT,      188, 230, NULL,   40,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { RNG_DS2_TEXT,    GO_TEXT,      188, 198, NULL,   40,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { RNG_DS3_TEXT,    GO_TEXT,      188, 166, NULL,   40,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { RNG_DS4_TEXT,    GO_TEXT,      188, 134, NULL,   40,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { RNG_DS5_TEXT,    GO_TEXT,      188, 102, NULL,   40,    1, BACK_COLOR, TEXT_COLOR,    false, NULL, NULL,        false },
    { RNG_DS0_LINE,    GO_LINE,      185, 110, 160,    1,     1, BACK_COLOR, CIRCLE_FCOLOR, false, NULL, NULL,        false },
    { RNG_DS1_LINE,    GO_LINE,      183, 238, 1,      5,     1, BACK_COLOR, CIRCLE_FCOLOR, false, NULL, NULL,        false },
    { RNG_DS2_LINE,    GO_LINE,      183, 206, 1,      5,     1, BACK_COLOR, CIRCLE_FCOLOR, false, NULL, NULL,        false },
    { RNG_DS3_LINE,    GO_LINE,      183, 174, 1,      5,     1, BACK_COLOR, CIRCLE_FCOLOR, false, NULL, NULL,        false },
    { RNG_DS4_LINE,    GO_LINE,      183, 142, 1,      5,     1, BACK_COLOR, CIRCLE_FCOLOR, false, NULL, NULL,        false },
    { RNG_DS5_LINE,    GO_LINE,      183, 110, 1,      5,     1, BACK_COLOR, CIRCLE_FCOLOR, false, NULL, NULL,        false },
    { RNG_MST_SCR,     GO_RECTANGLE, 1,   70,  214,    238,   1, BACK_COLOR, BACK_COLOR,    false, NULL, NULL,        false },
};

/*!
 * \brief DrawText( ) need char* to display a text. It can display until
 * 3 different texts on the same line. To avoid recursive use of the same
 * ressource, 3 temporary string are declared.
 */
char StringText[MAX_CHAR_PER_LINE + 1];  // don't forget the /0 (end of string)
char StringText2[MAX_CHAR_PER_LINE + 1];
char StringText3[MAX_CHAR_PER_LINE + 1];

/*!
 * \brief Pointer to GPS Data, will be updated when Max7GpsgetData( ) is called.
 */
GpsStruct *thisGps;

/*!
 * \brief CurrentPage store current displayed page (cf.: list of availlable
 * pages in Menu.h). PeviousPage is used only when navigate through "Radio
 * Config" to be able to restore the page before "Radio Congig", witch can be
 * PingPong, PER or START_PAGE. The goal is to speed menu navigation.
 */
static uint8_t CurrentPage  = START_PAGE;
static uint8_t PreviousPage = START_PAGE;

/*!
 * \brief In "Radio Config Freq", we can update the central frequency. To avoid 
 * keyboard, we use (+) et (-) keys. CurrentFreqBase is the offset to be applied
 * when we increase or decrease the frequency.
 */
static FreqBase CurrentFreqBase = FB100K;


void CheckScreen( void );
void CheckRefresh( void );
void DebouncedScreen( void );
void DrawActiveButton( uint8_t buttonId, uint8_t* text1, uint8_t* text2 );
void ButtonChangeText( uint8_t buttonId, uint8_t* text1, uint8_t* text2 );
void RemoveButton( uint8_t buttonId );
void DrawText( uint8_t lineId, uint8_t* text0, uint8_t* text1, uint8_t* text2 );


void MenuInit( void )
{
    DisplayDriverInit( );
    GraphObjectDraw( &( MenuObjects[PAGE_BORDER] ), NULL, false, false );
    GraphObjectDraw( &( MenuObjects[TITLE_LINE] ), NULL, false, false );
    MenuSetPage( START_PAGE );
    CheckScreenTouch.attach_us( &CheckScreen, 100000 );     // every 100 ms

    // Page Utilities only for GPS and proximity sensor :
    CheckPageRefresh.attach_us( &CheckRefresh, 1000000 );   // every 1 s
}

uint8_t MenuHandler( bool refresh )
{
    uint8_t graphObjectTouched = 0xFF;

    if( ( CheckScreenFlag == true ) && ( Eeprom.EepromData.DemoSettings.RngStatus != RNG_PROCESS ) )
    {
        if( ScreenBeenDebounced == false )
        {
            ScreenBeenDebounced = true;
            CheckScreenFlag = false;
            DebouncedScreenTouch.attach_us( &DebouncedScreen, 150000 ); // every 150 ms
            GraphObjectTouched( MenuObjects, GRAPH_OBJECTS_COUNT, &graphObjectTouched );
        }
    }
    else
    {
        // in the Utilities page, GPS infos must be refresh each second
        if( CurrentPage == PAGE_UTILITIES )
        {
            if( PageRefresh == true )
            {
                PageRefresh = false;
                refresh = true;
            }
        }
    }

    if( refresh == true )
    {
        MenuSetPage( CurrentPage );
    }
    else if( ( graphObjectTouched != 0xFF ) || ( refresh == true ) )
    {
        switch( CurrentPage )
        {
            case START_PAGE:
                if( graphObjectTouched == BTN2_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING )
                    {
                        Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_LORA;
                    }
                    EepromLoadSettings( ( RadioPacketTypes_t )Eeprom.EepromData.DemoSettings.ModulationType );
                    MenuSetPage( PAGE_PING_PONG );
                }
                else if( graphObjectTouched == BTN3_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING )
                    {
                        Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_LORA;
                    }
                    EepromLoadSettings( ( RadioPacketTypes_t )Eeprom.EepromData.DemoSettings.ModulationType );
                    MenuSetPage( PAGE_PER );
                }
                else if( graphObjectTouched == BTN4_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    EepromLoadSettings( PACKET_TYPE_RANGING );
                    if( Eeprom.EepromData.DemoSettings.Entity == MASTER )
                    {
                        MenuSetPage( PAGE_RANGING_MASTER );
                    }
                    else
                    {
                        MenuSetPage( PAGE_RANGING_SLAVE );
                    }
                }
                else if( graphObjectTouched == BTN5_BORDER )
                {
                    EepromLoadSettings( ( RadioPacketTypes_t )Eeprom.EepromData.DemoSettings.ModulationType );
                    MenuSetPage( PAGE_RADIO_TEST );
                }    
                else if( graphObjectTouched == BTN6_BORDER )
                {
                    EepromLoadSettings( ( RadioPacketTypes_t )Eeprom.EepromData.DemoSettings.ModulationType );
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    MenuSetPage( PAGE_UTILITIES );
                }
                break;

            case PAGE_PING_PONG:
                if( graphObjectTouched == BTN0_BORDER )
                {
                    ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                    if( Eeprom.EepromData.DemoSettings.Entity == MASTER )
                    {
                        Eeprom.EepromData.DemoSettings.Entity = SLAVE;
                        ButtonChangeText( BTN0_BORDER, ( uint8_t* )"   SLAVE", NULL );
                        DrawText( LINE3_COL0_TEXT, NULL, NULL, ( uint8_t* )" " );
                        DrawText( LINE4_COL0_TEXT, NULL, NULL, ( uint8_t* )" " );
                        DrawText( LINE5_COL0_TEXT, NULL, NULL, ( uint8_t* )" " );
                        DrawText( LINE6_COL0_TEXT, NULL, NULL, ( uint8_t* )" " );
                        DrawText( LINE7_COL0_TEXT, NULL, NULL, ( uint8_t* )" " );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.Entity = MASTER;
                        ButtonChangeText( BTN0_BORDER, ( uint8_t* )"   MASTER", NULL );
                        DrawText( LINE3_COL0_TEXT, NULL, NULL, ( uint8_t* )"MASTER" );
                    }
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    EepromSaveSettings( DEMO_SETTINGS );
                    MenuSetPage( PAGE_PING_PONG );
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    StopDemoApplication( );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        Eeprom.EepromData.DemoSettings.CntPacketTx          = 0;
                        Eeprom.EepromData.DemoSettings.CntPacketRxOK        = 0;
                        Eeprom.EepromData.DemoSettings.CntPacketRxOKSlave   = 0;
                        Eeprom.EepromData.DemoSettings.CntPacketRxKO        = 0;
                        Eeprom.EepromData.DemoSettings.CntPacketRxKOSlave   = 0;
                        Eeprom.EepromData.DemoSettings.RxTimeOutCount       = 0;
                        StopDemoApplication( );
                        RunDemoApplicationPingPong( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"    STOP", NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        StopDemoApplication( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_PING_PONG;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( refresh == true )
                {
                    MenuSetPage( PAGE_PING_PONG );
                }
                break;

            case PAGE_PER:
                if( graphObjectTouched == BTN0_BORDER )
                {
                    ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                    if( Eeprom.EepromData.DemoSettings.Entity == MASTER )
                    {
                        Eeprom.EepromData.DemoSettings.Entity = SLAVE;
                        ButtonChangeText( BTN0_BORDER, ( uint8_t* )"   SLAVE", NULL );
                        DrawText( LINE4_COL0_TEXT, ( uint8_t* )"Rx OK", NULL, NULL );
                        DrawText( LINE5_COL0_TEXT, ( uint8_t* )"Rx KO", NULL, NULL );
                        DrawText( LINE6_COL0_TEXT, ( uint8_t* )"PER",   NULL, NULL );
                        DrawText( LINE8_COL0_TEXT, ( uint8_t* )"Last Rssi", NULL, NULL );
                        DrawText( LINE9_COL0_TEXT, ( uint8_t* )"Last SNR", NULL, NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.Entity = MASTER;
                        ButtonChangeText( BTN0_BORDER, ( uint8_t* )"   MASTER", NULL );
                        DrawText( LINE4_COL0_TEXT, ( uint8_t* )"Transmitted", ( uint8_t* )GetMenuDemoNumSentPacket( ), NULL );
                        DrawText( LINE5_COL0_TEXT, ( uint8_t* )" ", ( uint8_t* )" ", NULL );
                        DrawText( LINE6_COL0_TEXT, ( uint8_t* )" ", ( uint8_t* )" ", NULL );
                        DrawText( LINE8_COL0_TEXT, ( uint8_t* )" ", ( uint8_t* )" ", NULL );
                        DrawText( LINE9_COL0_TEXT, ( uint8_t* )" ", ( uint8_t* )" ", NULL );
                    }
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    EepromSaveSettings( DEMO_SETTINGS );
                    MenuSetPage( PAGE_PER );
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        StopDemoApplication( );
                        Eeprom.EepromData.DemoSettings.CntPacketTx          = 0;
                        Eeprom.EepromData.DemoSettings.CntPacketRxOK        = 0;
                        Eeprom.EepromData.DemoSettings.CntPacketRxKO        = 0;
                        Eeprom.EepromData.DemoSettings.RxTimeOutCount       = 0;
                        RunDemoApplicationPer( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"    STOP", NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        StopDemoApplication( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_PER;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( refresh == true )
                {
                    MenuSetPage( PAGE_PER );
                }
                break;

            case PAGE_RANGING_MASTER:
                if( graphObjectTouched == BTN0_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.Entity = SLAVE;
                    ButtonChangeText( BTN0_BORDER, ( uint8_t* )"   SLAVE", NULL );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    StopDemoApplication( );
                    EepromSaveSettings( DEMO_SETTINGS );
                    MenuSetPage( PAGE_RANGING_SLAVE );
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    StopDemoApplication( );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"  PROCESS..", NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"  REFRESH", NULL );
                        StopDemoApplication( );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "SETTINGS" button can occure without "START"/"STOP" button
                    // also "SETTINGS" should stop the demo
                    StopDemoApplication( );
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_RANGING_MASTER;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == RNG_MST_SCR )
                {
                    // screen touched .. launch new measure
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"  PROCESS..", NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"  REFRESH", NULL );
                        StopDemoApplication( );
                    }
                }
                else if( refresh == true )
                {
                    MenuSetPage( PAGE_RANGING_MASTER );
                }
                break;

            case PAGE_RANGING_SLAVE:
                if( graphObjectTouched == BTN0_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.Entity = MASTER;
                    ButtonChangeText( BTN0_BORDER, ( uint8_t* )"   MASTER", NULL );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    StopDemoApplication( );
                    EepromSaveSettings( DEMO_SETTINGS );
                    MenuSetPage( PAGE_RANGING_MASTER );
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    StopDemoApplication( );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"    STOP", NULL );
                        StopDemoApplication( );
                        RunDemoApplicationRanging( );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                        StopDemoApplication( );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "SETTINGS" button can occure without "START"/"STOP" button
                    // also "SETTINGS" should stop the demo
                    StopDemoApplication( );
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_RANGING_SLAVE;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( refresh == true )
                {
                    MenuSetPage( PAGE_RANGING_SLAVE );
                }
                break;

            case PAGE_RADIO_PARAM:
                if( graphObjectTouched == BTN0_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA )
                    {
                        Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_FLRC;
                    }
                    else if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_FLRC )
                    {
                        Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_GFSK;
                    }
                    else if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_GFSK )
                    {
                        if( PreviousPage == START_PAGE )
                        {
                            Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_RANGING;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_LORA;
                        }
                    }
                    else    // PACKET_TYPE_RANGING
                    {
                        if( PreviousPage == START_PAGE )
                        {
                            Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_LORA;
                        }
                    }
                    EepromLoadSettings( ( RadioPacketTypes_t )Eeprom.EepromData.DemoSettings.ModulationType );
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    EepromSetRangingDefaultSettings( );
                    EepromLoadSettings( PACKET_TYPE_RANGING );
                    ButtonChangeText( BTN3_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters1( ) );
                    ButtonChangeText( BTN4_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters2( ) );
                    ButtonChangeText( BTN5_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters3( ) );
                }
                else if( graphObjectTouched == BTN1_BORDER )
                {
                    MenuSetPage( PAGE_RADIO_PARAM_FREQ );
                }
                else if( graphObjectTouched == BTN2_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.TxPower += 1;
                    if( Eeprom.EepromData.DemoSettings.TxPower > DEMO_POWER_TX_MAX )
                    {
                        Eeprom.EepromData.DemoSettings.TxPower = DEMO_POWER_TX_MIN;
                    }
                    ButtonChangeText( BTN2_BORDER, NULL, ( uint8_t* )GetRadioTxPower( ) );
                }
                else if( graphObjectTouched == BTN3_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA )
                    {
                        if( Eeprom.EepromData.DemoSettings.ModulationParam1 < LORA_SF12 )
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam1 = Eeprom.EepromData.DemoSettings.ModulationParam1 + 0x10;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam1 = LORA_SF5;
                        }
                    }
                    else if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING )
                    {
                        if( Eeprom.EepromData.DemoSettings.ModulationParam1 < LORA_SF10 )
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam1 = Eeprom.EepromData.DemoSettings.ModulationParam1 + 0x10;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam1 = LORA_SF5;
                        }
                    }
                    else if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_FLRC )
                    {
                        switch( Eeprom.EepromData.DemoSettings.ModulationParam1 )
                        {
                            case FLRC_BR_1_300_BW_1_2:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = FLRC_BR_1_040_BW_1_2;
                                break;

                            case FLRC_BR_1_040_BW_1_2:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = FLRC_BR_0_650_BW_0_6;
                                break;

                            case FLRC_BR_0_650_BW_0_6:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = FLRC_BR_0_520_BW_0_6;
                                break;

                            case FLRC_BR_0_520_BW_0_6:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = FLRC_BR_0_325_BW_0_3;
                                break;

                            case FLRC_BR_0_325_BW_0_3:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = FLRC_BR_0_260_BW_0_3;
                                break;

                            case FLRC_BR_0_260_BW_0_3:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = FLRC_BR_1_300_BW_1_2;
                                break;
                        }
                    }
                    else    // PACKET_TYPE_GFSK
                    {
                        switch( Eeprom.EepromData.DemoSettings.ModulationParam1 )
                        {
                            case GFSK_BLE_BR_2_000_BW_2_4:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_1_600_BW_2_4;
                                break;

                            case GFSK_BLE_BR_1_600_BW_2_4:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_1_000_BW_2_4;
                                break;

                            case GFSK_BLE_BR_1_000_BW_2_4:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_1_000_BW_1_2;
                                break;

                            case GFSK_BLE_BR_1_000_BW_1_2:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_800_BW_2_4;
                                break;

                            case GFSK_BLE_BR_0_800_BW_2_4:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_800_BW_1_2;
                                break;

                            case GFSK_BLE_BR_0_800_BW_1_2:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_500_BW_1_2;
                                break;

                            case GFSK_BLE_BR_0_500_BW_1_2:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_500_BW_0_6;
                                break;

                            case GFSK_BLE_BR_0_500_BW_0_6:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_400_BW_1_2;
                                break;

                            case GFSK_BLE_BR_0_400_BW_1_2:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_400_BW_0_6;
                                break;

                            case GFSK_BLE_BR_0_400_BW_0_6:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_250_BW_0_6;
                                break;

                            case GFSK_BLE_BR_0_250_BW_0_6:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_250_BW_0_3;
                                break;

                            case GFSK_BLE_BR_0_250_BW_0_3:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_125_BW_0_3;
                                break;

                            case GFSK_BLE_BR_0_125_BW_0_3:
                                Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_2_000_BW_2_4;
                                break;
                        }
                    }
                    ButtonChangeText( BTN3_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters1( ) );
                }
                else if( graphObjectTouched == BTN4_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA )
                    {
                        switch( Eeprom.EepromData.DemoSettings.ModulationParam2 )
                        {
                            case LORA_BW_0200:
                                Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_0400;
                                break;

                            case LORA_BW_0400:
                                Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_0800;
                                break;

                            case LORA_BW_0800:
                                Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_1600;
                                break;

                            case LORA_BW_1600:
                                Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_0200;
                                break;
                        }
                    }
                    else if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING )
                    {
                        switch( Eeprom.EepromData.DemoSettings.ModulationParam2 )
                        {
                            case LORA_BW_0400:
                                Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_0800;
                                break;

                            case LORA_BW_0800:
                                Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_1600;
                                break;

                            case LORA_BW_1600:
                                Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_0400;
                                break;
                        }
                    }
                    else if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_FLRC )
                    {
                        if( Eeprom.EepromData.DemoSettings.ModulationParam2 < FLRC_CR_1_0 )
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam2 = Eeprom.EepromData.DemoSettings.ModulationParam2 + 2;;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam2 = FLRC_CR_1_2;
                        }
                    }
                    else    // PACKET_TYPE_GFSK
                    {
                        if( Eeprom.EepromData.DemoSettings.ModulationParam2 < GFSK_BLE_MOD_IND_4_00 )
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam2++;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam2 = GFSK_BLE_MOD_IND_0_35;
                        }
                    }
                    ButtonChangeText( BTN4_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters2( ) );
                }
                else if( graphObjectTouched == BTN5_BORDER )
                {
                    if( ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA ) || \
                        ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING ) )
                    {
                        if( Eeprom.EepromData.DemoSettings.ModulationParam3 < LORA_CR_LI_4_7 )
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam3++;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam3 = LORA_CR_4_5;
                        }
                    }
                    else // PACKET_TYPE_GFSK and PACKET_TYPE_FLRC
                    {
                        if( Eeprom.EepromData.DemoSettings.ModulationParam3 < RADIO_MOD_SHAPING_BT_0_5 )
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam3 = Eeprom.EepromData.DemoSettings.ModulationParam3 + 0x10;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.ModulationParam3 = RADIO_MOD_SHAPING_BT_OFF;
                        }
                    }
                    ButtonChangeText( BTN5_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters3( ) );
                }
                else if( graphObjectTouched == BTN6_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA )
                    {
                        if( Eeprom.EepromData.DemoSettings.PacketParam3 < 32 )
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam3++;
                        }
                        else if( Eeprom.EepromData.DemoSettings.PacketParam3 <= ( DEMO_GFS_LORA_MAX_PAYLOAD - 8 ) )
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam3 += 8;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam3 = DEMO_MIN_PAYLOAD;
                        }
                    }
                    else if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_FLRC )
                    {
                        if( Eeprom.EepromData.DemoSettings.PacketParam5 < 32 )
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam5++;
                        }
                        else if( Eeprom.EepromData.DemoSettings.PacketParam5 <= ( DEMO_FLRC_MAX_PAYLOAD - 8 ) )
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam5 += 8;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam5 = DEMO_MIN_PAYLOAD;
                        }
                    }
                    else if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_GFSK )
                    {
                        if( Eeprom.EepromData.DemoSettings.PacketParam5 < 32 )
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam5++;
                        }
                        else if( Eeprom.EepromData.DemoSettings.PacketParam5 <= ( DEMO_GFS_LORA_MAX_PAYLOAD - 8 ) )
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam5 += 8;
                        }
                        else
                        {
                            Eeprom.EepromData.DemoSettings.PacketParam5 = DEMO_MIN_PAYLOAD;
                        }
                    }
                    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING )
                    {
                        MenuSetPage( PAGE_RANGING_PARAM );
                    }
                    else
                    {
                        ButtonChangeText( BTN6_BORDER, NULL, ( uint8_t* )GetRadioPayloadLength( ) );
                    }
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    uint8_t modTmp = Eeprom.EepromData.DemoSettings.ModulationType;

                    EepromMcuReadBuffer( 0, Eeprom.Buffer, EEPROM_BUFFER_SIZE );
                    EepromLoadGeneralSettings( );
                    EepromLoadSettings( ( RadioPacketTypes_t )modTmp );
                    MenuSetPage( PreviousPage );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    switch( Eeprom.EepromData.DemoSettings.ModulationType )
                    {
                        case PACKET_TYPE_RANGING:
                            EepromSaveSettings( RADIO_RANGING_PARAMS );
                            break;

                        case PACKET_TYPE_LORA:
                            EepromSaveSettings( RADIO_LORA_PARAMS );
                            break;

                        case PACKET_TYPE_FLRC:
                            EepromSaveSettings( RADIO_FLRC_PARAMS );
                            break;

                        case PACKET_TYPE_GFSK:
                            EepromSaveSettings( RADIO_GFSK_PARAMS );
                            break;
                    }
                    EepromSaveSettings( DEMO_SETTINGS );
                    MenuSetPage( PreviousPage );
                }
                break;

            case PAGE_RANGING_PARAM:
                if( graphObjectTouched == BTN1_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.RngRequestCount += 5;
                    if( Eeprom.EepromData.DemoSettings.RngRequestCount >= DEMO_RNG_CHANNELS_COUNT_MAX )
                    {
                        Eeprom.EepromData.DemoSettings.RngRequestCount = DEMO_RNG_CHANNELS_COUNT_MIN;
                    }
                    ButtonChangeText( BTN1_BORDER, NULL, ( uint8_t* )GetRangingRequestCount( ) );
                }
                else if( graphObjectTouched == BTN2_BORDER )
                {
                    switch( Eeprom.EepromData.DemoSettings.RngAddress )
                    {
                        case DEMO_RNG_ADDR_1:
                            Eeprom.EepromData.DemoSettings.RngAddress = DEMO_RNG_ADDR_2;
                            break;

                        case DEMO_RNG_ADDR_2:
                            Eeprom.EepromData.DemoSettings.RngAddress = DEMO_RNG_ADDR_3;
                            break;

                        case DEMO_RNG_ADDR_3:
                            Eeprom.EepromData.DemoSettings.RngAddress = DEMO_RNG_ADDR_4;
                            break;

                        case DEMO_RNG_ADDR_4:
                            Eeprom.EepromData.DemoSettings.RngAddress = DEMO_RNG_ADDR_5;
                            break;

                        case DEMO_RNG_ADDR_5:
                            Eeprom.EepromData.DemoSettings.RngAddress = DEMO_RNG_ADDR_1;
                            break;
                    }
                    ButtonChangeText( BTN2_BORDER, NULL, ( uint8_t* )GetRangingAddress( ) );
                }
                else if( graphObjectTouched == BTN3_BORDER )
                {
                    switch( Eeprom.EepromData.DemoSettings.RngAntenna )
                    {
                        case DEMO_RNG_ANT_1:
                            Eeprom.EepromData.DemoSettings.RngAntenna = DEMO_RNG_ANT_2;
                            break;

                        case DEMO_RNG_ANT_2:
                            Eeprom.EepromData.DemoSettings.RngAntenna = DEMO_RNG_ANT_BOTH;
                            break;

                        case DEMO_RNG_ANT_BOTH:
                            Eeprom.EepromData.DemoSettings.RngAntenna = DEMO_RNG_ANT_1;
                            break;
                    }
                    ButtonChangeText( BTN3_BORDER, NULL, ( uint8_t* )GetRangingAntenna( ) );
                }
                else if( graphObjectTouched == BTN4_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.RngUnit++;
                    if( Eeprom.EepromData.DemoSettings.RngUnit > DEMO_RNG_UNIT_SEL_MI )
                    {
                        Eeprom.EepromData.DemoSettings.RngUnit = DEMO_RNG_UNIT_SEL_M;
                    }
                    ButtonChangeText( BTN4_BORDER, NULL, ( uint8_t* )GetRangingUnit( ) );
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    EepromMcuReadBuffer( 0, Eeprom.Buffer, EEPROM_BUFFER_SIZE );
                    EepromLoadGeneralSettings( );
                    EepromLoadSettings( PACKET_TYPE_RANGING );
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    EepromSaveSettings( RADIO_RANGING_PARAMS );
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                break;

            case PAGE_RADIO_PARAM_FREQ:
                if( graphObjectTouched == BTN0_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.Frequency -= CurrentFreqBase;
                    if( Eeprom.EepromData.DemoSettings.Frequency < DEMO_CENTRAL_FREQ_MIN )
                    {
                        Eeprom.EepromData.DemoSettings.Frequency = DEMO_CENTRAL_FREQ_MIN;
                    }
                    ButtonChangeText( BTN2_BORDER, NULL, ( uint8_t* )GetRadioFrequency( ) );
                }
                else if( graphObjectTouched == BTN1_BORDER )
                {
                    switch( CurrentFreqBase )
                    {
                        case FB1:
                            CurrentFreqBase = FB10;
                            break;

                        case FB10:
                            CurrentFreqBase = FB100;
                            break;

                        case FB100:
                            CurrentFreqBase = FB1K;
                            break;

                        case FB1K:
                            CurrentFreqBase = FB10K;
                            break;

                        case FB10K:
                            CurrentFreqBase = FB100K;
                            break;

                        case FB100K:
                            CurrentFreqBase = FB1M;
                            break;

                        case FB1M:
                            CurrentFreqBase = FB10M;
                            break;

                        case FB10M:
                            CurrentFreqBase = FB1;
                            break;
                    }
                    ButtonChangeText( BTN1_BORDER, NULL, ( uint8_t* )GetRadioFreqBase( ) );
                }
                else if( graphObjectTouched == BTN3_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.Frequency = DEMO_CENTRAL_FREQ_PRESET1;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == BTN4_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.Frequency = DEMO_CENTRAL_FREQ_PRESET2;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == BTN5_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.Frequency = DEMO_CENTRAL_FREQ_PRESET3;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    memcpy( &( Eeprom.EepromData.DemoSettings.Frequency ), Eeprom.Buffer + APP_FREQ_EEPROM_ADDR, 4 );
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.Frequency += CurrentFreqBase;
                    if( Eeprom.EepromData.DemoSettings.Frequency > DEMO_CENTRAL_FREQ_MAX )
                    {
                        Eeprom.EepromData.DemoSettings.Frequency = DEMO_CENTRAL_FREQ_MAX;
                    }
                    ButtonChangeText( BTN2_BORDER, NULL, ( uint8_t* )GetRadioFrequency( ) );
                }
                break;

            case PAGE_UTILITIES:
                if( graphObjectTouched == BTN0_BORDER )
                {
                    FactoryReset( );
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN4_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.RadioPowerMode == USE_LDO )
                    {
                        Eeprom.EepromData.DemoSettings.RadioPowerMode = USE_DCDC;
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.RadioPowerMode = USE_LDO;
                    }
                    ButtonChangeText( BTN4_BORDER, NULL, ( uint8_t* )GetMenuDemoRadioPowerMode( ) );
                }
                else if( graphObjectTouched == BTN5_BORDER )
                {
                    switch( Eeprom.EepromData.DemoSettings.MaxNumPacket )
                    {
                        case 0:
                            Eeprom.EepromData.DemoSettings.MaxNumPacket = 100;
                            break;

                        case 100:
                            Eeprom.EepromData.DemoSettings.MaxNumPacket = 200;
                            break;

                        case 200:
                            Eeprom.EepromData.DemoSettings.MaxNumPacket = 500;
                            break;

                        case 500:
                            Eeprom.EepromData.DemoSettings.MaxNumPacket = 1000;
                            break;

                        case 1000:
                            Eeprom.EepromData.DemoSettings.MaxNumPacket = 0;
                            break;
                    }
                    ButtonChangeText( BTN5_BORDER, NULL, ( uint8_t* )GetMenuDemoMaxNumPacket( ) );
                }
                else if( graphObjectTouched == BTN6_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.AntennaSwitch == 0 )
                    {
                        Eeprom.EepromData.DemoSettings.AntennaSwitch = 1;
                        ButtonChangeText( BTN6_BORDER, NULL, ( uint8_t* )"ANT2" );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.AntennaSwitch = 0;
                        ButtonChangeText( BTN6_BORDER, NULL, ( uint8_t* )"ANT1" );
                    }
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    uint8_t modTmp = Eeprom.EepromData.DemoSettings.ModulationType;

                    EepromMcuReadBuffer( 0, Eeprom.Buffer, EEPROM_BUFFER_SIZE );
                    EepromLoadGeneralSettings( );
                    EepromLoadSettings( ( RadioPacketTypes_t )modTmp );
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    EepromSaveSettings( DEMO_SETTINGS );
                    MenuSetPage( START_PAGE );
                }
                else if( refresh == true )
                {
                    MenuSetPage( PAGE_UTILITIES );
                }
                break;

            case PAGE_RADIO_TEST:
                if( graphObjectTouched == BTN1_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    MenuSetPage( PAGE_SLEEP_MODE );
                }
                else if( graphObjectTouched == BTN2_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    MenuSetPage( PAGE_STBY_RC_MODE );
                }
                else if( graphObjectTouched == BTN3_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    MenuSetPage( PAGE_STBY_XOSC_MODE );
                }
                else if( graphObjectTouched == BTN4_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    MenuSetPage( PAGE_TX_CW );
                }
                else if( graphObjectTouched == BTN5_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    MenuSetPage( PAGE_CONT_MODULATION );
                }
                else if( graphObjectTouched == BTN7_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    MenuSetPage( START_PAGE );
                }
                break;

            case PAGE_SLEEP_MODE:
                if( graphObjectTouched == BTN7_BORDER )
                {
                    StopDemoApplication( );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        StopDemoApplication( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"    STOP", NULL );
                        RunDemoSleepMode( );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        StopDemoApplication( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                        MenuSetPage( PAGE_SLEEP_MODE );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_SLEEP_MODE;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                break;

            case PAGE_STBY_RC_MODE:
                if( graphObjectTouched == BTN7_BORDER )
                {
                    StopDemoApplication( );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        StopDemoApplication( );
                        RunDemoStandbyRcMode( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"    STOP", NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        StopDemoApplication( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_STBY_RC_MODE;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                break;

            case PAGE_STBY_XOSC_MODE:
                if( graphObjectTouched == BTN7_BORDER )
                {
                    StopDemoApplication( );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        StopDemoApplication( );
                        RunDemoStandbyXoscMode( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"    STOP", NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        StopDemoApplication( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_STBY_XOSC_MODE;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                break;

            case PAGE_TX_CW:
                if( graphObjectTouched == BTN7_BORDER )
                {
                    StopDemoApplication( );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        StopDemoApplication( );
                        RunDemoTxCw( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"    STOP", NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        StopDemoApplication( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_TX_CW;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                break;

            case PAGE_CONT_MODULATION:
                if( graphObjectTouched == BTN7_BORDER )
                {
                    StopDemoApplication( );
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    PreviousPage = START_PAGE; // clear Previous page
                    MenuSetPage( START_PAGE );
                }
                else if( graphObjectTouched == BTN8_BORDER )
                {
                    if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = false;
                        StopDemoApplication( );
                        RunDemoTxContinuousModulation( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"    STOP", NULL );
                    }
                    else
                    {
                        Eeprom.EepromData.DemoSettings.HoldDemo = true;
                        StopDemoApplication( );
                        ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                    }
                }
                else if( graphObjectTouched == BTN9_BORDER )
                {
                    Eeprom.EepromData.DemoSettings.HoldDemo = true;
                    // "CANCEL" or "OK & SAVE" button of PAGE_RADIO_PARAM use
                    // PreviousPage to set next page. Speed up the navigation.
                    PreviousPage = PAGE_CONT_MODULATION;
                    MenuSetPage( PAGE_RADIO_PARAM );
                }
                break;
        }
    }
    return CurrentPage;
}

void MenuSetPage( uint8_t page )
{
    if( page != CurrentPage )
    {
        // remove all button and text (except title, page border and line)
        // and disable all button (touchActive = false).
        GraphObject_t clearRect = { NULL, GO_RECTANGLE, 3, 35, 282, 230, 1, \
                                    BACK_COLOR, BACK_COLOR, true, BACK_COLOR, \
                                    NULL, false };

        GraphObjectClear( &clearRect, true );
        for( uint8_t i = 0; i < GRAPH_OBJECTS_COUNT; i++ )
        {
            MenuObjects[i].TouchActive = false;
        }
    }

    switch( page )
    {
        case START_PAGE:
            DrawText( TITLE_TEXT, ( uint8_t* )"      SX1280 Demo Kit", NULL, NULL );
            DisplayDriverDrawLogo( SemtechLogo, 20, 37 );
            DrawActiveButton( BTN2_BORDER, ( uint8_t* )"DEMO Ping Pong", NULL );
            DrawActiveButton( BTN3_BORDER, ( uint8_t* )"DEMO PER", NULL );
            DrawActiveButton( BTN4_BORDER, ( uint8_t* )"Outdoor ranging demo", NULL );
            DrawActiveButton( BTN5_BORDER, ( uint8_t* )"Radio Test Modes", NULL );
            DrawActiveButton( BTN6_BORDER, ( uint8_t* )"Radio Settings", NULL );
            DrawActiveButton( BTN7_BORDER, ( uint8_t* )"Utilities", NULL );
            CurrentPage = START_PAGE;
            break;

        case PAGE_PING_PONG:
            if( CurrentPage == PAGE_PING_PONG )
            {
                if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                {
                    ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                }
                if( Eeprom.EepromData.DemoSettings.Entity == MASTER )
                {
                    DrawText( LINE4_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRxOkSlave( ), ( uint8_t* )GetMenuDemoRxOk( ) );
                    DrawText( LINE5_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRxKoSlave( ), ( uint8_t* )GetMenuDemoRxKo( ) );
                    DrawText( LINE6_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoResultPerCent1( Eeprom.EepromData.DemoSettings.CntPacketRxOKSlave, Eeprom.EepromData.DemoSettings.CntPacketRxOKSlave + Eeprom.EepromData.DemoSettings.CntPacketRxKOSlave ), \
                              ( uint8_t* )GetMenuDemoResultPerCent2( Eeprom.EepromData.DemoSettings.CntPacketRxOK, Eeprom.EepromData.DemoSettings.CntPacketRxOK + Eeprom.EepromData.DemoSettings.CntPacketRxKO + Eeprom.EepromData.DemoSettings.RxTimeOutCount ) );
                    DrawText( LINE7_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoResultPerCent1( Eeprom.EepromData.DemoSettings.CntPacketRxKOSlave, Eeprom.EepromData.DemoSettings.CntPacketRxOKSlave + Eeprom.EepromData.DemoSettings.CntPacketRxKOSlave ), \
                              ( uint8_t* )GetMenuDemoResultPerCent2( Eeprom.EepromData.DemoSettings.CntPacketRxKO + Eeprom.EepromData.DemoSettings.RxTimeOutCount, Eeprom.EepromData.DemoSettings.CntPacketRxOK + Eeprom.EepromData.DemoSettings.CntPacketRxKO + Eeprom.EepromData.DemoSettings.RxTimeOutCount ) );
                    DrawText( LINE8_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRssi( ), NULL );
                    DrawText( LINE9_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoSnr( ), NULL );
                }
                else
                {
                    DrawText( LINE4_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRxOk( ), NULL );
                    DrawText( LINE5_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRxKo( ), NULL );
                    DrawText( LINE6_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoResultPerCent1( Eeprom.EepromData.DemoSettings.CntPacketRxOK, Eeprom.EepromData.DemoSettings.CntPacketRxOK + Eeprom.EepromData.DemoSettings.CntPacketRxKO + Eeprom.EepromData.DemoSettings.RxTimeOutCount ), NULL );
                    DrawText( LINE7_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoResultPerCent1( Eeprom.EepromData.DemoSettings.CntPacketRxKO + Eeprom.EepromData.DemoSettings.RxTimeOutCount, Eeprom.EepromData.DemoSettings.CntPacketRxOK + Eeprom.EepromData.DemoSettings.CntPacketRxKO + Eeprom.EepromData.DemoSettings.RxTimeOutCount ), NULL );
                    DrawText( LINE8_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRssi( ), NULL );
                    DrawText( LINE9_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoSnr( ), NULL );
                }
            }
            else
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"      PING PONG Demo", NULL, NULL );
                DisplayCurrentRadioParams( PAGE_PING_PONG );
                DrawText( LINE3_COL0_TEXT, ( uint8_t* )GetTotalPackets( ), ( uint8_t* )" SLAVE", NULL );
                DrawText( LINE4_COL0_TEXT, ( uint8_t* )"Rx OK", ( uint8_t* )"     0", NULL );
                DrawText( LINE5_COL0_TEXT, ( uint8_t* )"Rx KO", ( uint8_t* )"     0", NULL );
                DrawText( LINE6_COL0_TEXT, ( uint8_t* )"Rx PSR", ( uint8_t* )"  0.00", NULL );
                DrawText( LINE7_COL0_TEXT, ( uint8_t* )"Rx PER", ( uint8_t* )"  0.00", NULL );
                DrawText( LINE8_COL0_TEXT, ( uint8_t* )"Last Rssi", ( uint8_t* )"     0", NULL );
                DrawText( LINE9_COL0_TEXT, ( uint8_t* )"Last SNR", ( uint8_t* )"     0", NULL );
                if( Eeprom.EepromData.DemoSettings.Entity == MASTER )
                {
                    DrawActiveButton( BTN0_BORDER, ( uint8_t* )"   MASTER", NULL );
                    DrawText( LINE3_COL0_TEXT, NULL, NULL, ( uint8_t* )"MASTER" );
                    DrawText( LINE4_COL0_TEXT, NULL, NULL, ( uint8_t* )"     0" );
                    DrawText( LINE5_COL0_TEXT, NULL, NULL, ( uint8_t* )"     0" );
                    DrawText( LINE6_COL0_TEXT, NULL, NULL, ( uint8_t* )"  0.00" );
                    DrawText( LINE7_COL0_TEXT, NULL, NULL, ( uint8_t* )"  0.00" );
                }
                else
                {
                    DrawActiveButton( BTN0_BORDER, ( uint8_t* )"   SLAVE", NULL );
                }
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
            }
            CurrentPage = PAGE_PING_PONG;
            break;

        case PAGE_PER:
            if( CurrentPage == PAGE_PER )
            {
                if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                {
                    ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                }
                if( Eeprom.EepromData.DemoSettings.Entity == MASTER )
                {
                    DrawText( LINE4_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoNumSentPacket( ), NULL );
                }
                else
                {
                    DrawText( LINE4_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRxOk( ), NULL );
                    DrawText( LINE5_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRxKo( ), NULL );
                    DrawText( LINE6_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoResultPerCent1( Eeprom.EepromData.DemoSettings.CntPacketRxKO + Eeprom.EepromData.DemoSettings.RxTimeOutCount, Eeprom.EepromData.DemoSettings.CntPacketRxOK + Eeprom.EepromData.DemoSettings.CntPacketRxKO + Eeprom.EepromData.DemoSettings.RxTimeOutCount ), NULL );
                    DrawText( LINE8_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoRssi( ), NULL );
                    DrawText( LINE9_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoSnr( ), NULL );
                }
            }
            else
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"         PER Demo", NULL, NULL );
                DrawText( LINE3_COL0_TEXT, ( uint8_t* )"Test Total", ( uint8_t* )GetMenuDemoMaxNumPacket( ), NULL );
                DisplayCurrentRadioParams( PAGE_PER );
                if( Eeprom.EepromData.DemoSettings.Entity == MASTER )
                {
                    DrawActiveButton( BTN0_BORDER, ( uint8_t* )"   MASTER", NULL );
                    DrawText( LINE4_COL0_TEXT, ( uint8_t* )"Transmitted", ( uint8_t* )GetMenuDemoNumSentPacket( ), NULL );
                }
                else
                {
                    DrawActiveButton( BTN0_BORDER, ( uint8_t* )"   SLAVE", NULL );
                    DrawText( LINE4_COL0_TEXT, ( uint8_t* )"Rx OK", ( uint8_t* )"     0", NULL );
                    DrawText( LINE5_COL0_TEXT, ( uint8_t* )"Rx KO", ( uint8_t* )"     0", NULL );
                    DrawText( LINE6_COL0_TEXT, ( uint8_t* )"PER",   ( uint8_t* )"  0.00", NULL );
                    DrawText( LINE8_COL0_TEXT, ( uint8_t* )"Last Rssi", ( uint8_t* )"     0", NULL );
                    DrawText( LINE9_COL0_TEXT, ( uint8_t* )"Last SNR", ( uint8_t* )"     0", NULL );
                }
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
            }
            CurrentPage = PAGE_PER;
            break;

        case PAGE_RANGING_MASTER:
            if( CurrentPage == PAGE_RANGING_MASTER )
            {
                if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                {
                    ButtonChangeText( BTN8_BORDER, ( uint8_t* )"  REFRESH", NULL );
                }
                // Clear actual distance token
                GraphObjectClear( &( MenuObjects[RNG_DIST_CIRCLE] ), true );
                // update scale of circles
                for( uint8_t i = SCALE_LINE_COUNT; i > 0; i-- )
                {
                    sprintf( StringText, "%d", i * ( Eeprom.EepromData.DemoSettings.RngFullScale / SCALE_LINE_COUNT ) );
                    DrawText( RNG_DS1_TEXT + i - 1, ( uint8_t* )" ", NULL, NULL );
                    DrawText( RNG_DS1_TEXT + i - 1, ( uint8_t* )StringText, NULL, NULL );
                }
                uint16_t DistPixel;
                // Compute position (pixel) of token
                // distance line high is 160px : it's the reference for distance graphical objects (represent full scale)
                DistPixel = ( uint16_t )( Eeprom.EepromData.DemoSettings.RngDistance * 160.0 / Eeprom.EepromData.DemoSettings.RngFullScale );
                MenuObjects[RNG_DIST_CIRCLE].FrontColor = WHITE;
                // Need to change full scale ? yes if dist > fullscale or if dist < fullscale / SCALE_LINE_COUNT
                // fullscale will be updated next time
                if( DistPixel > 160 )
                {
                    DistPixel = 160;
                    MenuObjects[RNG_DIST_CIRCLE].FrontColor = OBJECT_ERROR;
                    Eeprom.EepromData.DemoSettings.RngFullScale = ( uint16_t )ceil( 1.2 * ( Eeprom.EepromData.DemoSettings.RngDistance / SCALE_LINE_COUNT ) ) * SCALE_LINE_COUNT;
                    if( Eeprom.EepromData.DemoSettings.RngFullScale > RANGING_FULLSCALE_MAX )
                    {
                        Eeprom.EepromData.DemoSettings.RngFullScale = RANGING_FULLSCALE_MAX;
                    }
                }
                else if( DistPixel < ( 160 / SCALE_LINE_COUNT ) )
                {
                    Eeprom.EepromData.DemoSettings.RngFullScale = ( uint16_t )ceil( 1.2 * ( Eeprom.EepromData.DemoSettings.RngDistance / SCALE_LINE_COUNT ) ) * SCALE_LINE_COUNT;
                    if( Eeprom.EepromData.DemoSettings.RngFullScale < RANGING_FULLSCALE_MIN )
                    {
                        Eeprom.EepromData.DemoSettings.RngFullScale = RANGING_FULLSCALE_MIN;
                    }
                }
                // Check timeout error
                if( Eeprom.EepromData.DemoSettings.RngStatus != RNG_VALID )
                {
                    MenuObjects[RNG_DIST_CIRCLE].FrontColor = OBJECT_ERROR;
                }
                switch( Eeprom.EepromData.DemoSettings.RngUnit )
                {
                    case DEMO_RNG_UNIT_SEL_M:
                        // Print distance on the top with decimal under 1000
                        if( Eeprom.EepromData.DemoSettings.RngDistance >= 1000.0 )
                        {
                            sprintf( StringText, "Range: %5d m", ( uint16_t )Eeprom.EepromData.DemoSettings.RngDistance );
                            sprintf( StringText2, "%6d", ( uint16_t )Eeprom.EepromData.DemoSettings.RngRawDistance );
                        }
                        else
                        {
                            sprintf( StringText, "Range: %5.1f m", Eeprom.EepromData.DemoSettings.RngDistance );
                            sprintf( StringText2, "%6.1f", Eeprom.EepromData.DemoSettings.RngRawDistance );
                        }
                        break;

                    case DEMO_RNG_UNIT_SEL_YD:
                        // Print distance on the top with decimal under 1000
                        if( Eeprom.EepromData.DemoSettings.RngDistance >= 1000.0 )
                        {
                            sprintf( StringText, "Range: %5d yd", ( uint16_t )Eeprom.EepromData.DemoSettings.RngDistance );
                            sprintf( StringText2, "%6d", ( uint16_t )Eeprom.EepromData.DemoSettings.RngRawDistance );
                        }
                        else
                        {
                            sprintf( StringText, "Range: %5.1f yd", Eeprom.EepromData.DemoSettings.RngDistance );
                            sprintf( StringText2, "%6.1f", Eeprom.EepromData.DemoSettings.RngRawDistance );
                        }
                        break;

                    case DEMO_RNG_UNIT_SEL_MI:
                        sprintf( StringText, "Range: %5.2f mi", Eeprom.EepromData.DemoSettings.RngDistance );
                        sprintf( StringText2, "%6.1f", Eeprom.EepromData.DemoSettings.RngRawDistance );
                        break;
                }
                DrawText( RNG_DIST_TEXT, ( uint8_t* )StringText, NULL, NULL );
                DrawText( LINE5_COL0_TEXT, NULL, ( uint8_t* )StringText2, NULL );
                DrawText( LINE6_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoNumSentPacket( ), NULL );
                DrawText( LINE7_COL0_TEXT, NULL, ( uint8_t* )GetFrequencyError( ), NULL );
                DrawText( LINE8_COL0_TEXT, NULL, ( uint8_t* )GetRngChannelsOk( ), NULL );
                sprintf( StringText, "%6d", Eeprom.EepromData.DemoSettings.CntPacketRxOKSlave );
                DrawText( LINE9_COL0_TEXT, NULL, ( uint8_t* )StringText, NULL );

                // Draw token at its new position. 270 is the absolute px position (y) of distance 0
                MenuObjects[RNG_DIST_CIRCLE].Ypos = 270 - DistPixel;
                GraphObjectDraw( &( MenuObjects[RNG_DIST_CIRCLE] ), NULL, true, false );
            }
            else
            {  
                GraphObject_t clearRect = { NULL, GO_RECTANGLE, 1, 284, 35, 238, \
                                            1, BACK_COLOR, BACK_COLOR, true, \
                                            BACK_COLOR, NULL, false };

                DrawText( TITLE_TEXT, ( uint8_t* )"   Outdoor ranging demo", NULL, NULL );
                DrawActiveButton( BTN0_BORDER, ( uint8_t* )"   MASTER", NULL );
                DisplayCurrentRadioParams( PAGE_RANGING_MASTER );
                // Draw lines
                GraphObjectDraw( &( MenuObjects[RNG_DS0_LINE] ), NULL, true, false );
                for( uint8_t i = RNG_DS1_LINE; i <= RNG_DS5_LINE; i++ )
                {
                    GraphObjectDraw( &( MenuObjects[i] ), NULL, true, false );
                }
                // Draw around scale of distance
                for( uint8_t i = SCALE_LINE_COUNT; i > 0; i-- )
                {
                    sprintf( StringText, "%d", i * ( Eeprom.EepromData.DemoSettings.RngFullScale / SCALE_LINE_COUNT ) );
                    GraphObjectDraw( &( MenuObjects[RNG_DS1_LINE + i - 1] ), NULL, false, false );
                    DrawText( RNG_DS1_TEXT + i - 1, ( uint8_t* )StringText, NULL, NULL );
                }
                // Clear the area for 2 downstair buttons (7 & 8)
                // { Id, Type, Xpos, Ypos, Height, Width, LineWidth, BackColor,  \
                     FrontColor,  DoFill, FillCol, Source,  TouchActive }
                GraphObjectClear( &clearRect, true );
                GraphObjectDraw( &( MenuObjects[PAGE_BORDER] ), NULL, false, false );
                DrawText( RNG_DIST_TEXT, ( uint8_t* )"Range: -----", NULL, NULL );
                DrawText( LINE5_COL0_TEXT, ( uint8_t* )"Raw [m]  :", ( uint8_t* )" -----", NULL );
                DrawText( LINE6_COL0_TEXT, ( uint8_t* )"Packet   :", ( uint8_t* )GetMenuDemoNumSentPacket( ), NULL );
                DrawText( LINE7_COL0_TEXT, ( uint8_t* )"FEI [Hz] :", ( uint8_t* )GetFrequencyError( ), NULL );
                DrawText( LINE8_COL0_TEXT, ( uint8_t* )"Channel  :", ( uint8_t* )GetRngChannelsOk( ), NULL );

                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
                // draw active rectangle for user touch (like "REFRESH" button)
                GraphObjectDraw( &( MenuObjects[RNG_MST_SCR] ), NULL, false, true );
            }
            CurrentPage = PAGE_RANGING_MASTER;
            break;

        case PAGE_RANGING_SLAVE:
            if( CurrentPage == PAGE_RANGING_SLAVE )
            {
                if( Eeprom.EepromData.DemoSettings.HoldDemo == true )
                {
                    ButtonChangeText( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                }
                DrawText( LINE4_COL0_TEXT, NULL, ( uint8_t* )GetMenuDemoNumSentPacket( ), NULL );
                DrawText( LINE5_COL0_TEXT, NULL, ( uint8_t* )GetFrequencyError( ), NULL );
                DrawText( LINE6_COL0_TEXT, NULL, ( uint8_t* )GetRngChannelsOk( ), NULL );
                sprintf( StringText ,"  %s", GetRangingAntenna( ) );
                DrawText( LINE7_COL0_TEXT, NULL, ( uint8_t* )StringText, NULL );
            }
            else
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"   Outdoor ranging demo", NULL, NULL );
                DisplayCurrentRadioParams( PAGE_RANGING_SLAVE );
                DrawText( LINE4_COL0_TEXT, ( uint8_t* )"Packet   :", ( uint8_t* )GetMenuDemoNumSentPacket( ), NULL );
                DrawText( LINE5_COL0_TEXT, ( uint8_t* )"FEI [Hz] :", ( uint8_t* )GetFrequencyError( ), NULL );
                DrawText( LINE6_COL0_TEXT, ( uint8_t* )"Channel  :", ( uint8_t* )GetRngChannelsOk( ), NULL );
                sprintf( StringText ,"  %s", GetRangingAntenna( ) );
                DrawText( LINE7_COL0_TEXT, ( uint8_t* )"Antenna :", ( uint8_t* )StringText, NULL );
                DrawActiveButton( BTN0_BORDER, ( uint8_t* )"   SLAVE", NULL );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
            }
            CurrentPage = PAGE_RANGING_SLAVE;
            break;

        case PAGE_RADIO_PARAM:
            if( CurrentPage == PAGE_RADIO_PARAM )
            {
                if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING )
                {
                    if( PreviousPage == START_PAGE )
                    {
                        ButtonChangeText( BTN0_BORDER, NULL, ( uint8_t* )GetMenuRadioFrameType( ) );
                    }
                    else
                    {
                        RemoveButton( BTN0_BORDER );
                    }
                    DrawActiveButton( BTN6_BORDER, ( uint8_t* )"     RANGING SETTINGS", NULL );
                    DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  BEST SET", NULL );
                }
                else
                {
                    DrawActiveButton( BTN0_BORDER, ( uint8_t* )"Mod:", ( uint8_t* )GetMenuRadioFrameType( ) );
                    DrawActiveButton( BTN6_BORDER, ( uint8_t* )"Payload:", ( uint8_t* )GetRadioPayloadLength( ) );
                    RemoveButton( BTN9_BORDER );
                }
                ButtonChangeText( BTN1_BORDER, NULL, ( uint8_t* )GetRadioFrequency( ) );
                ButtonChangeText( BTN2_BORDER, NULL, ( uint8_t* )GetRadioTxPower( ) );
                ButtonChangeText( BTN3_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters1( ) );
                ButtonChangeText( BTN4_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters2( ) );
                ButtonChangeText( BTN5_BORDER, NULL, ( uint8_t* )GetRadioModulationParameters3( ) );
            }
            else
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"      Radio Settings", NULL, NULL );
                if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING )
                {
                    if( PreviousPage == START_PAGE )
                    {
                        DrawActiveButton( BTN0_BORDER, ( uint8_t* )"Mod:", ( uint8_t* )GetMenuRadioFrameType( ) );
                    }
                    DrawActiveButton( BTN6_BORDER, ( uint8_t* )"     RANGING SETTINGS", NULL );
                    DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  BEST SET", NULL );
                }
                else
                {
                    DrawActiveButton( BTN0_BORDER, ( uint8_t* )"Mod:", ( uint8_t* )GetMenuRadioFrameType( ) );
                    DrawActiveButton( BTN6_BORDER, ( uint8_t* )"Payload:", ( uint8_t* )GetRadioPayloadLength( ) );
                }
                DrawActiveButton( BTN1_BORDER, ( uint8_t* )"Freq:", ( uint8_t* )GetRadioFrequency( ) );
                DrawActiveButton( BTN2_BORDER, ( uint8_t* )"Tx Pow:", ( uint8_t* )GetRadioTxPower( ) );
                DrawActiveButton( BTN3_BORDER, ( uint8_t* )"Param 1:", ( uint8_t* )GetRadioModulationParameters1( ) );
                DrawActiveButton( BTN4_BORDER, ( uint8_t* )"Param 2:", ( uint8_t* )GetRadioModulationParameters2( ) );
                DrawActiveButton( BTN5_BORDER, ( uint8_t* )"Param 3:", ( uint8_t* )GetRadioModulationParameters3( ) );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"   CANCEL", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )" OK & SAVE", NULL );
                CurrentPage = PAGE_RADIO_PARAM;
            }
            break;

        case PAGE_RANGING_PARAM:
            if( CurrentPage != PAGE_RANGING_PARAM )
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"    Settings : Ranging", NULL, NULL );

                DrawActiveButton( BTN1_BORDER, ( uint8_t* )"Request:", ( uint8_t* )GetRangingRequestCount( ) );
                DrawActiveButton( BTN2_BORDER, ( uint8_t* )"Address:", ( uint8_t* )GetRangingAddress( ) );
                DrawActiveButton( BTN3_BORDER, ( uint8_t* )"Antenna:", ( uint8_t* )GetRangingAntenna( ) );
                DrawActiveButton( BTN4_BORDER, ( uint8_t* )"Unit:", ( uint8_t* )GetRangingUnit( ) );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"   CANCEL", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )" OK & SAVE", NULL );
            }
            CurrentPage = PAGE_RANGING_PARAM;
            break;

        case PAGE_RADIO_PARAM_FREQ:
            if( CurrentPage != PAGE_RADIO_PARAM_FREQ )
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"   Radio Settings : Freq", NULL, NULL );
                DrawActiveButton( BTN0_BORDER, ( uint8_t* )"     -", NULL );
                DrawActiveButton( BTN1_BORDER, ( uint8_t* )"Step:", ( uint8_t* )GetRadioFreqBase( ) );
                // do not draw boarder for indicate actual frequency (it is not a button)
                ButtonChangeText( BTN2_BORDER, ( uint8_t* )"Freq:", ( uint8_t* )GetRadioFrequency( ) );
                DrawActiveButton( BTN3_BORDER, ( uint8_t* )"Preset1:", ( uint8_t* )GetRadioFreqBasePS1( ) );
                DrawActiveButton( BTN4_BORDER, ( uint8_t* )"Preset2:", ( uint8_t* )GetRadioFreqBasePS2( ) );
                DrawActiveButton( BTN5_BORDER, ( uint8_t* )"Preset3:", ( uint8_t* )GetRadioFreqBasePS3( ) );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"   CANCEL", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"     OK", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"     +", NULL );
            }
            CurrentPage = PAGE_RADIO_PARAM_FREQ;
            break;

        case PAGE_UTILITIES:
            if( CurrentPage == PAGE_UTILITIES )
            {
                DrawText( LINE1_TEXT, ( uint8_t* )GetGpsTime( ), NULL, NULL );
                DrawText( LINE2_TEXT, ( uint8_t* )GetGpsPos( ), NULL, NULL );
                DrawText( LINE3_TEXT, ( uint8_t* )GetProximityValue( ), NULL, NULL );
            }
            else
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"         Utilities", NULL, NULL );
                // FIRMWARE_VERSION is defined in DemoApplication.h
                DrawText( LINE0_TEXT, ( uint8_t* )FIRMWARE_VERSION, NULL, NULL );
                DrawText( LINE1_TEXT, ( uint8_t* )GetGpsTime( ), NULL, NULL );
                DrawText( LINE2_TEXT, ( uint8_t* )GetGpsPos( ), NULL, NULL );
                DrawText( LINE3_TEXT, ( uint8_t* )GetProximityValue( ), NULL, NULL );
                DrawActiveButton( BTN0_BORDER, ( uint8_t* )"Fact. Reset", NULL );
                DrawActiveButton( BTN4_BORDER, ( uint8_t* )"PA Mode:", ( uint8_t* )GetMenuDemoRadioPowerMode( ) );
                DrawActiveButton( BTN5_BORDER, ( uint8_t* )"Packets:", ( uint8_t* )GetMenuDemoMaxNumPacket( ) );
                DrawActiveButton( BTN6_BORDER, ( uint8_t* )"Antenna:", ( uint8_t* )GetAntennaSetting( ) );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"   CANCEL", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )" OK & SAVE", NULL );
            }
            CurrentPage = PAGE_UTILITIES;
            break;

        case PAGE_RADIO_TEST:
            DrawText( TITLE_TEXT, ( uint8_t* )"     Radio Test Modes", NULL, NULL );
            DrawActiveButton( BTN1_BORDER, ( uint8_t* )"        Sleep Mode", NULL );
            DrawActiveButton( BTN2_BORDER, ( uint8_t* )"      Standby RC Mode", NULL );
            DrawActiveButton( BTN3_BORDER, ( uint8_t* )"     Standby XOSC Mode", NULL );
            DrawActiveButton( BTN4_BORDER, ( uint8_t* )"        Set Tx CW", NULL );
            DrawActiveButton( BTN5_BORDER, ( uint8_t* )"    Set Cont. Modulation", NULL );
            DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    BACK", NULL );
            CurrentPage = PAGE_RADIO_TEST;
            break;

        case PAGE_SLEEP_MODE:
            if( CurrentPage != PAGE_UTILITIES )
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"        Sleep Mode", NULL, NULL );
                DisplayCurrentRadioParams( PAGE_SLEEP_MODE );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
                CurrentPage = PAGE_SLEEP_MODE;
            }
            break;

        case PAGE_STBY_RC_MODE:
            if( CurrentPage != PAGE_STBY_RC_MODE )
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"     Standby RC Mode", NULL, NULL );
                DisplayCurrentRadioParams( PAGE_STBY_RC_MODE );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
                CurrentPage = PAGE_STBY_RC_MODE;
            }
            break;

        case PAGE_STBY_XOSC_MODE:
            if( CurrentPage != PAGE_STBY_XOSC_MODE )
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"     Standby XOSC Mode", NULL, NULL );
                DisplayCurrentRadioParams( PAGE_STBY_XOSC_MODE );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
                CurrentPage = PAGE_STBY_XOSC_MODE;
            }
            break;

        case PAGE_TX_CW:
            if( CurrentPage != PAGE_TX_CW )
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"        Set Tx CW", NULL, NULL );
                DisplayCurrentRadioParams( PAGE_TX_CW );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
                CurrentPage = PAGE_TX_CW;
            }
            break;

        case PAGE_CONT_MODULATION:
            if( CurrentPage != PAGE_CONT_MODULATION )
            {
                DrawText( TITLE_TEXT, ( uint8_t* )"   Set Cont. Modulation", NULL, NULL );
                DisplayCurrentRadioParams( PAGE_CONT_MODULATION );
                DrawActiveButton( BTN7_BORDER, ( uint8_t* )"    EXIT", NULL );
                DrawActiveButton( BTN8_BORDER, ( uint8_t* )"   START", NULL );
                DrawActiveButton( BTN9_BORDER, ( uint8_t* )"  SETTINGS", NULL );
                CurrentPage = PAGE_CONT_MODULATION;
            }
            break;

        default:
            break;
    }
}

void DisplayCurrentRadioParams( uint8_t page )
{
    switch( page )
    {
        case PAGE_SLEEP_MODE:
        case PAGE_STBY_RC_MODE:
        case PAGE_STBY_XOSC_MODE:
        case PAGE_TX_CW:
            // Do display different without packet settings (no modulation here)
            // 2 + (9) + 4 + (7) + 2 + (4) = 28 (=<28 : ok)
            sprintf( StringText ,"f:%s, P:%s, %s", GetRadioFrequencyGHz( ), \
                     GetRadioTxPower( ), GetAntennaSetting( ) );
            DrawText( LINE0_TEXT, ( uint8_t* )StringText, NULL, NULL );
            sprintf( StringText ,"Radio Power Mode: %s", ( uint8_t* )GetMenuDemoRadioPowerMode( ) );
            DrawText( LINE1_TEXT, ( uint8_t* )StringText, NULL, NULL );
            sprintf( StringText ,"Press START to launch test" );
            DrawText( LINE3_TEXT, ( uint8_t* )StringText, NULL, NULL );
            break;

        case PAGE_CONT_MODULATION:
            // 2 + (9) + 4 + (7) + 2 + (4) = 28 (=<28 : ok)
            sprintf( StringText ,"f:%s, P:%s, %s", GetRadioFrequencyGHz( ), \
                     GetRadioTxPower( ), GetAntennaSetting( ) );
            DrawText( LINE0_TEXT, ( uint8_t* )StringText, NULL, NULL );
            // (7) + 2 + (19) = 28 (=<28 : ok)
            sprintf( StringText ,"%s: %s", GetMenuRadioFrameType( ), \
                                           GetRadioModulationParameters1( ) );
            DrawText( LINE1_TEXT, ( uint8_t* )StringText, NULL, NULL );
            // (3) + 2 + (10) + 2 + (8) = 25 (=<28 : ok)
            sprintf( StringText ,"%s, %s, %s", GetRadioModulationParameters2( ), \
                                               GetRadioModulationParameters3( ),
                                               ( uint8_t* )GetMenuDemoRadioPowerMode( ) );
            DrawText( LINE2_TEXT, ( uint8_t* )StringText, NULL, NULL );
            sprintf( StringText ,"Press START to launch test" );
            DrawText( LINE4_TEXT, ( uint8_t* )StringText, NULL, NULL );
            break;

        case PAGE_PING_PONG:    // standart display
        case PAGE_PER:
            // 2 + (9) + 4 + (7) + 2 + (4) = 28 (=<28 : ok)
            sprintf( StringText ,"f:%s, P:%s, %s", GetRadioFrequencyGHz( ), \
                     GetRadioTxPower( ), GetAntennaSetting( ) );
            DrawText( LINE0_TEXT, ( uint8_t* )StringText, NULL, NULL );
            // (7) + 2 + (19) = 28 (=<28 : ok)
            sprintf( StringText ,"%s: %s", GetMenuRadioFrameType( ), \
                                           GetRadioModulationParameters1( ) );
            DrawText( LINE1_TEXT, ( uint8_t* )StringText, NULL, NULL );
            // 3 + (3) + 2 + (10) + 2 + (8) = 28 (=<28 : ok)
            sprintf( StringText ,"PL:%s, %s, %s", GetRadioPayloadLength( ), \
                                                  GetRadioModulationParameters2( ), \
                                                  GetRadioModulationParameters3( ) );
            DrawText( LINE2_TEXT, ( uint8_t* )StringText, NULL, NULL );
            break;

        case PAGE_RANGING_SLAVE:
            // 2 + (9) + 4 + (7) = 22 (=<28 : ok)
            sprintf( StringText ,"f:%s, P:%s", GetRadioFrequencyGHz( ), \
                     GetRadioTxPower( ) );
            DrawText( LINE0_TEXT, ( uint8_t* )StringText, NULL, NULL );
            // (7) + 2 + (19) = 28 (=<28 : ok)
            sprintf( StringText ,"%s: %s", GetMenuRadioFrameType( ), \
                                           GetRadioModulationParameters1( ) );
            DrawText( LINE1_TEXT, ( uint8_t* )StringText, NULL, NULL );
            // (10) + 2 + (8) = 20 (=<28 : ok)
            sprintf( StringText ,"%s", GetRadioModulationParameters2( ) );
            DrawText( LINE2_TEXT, ( uint8_t* )StringText, NULL, NULL );
            break;

        case PAGE_RANGING_MASTER:
            sprintf( StringText ,"f:%s", GetRadioFrequencyGHz( ) );
            DrawText( LINE1_TEXT, ( uint8_t* )StringText, NULL, NULL );
            DrawText( LINE2_COL0_TEXT, ( uint8_t* )"Antenna", ( uint8_t* )GetRangingAntenna( ), NULL );
            sprintf( StringText ,"%s", GetRadioTxPower( ) );
            MenuObjects[LINE3_COL1_TEXT].FrontColor = TEXT_COLOR;
            DrawText( LINE3_COL0_TEXT, ( uint8_t* )"Tx Power", ( uint8_t* )StringText, NULL );
            sprintf( StringText ,"%s", GetRadioModulationParameters2( ) );
            DrawText( LINE4_COL0_TEXT, ( uint8_t* )StringText, NULL, NULL );
            sprintf( StringText ,"%s", GetRadioModulationParameters1( ) );
            MenuObjects[LINE4_COL1_TEXT].FrontColor = TEXT_COLOR;
            DrawText( LINE4_COL1_TEXT, ( uint8_t* )StringText, NULL, NULL );
            MenuObjects[LINE3_COL1_TEXT].FrontColor = TEXT_VALUE;
            MenuObjects[LINE4_COL1_TEXT].FrontColor = TEXT_VALUE;
            break;
    }
}

char* GetMenuRadioFrameType( void )
{
    switch( Eeprom.EepromData.DemoSettings.ModulationType )
    {
        case PACKET_TYPE_FLRC:    return ( char* )" FLRC";
        case PACKET_TYPE_RANGING: return ( char* )"RANGING";
        case PACKET_TYPE_GFSK:    return ( char* )" GFSK";
        case PACKET_TYPE_BLE:     return ( char* )"  BLE";
        case PACKET_TYPE_LORA:
        default:                  return ( char* )" LORA";
    }
}

char* GetRadioModulationParameters1( void )
{
    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_FLRC )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam1 )
        {
            case FLRC_BR_1_300_BW_1_2: return ( char* )"1.3 Mbps/BW 1.2 MHz";
            case FLRC_BR_1_040_BW_1_2: return ( char* )"1.0 Mbps/BW 1.2 MHz";
            case FLRC_BR_0_650_BW_0_6: return ( char* )"650 kbps/BW 600 kHz";
            case FLRC_BR_0_520_BW_0_6: return ( char* )"520 kbps/BW 600 kHz";
            case FLRC_BR_0_325_BW_0_3: return ( char* )"325 kbps/BW 300 kHz";
            case FLRC_BR_0_260_BW_0_3: return ( char* )"260 kbps/BW 300 kHz";
            default:                   return ( char* )"X";
        }
    }
    else if( ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_GFSK ) || \
             ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_BLE ) )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam1 )
        {
            case GFSK_BLE_BR_2_000_BW_2_4: return ( char* )"2.0 Mbps/BW 2.4 MHz";
            case GFSK_BLE_BR_1_600_BW_2_4: return ( char* )"1.6 Mbps/BW 2.4 MHz";
            case GFSK_BLE_BR_1_000_BW_2_4: return ( char* )"1.0 Mbps/BW 2.4 MHz";
            case GFSK_BLE_BR_1_000_BW_1_2: return ( char* )"1.0 Mbps/BW 1.2 MHz";
            case GFSK_BLE_BR_0_800_BW_2_4: return ( char* )"800 kbps/BW 2.4 MHz";
            case GFSK_BLE_BR_0_800_BW_1_2: return ( char* )"800 kbps/BW 1.2 MHz";
            case GFSK_BLE_BR_0_500_BW_1_2: return ( char* )"500 kbps/BW 1.2 MHz";
            case GFSK_BLE_BR_0_500_BW_0_6: return ( char* )"500 kbps/BW 600 kHz";
            case GFSK_BLE_BR_0_400_BW_1_2: return ( char* )"400 kbps/BW 1.2 MHz";
            case GFSK_BLE_BR_0_400_BW_0_6: return ( char* )"400 kbps/BW 600 kHz";
            case GFSK_BLE_BR_0_250_BW_0_6: return ( char* )"250 kbps/BW 600 kHz";
            case GFSK_BLE_BR_0_250_BW_0_3: return ( char* )"250 kbps/BW 300 kHz";
            case GFSK_BLE_BR_0_125_BW_0_3: return ( char* )"125 kbps/BW 300 kHz";
            default:                      return ( char* )"X";
        }
    }
    else if( ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA ) || \
             ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING ) )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam1 )
        {
            case LORA_SF5:  return ( char* )"SF5";
            case LORA_SF6:  return ( char* )"SF6";
            case LORA_SF7:  return ( char* )"SF7";
            case LORA_SF8:  return ( char* )"SF8";
            case LORA_SF9:  return ( char* )"SF9";
            case LORA_SF10: return ( char* )"SF10";
            case LORA_SF11: return ( char* )"SF11";
            case LORA_SF12: return ( char* )"SF12";
            default:        return ( char* )"X";
        }
    }
    else
    {
        return ( char* )"";
    }
}

char* GetRadioModulationParameters2( void )
{
    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_FLRC )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam2 )
        {
            case FLRC_CR_1_2: return ( char* )"CR 1/2";
            case FLRC_CR_3_4: return ( char* )"CR 3/4";
            case FLRC_CR_1_0: return ( char* )"CR 1";
            default:          return ( char* )"X";
        }
    }
    else if( ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_GFSK ) || \
             ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_BLE ) )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam2 )
        {
            case GFSK_BLE_MOD_IND_0_35: return ( char* )"Mod.i 0.35";
            case GFSK_BLE_MOD_IND_0_50: return ( char* )"Mod.i 0.5";
            case GFSK_BLE_MOD_IND_0_75: return ( char* )"Mod.i 0.75";
            case GFSK_BLE_MOD_IND_1_00: return ( char* )"Mod.i 1";
            case GFSK_BLE_MOD_IND_1_25: return ( char* )"Mod.i 1.25";
            case GFSK_BLE_MOD_IND_1_50: return ( char* )"Mod.i 1.5";
            case GFSK_BLE_MOD_IND_1_75: return ( char* )"Mod.i 1.75";
            case GFSK_BLE_MOD_IND_2_00: return ( char* )"Mod.i 2";
            case GFSK_BLE_MOD_IND_2_25: return ( char* )"Mod.i 2.25";
            case GFSK_BLE_MOD_IND_2_50: return ( char* )"Mod.i 2.50";
            case GFSK_BLE_MOD_IND_2_75: return ( char* )"Mod.i 2.75";
            case GFSK_BLE_MOD_IND_3_00: return ( char* )"Mod.i 3";
            case GFSK_BLE_MOD_IND_3_25: return ( char* )"Mod.i 3.25";
            case GFSK_BLE_MOD_IND_3_50: return ( char* )"Mod.i 3.5";
            case GFSK_BLE_MOD_IND_3_75: return ( char* )"Mod.i 3.75";
            case GFSK_BLE_MOD_IND_4_00: return ( char* )"Mod.i 4";
            default:                   return ( char* )"X";
        }
    }
    else if( ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA ) || \
             ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING ) )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam2 )
        {
            case LORA_BW_0200: return ( char* )"BW 200 kHz";
            case LORA_BW_0400: return ( char* )"BW 400 kHz";
            case LORA_BW_0800: return ( char* )"BW 800 kHz";
            case LORA_BW_1600: return ( char* )"BW 1.6 MHz";
            default:           return ( char* )"X"; 
        }
    }
    else
    {
        return ( char* )"";
    }
}

char* GetRadioModulationParameters3( void )
{
    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_FLRC )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam3 )
        {
            case RADIO_MOD_SHAPING_BT_OFF: return ( char* )"BT OFF";
            case RADIO_MOD_SHAPING_BT_1_0: return ( char* )"BT 1";
            case RADIO_MOD_SHAPING_BT_0_5: return ( char* )"BT 0.5";
            default:                       return ( char* )"X";
        }
    }
    else if( ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_GFSK ) || \
             ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_BLE ) )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam3 )
        {
            case RADIO_MOD_SHAPING_BT_OFF: return ( char* )"BT OFF";
            case RADIO_MOD_SHAPING_BT_1_0: return ( char* )"BT 1";
            case RADIO_MOD_SHAPING_BT_0_5: return ( char* )"BT 0.5";
            default:                       return ( char* )"X";
        }
    }
    else if( ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA ) || \
             ( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_RANGING ) )
    {
        switch( Eeprom.EepromData.DemoSettings.ModulationParam3 )
        {
            case LORA_CR_4_5:    return ( char* )"CR 4/5";
            case LORA_CR_4_6:    return ( char* )"CR 4/6";
            case LORA_CR_4_7:    return ( char* )"CR 4/7";
            case LORA_CR_4_8:    return ( char* )"CR 4/8";
            case LORA_CR_LI_4_5: return ( char* )"CRLI 4/5";
            case LORA_CR_LI_4_6: return ( char* )"CRLI 4/6";
            case LORA_CR_LI_4_7: return ( char* )"CRLI 4/7";
            default:             return ( char* )"X";
        }
    }
    else
    {
        return ( char* )"";
    }
}

char* GetRadioFrequency( void )
{
    sprintf( StringText2, "%lu Hz", \
             ( unsigned long )Eeprom.EepromData.DemoSettings.Frequency );
    return StringText2;
}

char* GetRadioFrequencyGHz( void )
{
    // quicker than using a float (which is not optimized in MBED)
    sprintf( StringText3, "%1d.%03d GHz", \
             Eeprom.EepromData.DemoSettings.Frequency / 1000000000, \
             ( Eeprom.EepromData.DemoSettings.Frequency / 1000000 ) % 1000 );
    return StringText3;
}

char* GetRadioFreqBase( void )
{
    switch( CurrentFreqBase )
    {
        case FB1:    return ( char* )"1 Hz";
        case FB10:   return ( char* )"10 Hz";
        case FB100:  return ( char* )"100 Hz";
        case FB1K:   return ( char* )"1 kHz";
        case FB10K:  return ( char* )"10 kHz";
        case FB100K: return ( char* )"100 kHz";
        case FB1M:   return ( char* )"1 MHz";
        case FB10M:  return ( char* )"10 MHz";
        default:     return ( char* )"X";
    }
}

char* GetRadioFreqBasePS1( void )
{
    sprintf( StringText, "%lu Hz", DEMO_CENTRAL_FREQ_PRESET1 );
    return StringText;
}

char* GetRadioFreqBasePS2( void )
{
    sprintf( StringText, "%lu Hz", DEMO_CENTRAL_FREQ_PRESET2 );
    return StringText;
}

char* GetRadioFreqBasePS3( void )
{
    sprintf( StringText, "%lu Hz", DEMO_CENTRAL_FREQ_PRESET3 );
    return StringText;
}

char* GetRadioTxPower( void )
{
    sprintf( StringText2, "%d dBm", Eeprom.EepromData.DemoSettings.TxPower );
    return StringText2;
}

char* GetRadioPayloadLength( void )
{
    if( Eeprom.EepromData.DemoSettings.ModulationType == PACKET_TYPE_LORA )
    {
        sprintf( StringText2, "%d", Eeprom.EepromData.DemoSettings.PacketParam3 );
    }
    else // PACKET_TYPE_GFSK, PACKET_TYPE_FLRC
    {
        sprintf( StringText2, "%d", Eeprom.EepromData.DemoSettings.PacketParam5 );
    }
    return StringText2;
}

char* GetMenuDemoMaxNumPacket( void )
{
    if( Eeprom.EepromData.DemoSettings.MaxNumPacket == 0 )
    {
        return ( char* )"Infinite";
    }
    else
    {
        sprintf( StringText, "%6d", Eeprom.EepromData.DemoSettings.MaxNumPacket );
    }
    return StringText;
}

char* GetMenuDemoNumSentPacket( void )
{
    sprintf( StringText2, "%6d", Eeprom.EepromData.DemoSettings.CntPacketTx );
    return StringText2;
}

char* GetMenuDemoRxOk( void )
{
    sprintf( StringText2, "%6lu", \
            ( unsigned long )( Eeprom.EepromData.DemoSettings.CntPacketRxOK ) );
    return StringText2;
}

char* GetMenuDemoRxKo( void )
{
    sprintf( StringText2, "%6lu", \
             ( unsigned long )( Eeprom.EepromData.DemoSettings.CntPacketRxKO + \
             Eeprom.EepromData.DemoSettings.RxTimeOutCount ) ); 
    return StringText2;
}

char* GetMenuDemoRxOkSlave( void )
{
    sprintf( StringText3, "%6lu", \
       ( unsigned long )( Eeprom.EepromData.DemoSettings.CntPacketRxOKSlave ) );
    return StringText3;
}

char* GetMenuDemoResultPerCent1( uint32_t value, uint32_t reference )
{
    // quicker than using a float (which is not optimized in MBED)
    sprintf( StringText2, "%3d.%02d", \
             ( ( value * 10000 ) / reference ) / 100, \
             ( ( value * 10000 ) / reference ) % 100 );
    return StringText2;
}

char* GetMenuDemoResultPerCent2( uint32_t value, uint32_t reference )
{
    // quicker than using a float (which is not optimized in MBED)
    sprintf( StringText3, "%3d.%02d", \
             ( ( value * 10000 ) / reference ) / 100, \
             ( ( value * 10000 ) / reference ) % 100 );
    return StringText3;
}

char* GetMenuDemoRxKoSlave( void )
{
    sprintf( StringText3, "%6lu", \
       ( unsigned long )( Eeprom.EepromData.DemoSettings.CntPacketRxKOSlave ) );
    return StringText3;
}

char* GetMenuDemoRssi( void )
{
    sprintf( StringText2, "%6d", Eeprom.EepromData.DemoSettings.RssiValue );
    return StringText2;
}

char* GetMenuDemoSnr( void )
{
    if( Eeprom.EepromData.DemoSettings.SnrValue >= 0 )
    {
        sprintf( StringText2, "     /" ); 
    }
    else
    {
        sprintf( StringText2, "%6d", Eeprom.EepromData.DemoSettings.SnrValue );
    }
    return StringText2;
}

char* GetAntennaSetting( void )
{
    if( Eeprom.EepromData.DemoSettings.AntennaSwitch == 0 )
    {
        return ( char* )"ANT1";
    }
    else
    {
        return ( char* )"ANT2";
    }
}

char* GetTotalPackets( void )
{
    if( Eeprom.EepromData.DemoSettings.MaxNumPacket == 0 )
    {
        return ( char* )"Total:  Inf.";
    }
    else
    {
        sprintf( StringText, "Total: %5lu", \
                 ( unsigned long )( Eeprom.EepromData.DemoSettings.MaxNumPacket ) );
        return StringText;
    }
}

char* GetGpsTime( void )
{
    thisGps = Max7GpsgetData( );
    if( ( thisGps->Position.Fixed ) && ( thisGps->Time.Updated ) )
    {
        sprintf( StringText, "GPS:  %s.%s.%s %s:%s:%s", thisGps->Time.Year, \
                                                        thisGps->Time.Month, \
                                                        thisGps->Time.Day, \
                                                        thisGps->Time.Hour, \
                                                        thisGps->Time.Minute, \
                                                        thisGps->Time.Second );
        thisGps->Time.Updated = false;
        return StringText;
    }
    else
    {
        return ( char* )"GPS: Satellites searching..";
    }
}

char* GetGpsPos( void )
{
    thisGps = Max7GpsgetData( );
    if( thisGps->Position.Fixed )
    {
        sprintf( StringText,"%s, %s", thisGps->Position.Lat, \
                                      thisGps->Position.Long );
        return StringText;
    }
    else
    {
        return ( char* )"Pos: Satellites searching..";
    }
}

char* GetProximityValue( void )
{
    sprintf( StringText,"Proximity : %06d, %06d", \
                        SX9306proximityGetReadValue( 1 ), \
                        SX9306proximityGetReadValue( 0 ) ); // Left then right
    return StringText;
}

char* GetMenuDemoRadioPowerMode( void )
{
    if( Eeprom.EepromData.DemoSettings.RadioPowerMode == USE_LDO )
    {
        return ( char* )"  LDO";
    }
    else
    {
        return ( char* )"  DCDC";
    }
}

char* GetFrequencyError( void )
{
    sprintf( StringText2, "%6d", ( int32_t )Eeprom.EepromData.DemoSettings.RngFei );
    return StringText2;
}

char* GetRngChannelsOk( void )
{
    if( Eeprom.EepromData.DemoSettings.Entity == SLAVE )
    {
        Eeprom.EepromData.DemoSettings.CntPacketRxOK /= 2;
    }
    sprintf( StringText2, "%03d/%03d", Eeprom.EepromData.DemoSettings.CntPacketRxOK, Eeprom.EepromData.DemoSettings.RngRequestCount );
    return StringText2;
}

char* GetRangingRequestCount( void )
{
    sprintf( StringText2, "%d", Eeprom.EepromData.DemoSettings.RngRequestCount );
    return StringText2;
}

char* GetRangingAddress( void )
{
    sprintf( StringText2, "0x%08x", Eeprom.EepromData.DemoSettings.RngAddress );
    return StringText2;
}

char* GetRangingAntenna( void )
{
    switch( Eeprom.EepromData.DemoSettings.RngAntenna )
    {
        case DEMO_RNG_ANT_1:    return ( char* )"ANT1";
        case DEMO_RNG_ANT_2:    return ( char* )"ANT2";
        case DEMO_RNG_ANT_BOTH: return ( char* )"BOTH";
        default:                return ( char* )"X";
    }
}

char* GetRangingUnit( void )
{
    switch( Eeprom.EepromData.DemoSettings.RngUnit )
    {
        case DEMO_RNG_UNIT_SEL_M:  return ( char* )"Meter";
        case DEMO_RNG_UNIT_SEL_YD: return ( char* )"Yard";
        case DEMO_RNG_UNIT_SEL_MI: return ( char* )"Mile";
        default:                   return ( char* )"X";
    }
}

void CheckScreen( void )
{
    CheckScreenFlag = true;
}

void CheckRefresh( void )
{
    PageRefresh = true;
}

void DebouncedScreen (void )
{
    ScreenBeenDebounced = false;
    DebouncedScreenTouch.detach( );
}

/*!
 * \brief Draw button
 *
 * \param [in]  buttonId      Id of the button to draw
 * \param [in]  *text1        Text to draw as label
 * \param [in]  *text2        Text to draw as value
 */
void DrawActiveButton( uint8_t buttonId, uint8_t* text1, uint8_t* text2 )
{
    GraphObjectDraw( &( MenuObjects[buttonId] ), NULL, NULL, true );
    if( text2 == NULL )
    {
        if( text1 != NULL )
        {
            GraphObjectDraw( &( MenuObjects[buttonId + 1] ), text1, NULL, false );
        }
    }
    else
    {
        if( text1 != NULL )
        {
            GraphObjectDraw( &( MenuObjects[buttonId + 2] ), text1, NULL, false );
        }
        GraphObjectDraw( &( MenuObjects[buttonId + 3] ), text2, NULL, false );
    }
}

void ButtonChangeText( uint8_t buttonId, uint8_t* text1, uint8_t* text2 )
{
    if( text2 == NULL )
    {
        if( text1 != NULL )
        {
            GraphObjectDraw( &( MenuObjects[buttonId + 1] ), text1, NULL, false );
        }
    }
    else
    {
        if( text1 != NULL )
        {
            GraphObjectDraw( &( MenuObjects[buttonId + 2] ), text1, NULL, false );
        }
        GraphObjectDraw( &( MenuObjects[buttonId + 3] ), text2, NULL, false );
    }
}

void RemoveButton( uint8_t buttonId )
{
    GraphObjectClear( &( MenuObjects[buttonId] ), true );
}

void DrawText( uint8_t lineId, uint8_t* text0, uint8_t* text1, uint8_t* text2 )
{
    if( text0 != NULL )
    {
        GraphObjectDraw( &( MenuObjects[lineId] ), text0, NULL, false );
    }
    if( text1 != NULL )
    {
        GraphObjectDraw( &( MenuObjects[lineId + 1] ), text1, NULL, false );
    }
    if( text2 != NULL )
    {
        GraphObjectDraw( &( MenuObjects[lineId + 2] ), text2, NULL, false );
    }
}
