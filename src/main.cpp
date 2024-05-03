#include "common.h"
// clang-format off
/*****************************************************************************************************************************************************
    MiniWebRadio -- Webradio receiver for ESP32

    first release on 03/2017                                                                                                      */String Version ="\
    Version 3.01h  May 01/2024                                                                                                                       ";

/*  2.8" color display (320x240px) with controller ILI9341 or HX8347D (SPI) or
    3.5" color display (480x320px) wiht controller ILI9486 or ILI9488 (SPI)


    SD_MMC is mandatory
    IR remote is optional
    BT Transmitter is optional

*****************************************************************************************************************************************************/

// THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED
// WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
// AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

// clang-format on

// global variables

enum status {
    NONE = 0,
    RADIO = 1,
    PLAYER = 2,
    DLNA = 3,
    CLOCK = 4,
    BRIGHTNESS = 5,
    ALARM = 6,
    SLEEP = 9,

    STATIONSLIST = 11,
    AUDIOFILESLIST = 12,
    DLNAITEMSLIST = 13,
    BLUETOOTH = 14,
    EQUALIZER = 15,
    CLOCKico = 16,
    UNDEFINED = 255
};

char _hl_item[16][40]{"",                 // none
                      "Internet Radio",   // "* интернет-радио *"  "ραδιόφωνο Internet"
                      "Audio player",     // "** цифрово́й плеер **
                      "DLNA",             // Digital Living Network Alliance
                      "Clock",            // Clock "** часы́ **"  "** ρολόι **"
                      "Brightness",       // Brightness яркость λάμψη
                      "Alarm (hh:mm)",    // Alarm
                      "Off Timer (h:mm)", // "Sleeptimer" "Χρονομετρητής" "Таймер сна"
                      ""
                      "Stations List",
                      "Audio Files",
                      "DLNA List",
                      "Bluetooth",
                      "Equalizer",
                      ""
                      ""};

const uint8_t       _max_volume = 21;
const uint16_t      _max_stations = 1000;
int16_t             _releaseNr = -1;
int8_t              _currDLNAsrvNr = -1;
uint8_t             _alarmdays = 0;
uint8_t             _cur_volume = 0;           // will be set from stored preferences
uint8_t             _ringvolume = _max_volume; //
uint8_t             _brightness = 0;
uint8_t             _state = UNDEFINED;  // statemaschine
uint8_t             _commercial_dur = 0; // duration of advertising
uint8_t             _cur_Codec = 0;
uint8_t             _VUleftCh = 0;   // VU meter left channel
uint8_t             _VUrightCh = 0;  // VU meter right channel
uint8_t             _numServers = 0; //
uint8_t             _level = 0;
uint8_t             _timeFormat = 24; // 24 or 12
uint8_t             _staListPos = 0;
uint8_t             _semaphore = 0;
uint16_t            _staListNr = 0;
uint8_t             _fileListPos = 0;
uint8_t             _radioSubmenue = 0;
uint8_t             _playerSubmenue = 0;
uint8_t             _clockSubMenue = 0;
uint16_t            _fileListNr = 0;
uint8_t             _itemListPos = 0; // DLNA items
uint16_t            _dlnaItemNr = 0;
uint8_t             _dlnaLevel = 0;
int8_t              _rssi_bt = -127;
int16_t             _alarmtime[7] = {0};  // in minutes (23:59 = 23 *60 + 59) [0] Sun, [1] Mon
int16_t             _toneLP = 0;          // -40 ... +6 (dB)        audioI2S
int16_t             _toneBP = 0;          // -40 ... +6 (dB)        audioI2S
int16_t             _toneHP = 0;          // -40 ... +6 (dB)        audioI2S
int16_t             _toneBAL = 0;         // -16...0....+16         audioI2S
uint16_t            _icyBitRate = 0;      // from http response header via event
uint16_t            _avrBitRate = 0;      // from decoder via getBitRate(true)
uint16_t            _cur_station = 0;     // current station(nr), will be set later
uint16_t            _cur_AudioFileNr = 0; // position inside _SD_content
uint16_t            _sleeptime = 0;       // time in min until MiniWebRadio goes to sleep
uint16_t            _sum_stations = 0;
uint16_t            _plsCurPos = 0;
uint16_t            _totalNumberReturned = 0;
uint16_t            _dlnaMaxItems = 0;
uint32_t            _resumeFilePos = 0; //
uint32_t            _playlistTime = 0;  // playlist start time millis() for timeout
uint32_t            _settingsHash = 0;
uint32_t            _audioFileSize = 0;
uint32_t            _media_downloadPort = 0;
uint32_t            _audioCurrentTime = 0;
uint32_t            _audioFileDuration = 0;
uint8_t             _resetResaon = (esp_reset_reason_t)ESP_RST_UNKNOWN;
const char*         _pressBtn[8];
const char*         _releaseBtn[8];
char                _chbuf[512];
char                _fName[256];
char                _myIP[25] = {0};
char                _path[128];
char                _prefix[5] = "/s";
char                _commercial[25];
char                _icyDescription[512] = {};
char                _streamTitle[512] = {};
char*               _lastconnectedfile = NULL;
char*               _stationURL = NULL;
char*               _JSONstr = NULL;
char*               _BT_metaData = NULL;
char*               _playlistPath = NULL;
bool                _f_rtc = false; // true if time from ntp is received
bool                _f_100ms = false;
bool                _f_1sec = false;
bool                _f_10sec = false;
bool                _f_1min = false;
bool                _f_mute = false;
bool                _f_sleeping = false;
bool                _f_isWebConnected = false;
bool                _f_isFSConnected = false;
bool                _f_eof = false;
bool                _f_eof_alarm = false;
bool                _f_alarm = false;
bool                _f_irNumberSeen = false;
bool                _f_newIcyDescription = false;
bool                _f_newStreamTitle = false;
bool                _f_newBitRate = false;
bool                _f_newLogoAndStation = false;
bool                _f_newCommercial = false;
bool                _f_volBarVisible = false;
bool                _f_switchToClock = false;    // jump into CLOCK mode at the next opportunity
bool                _f_hpChanged = false;        // true, if HeadPhone is plugged or unplugged
bool                _f_timeAnnouncement = false; // time announcement every full hour
bool                _f_playlistEnabled = false;
bool                _f_playlistNextFile = false;
bool                _f_logoUnknown = false;
bool                _f_pauseResume = false;
bool                _f_accessPoint = false;
bool                _f_VUmeterIsVisible = false;
bool                _f_SD_Upload = false;
bool                _f_PSRAMfound = false;
bool                _f_FFatFound = false;
bool                _f_SD_MMCfound = false;
bool                _f_ESPfound = false;
bool                _f_clearLogo = false;
bool                _f_clearStationName = false;
bool                _f_shuffle = false;
bool                _f_dlnaBrowseServer = false;
bool                _f_dlnaWaitForResponse = false;
bool                _f_dlnaSeekServer = false;
String              _station = "";
String              _stationName_nvs = "";
String              _stationName_air = "";
String              _homepage = "";
String              _filename = "";
String              _lastconnectedhost = "";
String              _scannedNetworks = "";
String              _curAudioFolder = "/audiofiles";
String              _TZName = "Europe/Berlin";
String              _TZString = "CET-1CEST,M3.5.0,M10.5.0/3";
String              _media_downloadIP = "";
std::vector<String> _names{};
std::vector<char*>  _SD_content;
std::vector<char*>  _PLS_content;

struct timecounter {
    uint8_t timer = 0;
    float   factor = 2.0;
} _timeCounter;

struct dlnaHistory {
    char* objId = NULL;
    char* name = NULL;
} _dlnaHistory[10];

const char* codecname[10] = {"unknown", "WAV", "MP3", "AAC", "M4A", "FLAC", "AACP", "OPUS", "OGG", "VORBIS"};

Preferences    pref;
Preferences    stations;
WebSrv         webSrv;
WiFiMulti      wifiMulti;
RTIME          rtc;
Ticker         ticker100ms;
IR             ir(IR_PIN); // do not change the objectname, it must be "ir"
TP             tp(TP_CS, TP_IRQ);
File           audioFile;
FtpServer      ftpSrv;
WiFiClient     client;
WiFiUDP        udp;
DLNA_Client    dlna;
KCX_BT_Emitter bt_emitter(BT_EMITTER_RX, BT_EMITTER_TX, BT_EMITTER_LINK, BT_EMITTER_MODE);

#if DECODER == 2 // ac101
AC101 dac;
#endif
#if DECODER == 3 // es8388
ES8388 dac;
#endif
#if DECODER == 4 // wm8978
WM8978 dac;
#endif

SemaphoreHandle_t mutex_rtc;
SemaphoreHandle_t mutex_display;

#if TFT_CONTROLLER == 0 || TFT_CONTROLLER == 1 // ⏹⏹⏹⏹
// clang-format off
//
//  Display 320x240
//  +-------------------------------------------+ _yHeader=0
//  | Header                                    |       _hHeader=20px
//  +-------------------------------------------+ _yName=20
//  |                                           |
//  | Logo                   StationName        |       _hName=100px
//  |                                           |
//  +-------------------------------------------+ _yTitle=120
//  |                                           |
//  |              StreamTitle                  |       _hTitle=100px
//  |                                           |
//  +-------------------------------------------+ _yFooter=220
//  | Footer                                    |       _hFooter=20px
//  +-------------------------------------------+ 240
//                                             320

const uint8_t _fonts[9] = { 15, 16, 21, 25, 27, 34, 38, 43, 156};


struct w_h  {uint16_t x =   0; uint16_t y =   0; uint16_t w = 320; uint16_t h =  20;} const _winHeader;
struct w_l  {uint16_t x =   0; uint16_t y =  20; uint16_t w = 100; uint16_t h = 100;} const _winLogo;
struct w_n  {uint16_t x = 100; uint16_t y =  20; uint16_t w = 220; uint16_t h = 100;} const _winName;
struct w_e  {uint16_t x =   0; uint16_t y =  20; uint16_t w = 320; uint16_t h = 100;} const _winFName;
struct w_j  {uint16_t x =   0; uint16_t y = 120; uint16_t w = 100; uint16_t h =  46;} const _winFileNr;
struct w_t  {uint16_t x =   0; uint16_t y = 120; uint16_t w = 320; uint16_t h = 100;} const _winTitle;
struct w_c  {uint16_t x =   0; uint16_t y = 120; uint16_t w = 296; uint16_t h = 100;} const _winSTitle;
struct w_g  {uint16_t x = 296; uint16_t y = 120; uint16_t w =  24; uint16_t h = 100;} const _winVUmeter;
struct w_f  {uint16_t x =   0; uint16_t y = 220; uint16_t w = 320; uint16_t h =  20;} const _winFooter;
struct w_i  {uint16_t x =   0; uint16_t y =   0; uint16_t w = 180; uint16_t h =  20;} const _winItem;
struct w_v  {uint16_t x = 180; uint16_t y =   0; uint16_t w =  50; uint16_t h =  20;} const _winVolume;
struct w_m  {uint16_t x = 260; uint16_t y =   0; uint16_t w =  60; uint16_t h =  20;} const _winTime;
struct w_s  {uint16_t x =   0; uint16_t y = 220; uint16_t w =  60; uint16_t h =  20;} const _winStaNr;
struct w_p  {uint16_t x =  60; uint16_t y = 220; uint16_t w =  65; uint16_t h =  20;} const _winSleep;
struct w_r  {uint16_t x = 125; uint16_t y = 220; uint16_t w =  25; uint16_t h =  20;} const _winRSSID;
struct w_u  {uint16_t x = 150; uint16_t y = 220; uint16_t w =  40; uint16_t h =  20;} const _winBitRate;
struct w_a  {uint16_t x = 190; uint16_t y = 220; uint16_t w = 130; uint16_t h =  20;} const _winIPaddr;
struct w_b  {uint16_t x =   0; uint16_t y = 170; uint16_t w = 320; uint16_t h =   6;} const _winVolBar;
struct w_o  {uint16_t x =   0; uint16_t y = 180; uint16_t w =  40; uint16_t h =  40;} const _winButton;
struct w_d  {uint16_t x =   0; uint16_t y =  50; uint16_t w = 320; uint16_t h = 120;} const _winDigits;    // clock
struct w_y  {uint16_t x =   0; uint16_t y =  20; uint16_t w = 320; uint16_t h = 160;} const _winAlarm;
struct w_w  {uint16_t x =   0; uint16_t y =  20; uint16_t w = 320; uint16_t h = 200;} const _winWoHF;      // without Header and Footer
struct w_s1 {uint16_t x =  80; uint16_t y =  30; uint16_t w = 150; uint16_t h =  34;} const _sdrLP;        // slider lowpass in equalizer
struct w_s2 {uint16_t x =  80; uint16_t y =  64; uint16_t w = 150; uint16_t h =  34;} const _sdrBP;        // slider bandpass in equalizer
struct w_s3 {uint16_t x =  80; uint16_t y =  98; uint16_t w = 150; uint16_t h =  34;} const _sdrHP;        // slider highpass in equalizer
struct w_s4 {uint16_t x =  80; uint16_t y = 132; uint16_t w = 150; uint16_t h =  34;} const _sdrBAL;       // slider balance in equalizer

uint16_t _alarmdaysXPos[7] = {3, 48, 93, 138, 183, 228, 273};
uint16_t _alarmtimeXPos7S[5] = {2, 75, 148, 173, 246}; // seven segment digits
uint16_t _alarmtimeXPosFN[6] = {0, 56, 112, 152, 208, 264}; // folded numbers
uint16_t _sleeptimeXPos[5] = {5, 77, 129, 57}; // last is colon
uint8_t  _alarmdays_w = 44 + 4;
uint8_t  _alarmdays_h = 40;
uint16_t _dispWidth   = 320;
uint16_t _dispHeight  = 240;
uint8_t  _tftSize     = 0;
uint8_t  _irNumber_x  = 25;
uint8_t  _irNumber_y  = 40;
//
TFT tft(TFT_CONTROLLER, DISPLAY_INVERSION);
//
// clang-format on
#endif // TFT_CONTROLLER == 0 || TFT_CONTROLLER == 1

#if TFT_CONTROLLER == 2 || TFT_CONTROLLER == 3 || TFT_CONTROLLER == 4 || TFT_CONTROLLER == 5 || TFT_CONTROLLER == 6 // ⏹⏹⏹⏹
// clang-format off
//
//  Display 480x320
//  +-------------------------------------------+ _yHeader=0
//  | Header                                    |       _winHeader=30px
//  +-------------------------------------------+ _yName=30
//  |                                           |
//  | Logo                   StationName        |       _winFName=130px
//  |                                           |
//  +-------------------------------------------+ _yTitle=160
//  |                                           |
//  |              StreamTitle                  |       _winTitle=130px
//  |                                           |
//  +-------------------------------------------+ _yFooter=290
//  | Footer                                    |       _winFooter=30px
//  +-------------------------------------------+ 320
//                                             480

const uint8_t _fonts[9] = {21, 25, 27, 34, 38, 43, 56, 66, 156};

struct w_h  {uint16_t x =   0; uint16_t y =   0; uint16_t w = 480; uint16_t h =  30;} const _winHeader;
struct w_l  {uint16_t x =   0; uint16_t y =  30; uint16_t w = 130; uint16_t h = 132;} const _winLogo;
struct w_n  {uint16_t x = 130; uint16_t y =  30; uint16_t w = 350; uint16_t h = 132;} const _winName;
struct w_e  {uint16_t x =   0; uint16_t y =  30; uint16_t w = 480; uint16_t h = 132;} const _winFName;
struct w_j  {uint16_t x =   0; uint16_t y = 162; uint16_t w = 130; uint16_t h =  60;} const _winFileNr;
struct w_t  {uint16_t x =   0; uint16_t y = 162; uint16_t w = 480; uint16_t h = 128;} const _winTitle;
struct w_c  {uint16_t x =   0; uint16_t y = 162; uint16_t w = 448; uint16_t h = 128;} const _winSTitle;
struct w_g  {uint16_t x = 448; uint16_t y = 162; uint16_t w =  32; uint16_t h = 128;} const _winVUmeter;
struct w_f  {uint16_t x =   0; uint16_t y = 290; uint16_t w = 480; uint16_t h =  30;} const _winFooter;
struct w_m  {uint16_t x = 380; uint16_t y =   0; uint16_t w = 100; uint16_t h =  30;} const _winTime;
struct w_i  {uint16_t x =   0; uint16_t y =   0; uint16_t w = 280; uint16_t h =  30;} const _winItem;
struct w_v  {uint16_t x = 280; uint16_t y =   0; uint16_t w = 100; uint16_t h =  30;} const _winVolume;
struct w_s  {uint16_t x =   0; uint16_t y = 290; uint16_t w =  85; uint16_t h =  30;} const _winStaNr;
struct w_p  {uint16_t x =  85; uint16_t y = 290; uint16_t w =  87; uint16_t h =  30;} const _winSleep;
struct w_r  {uint16_t x = 172; uint16_t y = 290; uint16_t w =  32; uint16_t h =  30;} const _winRSSID;
struct w_u  {uint16_t x = 204; uint16_t y = 290; uint16_t w =  64; uint16_t h =  30;} const _winBitRate;
struct w_a  {uint16_t x = 268; uint16_t y = 290; uint16_t w = 212; uint16_t h =  30;} const _winIPaddr;
struct w_b  {uint16_t x =   0; uint16_t y = 222; uint16_t w = 480; uint16_t h =   8;} const _winVolBar;
struct w_o  {uint16_t x =   0; uint16_t y = 234; uint16_t w =  56; uint16_t h =  56;} const _winButton;
struct w_d  {uint16_t x =   0; uint16_t y =  70; uint16_t w = 480; uint16_t h = 160;} const _winDigits;
struct w_y  {uint16_t x =   0; uint16_t y =  30; uint16_t w = 480; uint16_t h = 200;} const _winAlarm;
struct w_w  {uint16_t x =   0; uint16_t y =  30; uint16_t w = 480; uint16_t h = 260;} const _winWoHF;      // without Header and Footer
struct w_s1 {uint16_t x = 140; uint16_t y =  30; uint16_t w = 200; uint16_t h =  50;} const _sdrLP;        // slider lowpass in equalizer
struct w_s2 {uint16_t x = 140; uint16_t y =  80; uint16_t w = 200; uint16_t h =  50;} const _sdrBP;        // slider bandpass in equalizer
struct w_s3 {uint16_t x = 140; uint16_t y = 130; uint16_t w = 200; uint16_t h =  50;} const _sdrHP;        // slider highpass in equalizer
struct w_s4 {uint16_t x = 140; uint16_t y = 180; uint16_t w = 200; uint16_t h =  50;} const _sdrBAL;       // slider balance in equalizer

uint16_t _alarmdaysXPos[7] = {2, 70, 138, 206, 274, 342, 410};
uint16_t _alarmtimeXPos7S[5] = {12, 118, 224, 266, 372}; // seven segment digits
uint16_t _alarmtimeXPosFN[6] = {16, 96, 176, 224, 304, 384}; // folded numbers
uint16_t _sleeptimeXPos[5] = {5, 107, 175, 73 };
uint8_t  _alarmdays_w = 64 + 4;
uint8_t  _alarmdays_h = 56;
uint16_t _dispWidth   = 480;
uint16_t _dispHeight  = 320;
uint8_t  _tftSize     = 1;
uint8_t  _irNumber_x  = 100;
uint8_t  _irNumber_y  = 80;
//
TFT tft(TFT_CONTROLLER, DISPLAY_INVERSION);
//
// clang-format on
#endif // #if TFT_CONTROLLER == 2 || TFT_CONTROLLER == 3 || TFT_CONTROLLER == 4 || TFT_CONTROLLER == 5|| TFT_CONTROLLER == 6

// RADIO
button2state btn_R_Mute("btn_R_Mute");
button1state btn_R_volDown("btn_R_volDown"), btn_R_volUp("btn_R_volUp"), btn_R_prevSta("btn_R_prevSta"), btn_R_nextSta("btn_R_nextSta");
button1state btn_R_staList("btn_R_staList"), btn_R_player("btn_R_player"), btn_R_dlna("btn_R_dlna"), btn_R_clock("btn_R_clock");
button1state btn_R_sleep("btn_R_sleep"), btn_R_bright("btn_R_bright"), btn_R_equal("btn_R_equal");
// PLAYER
button2state btn_P_Mute("btn_P_Mute"), btn_P_pause("btn_P_pause");
button1state btn_P_volDown("btn_P_volDown"), btn_P_volUp("btn_P_volUp"), btn_P_ready("btn_P_ready"), btn_P_shuffle("btn_P_shuffle");
button1state btn_P_playAll("btn_P_playAll"), btn_P_fileList("btn_P_fileList"), btn_P_radio("btn_P_radio"), btn_P_cancel("btn_P_cancel");
button1state btn_P_prevFile("btn_P_prevFile"), btn_P_nextFile("btn_P_nextFile");
// DLNA
button2state btn_D_Mute("btn_D_Mute"), btn_D_pause("btn_D_pause");
button1state btn_D_volDown("btn_D_volDown"), btn_D_volUp("btn_D_volUp");
button1state btn_D_radio("btn_D_radio"), btn_D_fileList("btn_D_fileList"), btn_D_cancel("btn_D_cancel");
// CLOCK
imgClock     clk_C_green("clk_C_green");
button2state btn_C_Mute("btn_C_Mute");
button1state btn_C_alarm("btn_C_alarm"), btn_C_radio("btn_C_radio"), btn_C_volDown("btn_C_volDown"), btn_C_volUp("btn_C_volUp");
// ALARM
alarmClock   clk_A_red("clk_C_green");
button1state btn_A_left("btn_A_left"), btn_A_right("btn_A_right"), btn_A_up("btn_A_up"), btn_A_down("btn_A_down");
button1state btn_A_ready("btn_A_ready");
// EQUALIZER
slider       sdr_E_lowPass("sdr_E_LP"), sdr_E_bandPass("sdr_E_BP"), sdr_E_highPass("sdr_E_HP"), sdr_E_balance("sdr_E_BAL");
textbox      txt_E_lowPass("txt_E_LP"), txt_E_bandPass("txt_E_BP"), txt_E_highPass("txt_E_HP"), txt_E_balance("txt_E_BAL");
button1state btn_E_lowPass("btn_E_LP");
button1state btn_E_bandPass("btn_E_BP"), btn_E_highPass("btn_E_HP"), btn_E_balance("btn_E_BAL");
button1state btn_E_Radio("btn_E_Radio"), btn_E_Player("btn_E_Player");
button2state btn_E_Mute("btn_E_Mute");

/*  ╔═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
    ║                                                     D E F A U L T S E T T I N G S                                                         ║
    ╚═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝   */

// clang-format off
boolean defaultsettings(){
    if(!SD_MMC.exists("/settings.json")){
        File file = SD_MMC.open("/settings.json","w", true);
        char*  jO = x_ps_malloc(1024); // JSON Object
        strcpy(jO, "{");
        strcat(jO, "\"volume\":");            strcat(jO, "12,"); // 0...21
        strcat(jO, "\"ringvolume\":");        strcat(jO, "21,");
        strcat(jO, "\"alarmtime_sun\":");     strcat(jO, "00:00,");
        strcat(jO, "\"alarmtime_mon\":");     strcat(jO, "00:00,");
        strcat(jO, "\"alarmtime_tue\":");     strcat(jO, "00:00,");
        strcat(jO, "\"alarmtime_wed\":");     strcat(jO, "00:00,");
        strcat(jO, "\"alarmtime_fri\":");     strcat(jO, "00:00,");
        strcat(jO, "\"alarmtime_sat\":");     strcat(jO, "00:00,");
        strcat(jO, "\"alarm_weekdays\":");    strcat(jO, "0,");
        strcat(jO, "\"timeAnnouncing\":");    strcat(jO, "\"true\",");
        strcat(jO, "\"mute\":");              strcat(jO, "\"false\","); // no mute
        strcat(jO, "\"brightness\":");        strcat(jO, "100,");  // 0...100
        strcat(jO, "\"sleeptime\":");         strcat(jO, "0,");
        strcat(jO, "\"lastconnectedhost\":"); strcat(jO, "\"\",");
        strcat(jO, "\"station\":");           strcat(jO, "1,");
        strcat(jO, "\"sumstations\":");       strcat(jO, "0,");
        strcat(jO, "\"Timezone_Name\":");     strcat(jO, "\"Europe/Berlin\",");
        strcat(jO, "\"Timezone_String\":");   strcat(jO, "\"CET-1CEST,M3.5.0,M10.5.0/3\",");
        strcat(jO, "\"toneLP\":");            strcat(jO, "0,"); // -40 ... +6 (dB)        audioI2S
        strcat(jO, "\"toneBP\":");            strcat(jO, "0,"); // -40 ... +6 (dB)        audioI2S
        strcat(jO, "\"toneHP\":");            strcat(jO, "0,"); // -40 ... +6 (dB)        audioI2S
        strcat(jO, "\"balance\":");           strcat(jO, "0,"); // -16 ... +16            audioI2S
        strcat(jO, "\"timeFormat\":");        strcat(jO, "24}");
        file.print(jO);
        if(jO){free(jO); jO = NULL;}
    }

    File file = SD_MMC.open("/settings.json","r", false);
    char*  jO = x_ps_calloc(1024, 1);
    char* tmp = x_ps_malloc(512);
    file.readBytes(jO, 1024);
    _settingsHash = simpleHash(jO);

    auto parseJson = [&](const char* s) { // lambda, inner function
        int16_t pos1 = 0, pos2 = 0, pos3 = 0;
        pos1 = indexOf(jO, s, 0);
        pos2 = indexOf(jO, ":", pos1) + 1;
        pos3 = indexOf(jO, ",\"", pos2);
        if(pos3 < 0) pos3 = indexOf(jO, "}", pos2);
        if(pos1 < 0) {log_e("index %s not found", s); return "";}
        if(jO[pos2] == '\"'){pos2++; pos3--;}  // remove \" embraced strings
        strncpy(tmp, jO + pos2, pos3 - pos2);
        tmp[pos3 - pos2] = '\0';
        return (const char*)tmp;
    };

    auto computeMinuteOfTheDay = [&](const char* s){
        if(!s) return 0;
        int h = atoi(s);
        int m = atoi(s + 3);
        return h * 60 + m;
    };

    _cur_volume          = atoi(   parseJson("\"volume\":"));
    _ringvolume          = atoi(   parseJson("\"ringvolume\":"));
    _alarmtime[0]        = computeMinuteOfTheDay(parseJson("\"alarmtime_sun\":"));
    _alarmtime[1]        = computeMinuteOfTheDay(parseJson("\"alarmtime_mon\":"));
    _alarmtime[2]        = computeMinuteOfTheDay(parseJson("\"alarmtime_tue\":"));
    _alarmtime[3]        = computeMinuteOfTheDay(parseJson("\"alarmtime_wed\":"));
    _alarmtime[4]        = computeMinuteOfTheDay(parseJson("\"alarmtime_thu\":"));
    _alarmtime[5]        = computeMinuteOfTheDay(parseJson("\"alarmtime_fri\":"));
    _alarmtime[6]        = computeMinuteOfTheDay(parseJson("\"alarmtime_sat\":"));
    _alarmdays           = atoi(   parseJson("\"alarm_weekdays\":"));
    _f_timeAnnouncement  = (strcmp(parseJson("\"timeAnnouncing\":"), "true") == 0) ? 1 : 0;
    _f_mute              = (strcmp(parseJson("\"mute\":"), "true") == 0) ? 1 : 0;
    _brightness          = atoi(   parseJson("\"brightness\":"));
    _sleeptime           = atoi(   parseJson("\"sleeptime\":"));
    _cur_station         = atoi(   parseJson("\"station\":"));
    _sum_stations        = atoi(   parseJson("\"sumstations\":"));
    _toneLP              = atoi(   parseJson("\"toneLP\":"));
    _toneBP              = atoi(   parseJson("\"toneBP\":"));
    _toneHP              = atoi(   parseJson("\"toneHP\":"));
    _toneBAL             = atoi(   parseJson("\"balance\":"));
    _timeFormat          = atoi(   parseJson("\"timeFormat\":"));
    _TZName              =         parseJson("\"Timezone_Name\":");
    _TZString            =         parseJson("\"Timezone_String\":");
    _lastconnectedhost   =         parseJson("\"lastconnectedhost\":");


    if(!pref.isKey("stations_filled")|| _sum_stations == 0) saveStationsToNVS();  // first init
    if(pref.getShort("IR_numButtons", 0) == 0) saveDefaultIRbuttonsToNVS();
    loadIRbuttonsFromNVS();

    if(jO) {free(jO);   jO = NULL;}
    if(tmp){free(tmp); tmp = NULL;}
    return true;
}
// clang-format on

boolean saveStationsToNVS() {
    String   Hide = "", Cy = "", StationName = "", StreamURL = "", currentLine = "", tmp = "";
    uint16_t cnt = 0;
    // StationList
    if(!SD_MMC.exists("/stations.csv")) {
        SerialPrintfln(ANSI_ESC_RED "SD_MMC/stations.csv not found");
        return false;
    }

    File file = SD_MMC.open("/stations.csv");
    if(file) { // try to read from SD_MMC
        stations.clear();
        currentLine = file.readStringUntil('\n'); // read the headline
        while(file.available()) {
            currentLine = file.readStringUntil('\n'); // read the line
            uint p = 0, q = 0;
            Hide = "";
            Cy = "";
            StationName = "";
            StreamURL = "";
            for(int32_t i = 0; i < currentLine.length() + 1; i++) {
                if(currentLine[i] == '\t' || i == currentLine.length()) {
                    if(p == 0) Hide = currentLine.substring(q, i);
                    if(p == 1) Cy = currentLine.substring(q, i);
                    if(p == 2) StationName = currentLine.substring(q, i);
                    if(p == 3) StreamURL = currentLine.substring(q, i);
                    p++;
                    i++;
                    q = i;
                }
            }
            if(Hide == "*") continue;
            if(StationName == "") continue; // is empty
            if(StreamURL == "") continue;   // is empty
            SerialPrintfln("Cy=%s, StationName=%s, StreamURL=%s", Cy.c_str(), StationName.c_str(), StreamURL.c_str());
            cnt++;
            if(cnt == _max_stations) {
                SerialPrintfln(ANSI_ESC_RED "No more than %d entries in stationlist allowed!", _max_stations);
                cnt--; // maxstations 999
                break;
            }
            tmp = StationName + "#" + StreamURL;
            sprintf(_chbuf, "station_%03d", cnt);
            stations.putString(_chbuf, tmp);
        }
        _sum_stations = cnt;
        stations.putLong("stations.size", file.size());
        file.close();
        pref.putBool("stations_filled", true);
        SerialPrintfln("stationlist internally loaded");
        SerialPrintfln("number of stations: " ANSI_ESC_CYAN "%i", cnt);
        return true;
    }
    else return false;
}

boolean saveDefaultIRbuttonsToNVS() { // default values, first init
    pref.putShort("irAddress", 0x00);
    pref.putShort("button_0", 0x52);  // '0';
    pref.putShort("button_1", 0x16);  // '1';
    pref.putShort("button_2", 0x19);  // '2';
    pref.putShort("button_3", 0x0D);  // '3';
    pref.putShort("button_4", 0x0C);  // '4';
    pref.putShort("button_5", 0x18);  // '5';
    pref.putShort("button_6", 0x5E);  // '6';
    pref.putShort("button_7", 0x08);  // '7';
    pref.putShort("button_8", 0x1C);  // '8';
    pref.putShort("button_9", 0x5A);  // '9';
    pref.putShort("button_10", 0x40); // 'm';  // MUTE
    pref.putShort("button_11", 0x46); // 'u';  // VOLUME+
    pref.putShort("button_12", 0x15); // 'd';  // VOLUME-
    pref.putShort("button_13", 0x43); // 'p';  // PREVIOUS STATION
    pref.putShort("button_14", 0x44); // 'n';  // NEXT STATION
    pref.putShort("button_15", 0x4A); // 'k';  // CLOCK <--> RADIO
    pref.putShort("button_16", 0x42); // 's';  // OFF TIMER
    pref.putShort("button_17", 0x00); // '0';
    pref.putShort("button_18", 0x00); // '0';
    pref.putShort("button_19", 0x00); // '0';

    pref.putShort("IR_numButtons", 20);
    // log_i("saveDefaultIRbuttonsToNVS");

    loadIRbuttonsFromNVS();

    return true;
}

void saveIRbuttonsToNVS() {
    uint8_t  ir_addr = ir.get_irAddress();
    uint8_t* ir_buttons = ir.get_irButtons();
    char     buf[12];
    pref.putShort("irAddress", ir_addr);
    for(uint8_t i = 0; i < 20; i++) {
        sprintf(buf, "button_%d", i);
        pref.putShort(buf, ir_buttons[i]);
        log_i("i=%i ir_buttons[i] %X", i, ir_buttons[i]);
    }
    pref.putShort("IR_numButtons", 20);
}

void loadIRbuttonsFromNVS() {
    // load IR settings from NVS
    uint numButtons = pref.getShort("IR_numButtons", 0);
    ir.set_irAddress(pref.getShort("irAddress", 0));
    char    buf[12];
    uint8_t cmd = 0;
    for(uint i = 0; i < numButtons; i++) {
        sprintf(buf, "button_%d", i);
        cmd = pref.getShort(buf, 0);
        ir.set_irButtons(i, cmd);
    }
}

// clang-format off
void updateSettings(){
    if(!_lastconnectedhost)_lastconnectedhost = "";
    char*  jO = x_ps_malloc(1024 + _lastconnectedhost.length()); // JSON Object
    char tmp[40 + _lastconnectedhost.length()];
    strcpy(jO, "{");
    sprintf(tmp,  "\"volume\":%i", _cur_volume);                                            strcat(jO, tmp);
    sprintf(tmp, ",\"ringvolume\":%i", _ringvolume);                                        strcat(jO, tmp);
    sprintf(tmp, ",\"alarmtime_sun\":%02d:%02d", _alarmtime[0] / 60, _alarmtime[0] % 60);   strcat(jO, tmp);
    sprintf(tmp, ",\"alarmtime_mon\":%02d:%02d", _alarmtime[1] / 60, _alarmtime[1] % 60);   strcat(jO, tmp);
    sprintf(tmp, ",\"alarmtime_tue\":%02d:%02d", _alarmtime[2] / 60, _alarmtime[2] % 60);   strcat(jO, tmp);
    sprintf(tmp, ",\"alarmtime_wed\":%02d:%02d", _alarmtime[3] / 60, _alarmtime[3] % 60);   strcat(jO, tmp);
    sprintf(tmp, ",\"alarmtime_thu\":%02d:%02d", _alarmtime[4] / 60, _alarmtime[4] % 60);   strcat(jO, tmp);
    sprintf(tmp, ",\"alarmtime_fri\":%02d:%02d", _alarmtime[5] / 60, _alarmtime[5] % 60);   strcat(jO, tmp);
    sprintf(tmp, ",\"alarmtime_sat\":%02d:%02d", _alarmtime[6] / 60, _alarmtime[6] % 60);   strcat(jO, tmp);
    sprintf(tmp, ",\"alarm_weekdays\":%i", _alarmdays);                                     strcat(jO, tmp);
    strcat(jO,   ",\"timeAnnouncing\":"); (_f_timeAnnouncement == true) ?                   strcat(jO, "\"true\"") : strcat(jO, "\"false\"");
    strcat(jO,   ",\"mute\":");           (_f_mute == true)             ?                   strcat(jO, "\"true\"") : strcat(jO, "\"false\"");
    sprintf(tmp, ",\"brightness\":%i", _brightness);                                        strcat(jO, tmp);
    sprintf(tmp, ",\"sleeptime\":%i", _sleeptime);                                          strcat(jO, tmp);
    sprintf(tmp, ",\"lastconnectedhost\":\"%s\"", _lastconnectedhost.c_str());              strcat(jO, tmp);
    sprintf(tmp, ",\"station\":%i", _cur_station);                                          strcat(jO, tmp);
    sprintf(tmp, ",\"sumstations\":%i", _sum_stations);                                     strcat(jO, tmp);
    sprintf(tmp, ",\"Timezone_Name\":\"%s\"", _TZName.c_str());                             strcat(jO, tmp);
    sprintf(tmp, ",\"Timezone_String\":\"%s\"", _TZString.c_str());                         strcat(jO, tmp);
    sprintf(tmp, ",\"toneLP\":%i", _toneLP);                                                strcat(jO, tmp);
    sprintf(tmp, ",\"toneBP\":%i", _toneBP);                                                strcat(jO, tmp);
    sprintf(tmp, ",\"toneHP\":%i", _toneHP);                                                strcat(jO, tmp);
    sprintf(tmp, ",\"balance\":%i", _toneBAL);                                              strcat(jO, tmp);
    sprintf(tmp, ",\"timeFormat\":%i}", _timeFormat);                                       strcat(jO, tmp);

    if(_settingsHash != simpleHash(jO)) {
        File file = SD_MMC.open("/settings.json", "w", false);
        if(!file) {
            log_e("file \"settings.json\" not found");
            return;
        }
        file.print(jO);
        _settingsHash = simpleHash(jO);
    }
    if(jO){free(jO); jO = NULL;}
}
// clang-format on

/*****************************************************************************************************************************************************
 *                                                    F I L E   E X P L O R E R                                                                      *
 *****************************************************************************************************************************************************/
// Sends a list of the content of a directory as JSON file
const char* SD_stringifyDirContent(String path) {
    uint16_t JSONstrLength = 0;
    uint8_t  isDir = 0;
    uint16_t fnLen = 0; // length of file mame
    uint8_t  fsLen = 0; // length of file size
    if(_JSONstr) {
        free(_JSONstr);
        _JSONstr = NULL;
    }
    if(!SD_listDir(path.c_str(), false, false)) return "[]"; // if success: result will be in _SD_content
    if(psramFound()) { _JSONstr = (char*)ps_malloc(2); }
    else { _JSONstr = (char*)malloc(2); }
    JSONstrLength += 2;
    memcpy(_JSONstr, "[\0", 2);
    if(!_SD_content.size()) return "[]"; // empty?

    for(int i = 0; i < _SD_content.size(); i++) { // build a JSON string in PSRAM, e.g. [{"name":"m","dir":true},{"name":"s","dir":true}]
        const char* fn = _SD_content[i];
        if(startsWith(fn, "/.")) continue;    // ignore hidden folders
        int16_t idx = indexOf(fn, "\033", 1); // idx >0 we have size (after ANSI ESC SEQUENCE)
        if(idx > 0) {
            isDir = 0;
            fnLen = idx;
            fsLen = strlen(fn) - (idx + 6);          // "033[33m"
            JSONstrLength += fnLen + 24 + 8 + fsLen; // {"name":"test.mp3","dir":false,"size":"3421"}
        }
        else {
            isDir = 1;
            fnLen = strlen(fn);
            fsLen = 0;
            JSONstrLength += fnLen + 23 + 11;
        }
        if(psramFound()) { _JSONstr = (char*)ps_realloc(_JSONstr, JSONstrLength); }
        else { _JSONstr = (char*)realloc(_JSONstr, JSONstrLength); }

        strcat(_JSONstr, "{\"name\":\"");
        strncat(_JSONstr, fn, fnLen);
        strcat(_JSONstr, "\",\"dir\":");
        if(isDir) { strcat(_JSONstr, "true"); }
        else { strcat(_JSONstr, "false"); }
        if(!isDir) {
            strcat(_JSONstr, ",\"size\":");
            strncat(_JSONstr, fn + idx + 6, fsLen);
        }
        else { strcat(_JSONstr, ",\"size\": \"\""); }
        strcat(_JSONstr, "},");
    }
    _JSONstr[JSONstrLength - 2] = ']'; // replace comma by square bracket close
    return _JSONstr;
}

/*****************************************************************************************************************************************************
 *                                                    T F T   B R I G H T N E S S                                                                    *
 *****************************************************************************************************************************************************/
void setTFTbrightness(uint8_t duty) { // duty 0...100 (min...max)
    if(TFT_BL == -1) return;
    uint8_t d = round((double)duty * 2.55); // #186
    ledcWrite(TFT_BL, d);
}

inline uint8_t downBrightness() {
    if(_brightness > 5) {
        _brightness -= 5;
        setTFTbrightness(_brightness);
        showBrightnessBar();
        log_i("br %i", _brightness);
    }
    return _brightness;
}

inline uint8_t upBrightness() {
    if(_brightness < 100) {
        _brightness += 5;
        setTFTbrightness(_brightness);
        showBrightnessBar();
        log_i("br %i", _brightness);
    }
    return _brightness;
}
inline uint8_t getBrightness() { return _brightness; }

/*****************************************************************************************************************************************************
 *                                                      U R L d e c o d e                                                                            *
 *****************************************************************************************************************************************************/
// In m3u playlists, file names can be URL encoded.
// Since UTF-8 is always shorter than URI, the same memory is used for decoding
// e.g. Born%20On%20The%20B.mp3 --> Born On The B.mp3
// e.g. %D0%B8%D1%81%D0%BF%D1%8B%D1%82%D0%B0%D0%BD%D0%B8%D0%B5.mp3 --> испытание.mp3
void urldecode(char* str) {
    uint16_t p1 = 0, p2 = 0;
    char     a, b;
    while(str[p1]) {
        if((str[p1] == '%') && ((a = str[p1 + 1]) && (b = str[p1 + 2])) && (isxdigit(a) && isxdigit(b))) {
            if(a >= 'a') a -= 'a' - 'A';
            if(a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if(b >= 'a') b -= 'a' - 'A';
            if(b >= 'A') b -= ('A' - 10);
            else b -= '0';
            str[p2++] = 16 * a + b;
            p1 += 3;
        }
        else if(str[p1] == '+') {
            str[p2++] = ' ';
            p1++;
        }
        else { str[p2++] = str[p1++]; }
    }
    str[p2++] = '\0';
}

/*****************************************************************************************************************************************************
 *                                                               T I M E R                                                                           *
 *****************************************************************************************************************************************************/

// clang-format off
void timer100ms(){
    static uint16_t ms100 = 0;
    _f_100ms = true;
    ms100 ++;
    if(!(ms100 % 10))   _f_1sec  = true;
    if(!(ms100 % 100))  _f_10sec = true;
    if(!(ms100 % 600)) {_f_1min  = true; ms100 = 0;}

}
// clang-format on

/*****************************************************************************************************************************************************
 *                                                               D I S P L A Y                                                                       *
 *****************************************************************************************************************************************************/

// clang-format off
inline void clearHeader()             {tft.fillRect(_winHeader.x,    _winHeader.y,    _winHeader.w,    _winHeader.h,   TFT_BLACK);}
inline void clearLogo()               {tft.fillRect(_winLogo.x,      _winLogo.y,      _winLogo.w,      _winLogo.h,     TFT_BLACK);}
inline void clearStationName()        {tft.fillRect(_winName.x,      _winName.y,      _winName.w,      _winName.h,     TFT_BLACK);}
inline void clearLogoAndStationname() {tft.fillRect(_winFName.x,     _winFName.y,     _winFName.w,     _winFName.h,    TFT_BLACK);}
inline void clearTitle()              {tft.fillRect(_winTitle.x,     _winTitle.y,     _winTitle.w,     _winTitle.h,    TFT_BLACK);} // incl. VUmeter
inline void clearStreamTitle()        {tft.fillRect(_winSTitle.x,    _winSTitle.y,    _winSTitle.w,    _winSTitle.h,   TFT_BLACK);} // without VUmeter
inline void clearWithOutHeaderFooter(){tft.fillRect(_winWoHF.x,      _winWoHF.y,      _winWoHF.w,      _winWoHF.h,     TFT_BLACK);}
inline void clearFooter()             {tft.fillRect(_winFooter.x,    _winFooter.y,    _winFooter.w,    _winFooter.h,   TFT_BLACK);}
inline void clearTime()               {tft.fillRect(_winTime.x,      _winTime.y,      _winTime.w,      _winTime.h,     TFT_BLACK);}
inline void clearItem()               {tft.fillRect(_winItem.x,      _winItem.y,      _winItem.w,      _winTime.h,     TFT_BLACK);}
inline void clearVolume()             {tft.fillRect(_winVolume.x,    _winVolume.y,    _winVolume.w,    _winVolume.h,   TFT_BLACK);}
inline void clearIPaddr()             {tft.fillRect(_winIPaddr.x,    _winIPaddr.y,    _winIPaddr.w,    _winIPaddr.h,   TFT_BLACK);}
inline void clearBitRate()            {tft.fillRect(_winBitRate.x,   _winBitRate.y,   _winBitRate.w,   _winBitRate.h,  TFT_BLACK);}
inline void clearStaNr()              {tft.fillRect(_winStaNr.x,     _winStaNr.y,     _winStaNr.w,     _winStaNr.h,    TFT_BLACK);}
inline void clearSleep()              {tft.fillRect(_winSleep.x,     _winSleep.y,     _winSleep.w,     _winSleep.h,    TFT_BLACK);}
inline void clearVolBar()             {tft.fillRect(_winVolBar.x,    _winVolBar.y,    _winVolBar.w,    _winVolBar.h,   TFT_BLACK);}
inline void clearDigits()             {tft.fillRect(_winDigits.x,    _winDigits.y,    _winDigits.w,    _winDigits.h,   TFT_BLACK);}
inline void clearButtonBar()          {tft.fillRect( 0,              _winButton.y,    _dispWidth,      _winButton.h,   TFT_BLACK);}
inline void clearAll()                {tft.fillScreen(TFT_BLACK);}                      // y   0...239
// clang-format on

inline uint16_t txtlen(String str) {
    uint16_t len = 0;
    for(int32_t i = 0; i < str.length(); i++)
        if(str[i] <= 0xC2) len++;
    return len;
}

void showHeadlineVolume() {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    tft.setFont(_fonts[1]);
    if(_f_mute) tft.setTextColor(TFT_RED);
    else        tft.setTextColor(TFT_DEEPSKYBLUE);
    clearVolume();
    sprintf(_chbuf, "Vol %02d", _cur_volume);
    tft.writeText(_chbuf, _winVolume.x + 6, _winVolume.y, _winVolume.w, _winVolume.h);
    xSemaphoreGive(mutex_display);
}
void showHeadlineTime(bool complete) {
    static char oldtime[8]; // hhmmss
    char        newtime[8] = {255, 255, 255, 255, 255, 255, 255, 255};
    uint8_t     pos_s[8] = {0, 9, 18, 21, 30, 39, 42, 51};  // display 320x240
    uint8_t     pos_m[8] = {7, 20, 33, 40, 53, 66, 73, 86}; // display 480x320
    uint8_t*    pos = NULL;
    uint8_t     w = 0;
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    tft.setFont(_fonts[1]);
    tft.setTextColor(TFT_GREENYELLOW);
    if(!_f_rtc) {
        xSemaphoreGive(mutex_display);
        clearTime();
        return;
    } // has rtc the correct time? no -> return
    memcpy(newtime, rtc.gettime_s(), 8);
    if(complete == true) {
        clearTime();
        for(uint8_t i = 0; i < 8; i++) { oldtime[i] = 255; }
    }
    for(uint8_t i = 0; i < 8; i++) {
        if(oldtime[i] != newtime[i]) {
            char ch[2] = {0, 0};
            ch[0] = newtime[i];
            if(TFT_CONTROLLER < 2) { // 320x240
                pos = pos_s;
                w = 9;
            }
            else { // 480x320
                pos = pos_m;
                w = 13;
            }
            tft.fillRect(_winTime.x + pos[i], _winTime.y, w, _winTime.h, TFT_BLACK);
            tft.writeText(ch, _winTime.x + pos[i], _winTime.y, w, _winTime.h, TFT_ALIGN_LEFT, true);
            oldtime[i] = newtime[i];
        }
    }

    xSemaphoreGive(mutex_display);
}
void showHeadlineItem(uint8_t idx) { // radio, clock, audioplayer...
    if(_f_sleeping) return;
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    tft.setFont(_fonts[1]);
    tft.setTextColor(TFT_GREENYELLOW);
    clearItem();
    tft.writeText(_hl_item[idx], _winItem.x + 6, _winItem.y, _winItem.w, _winItem.h);
    xSemaphoreGive(mutex_display);
}
void showFooterIPaddr() {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    char myIP[30] = "IP:";
    strcpy(myIP + 3, _myIP);
    tft.setFont(_fonts[1]);
    tft.setTextColor(TFT_GREENYELLOW);
    clearIPaddr();
    tft.writeText(myIP, _winIPaddr.x, _winIPaddr.y, _winIPaddr.w, _winIPaddr.h, TFT_ALIGN_RIGHT, true);
    xSemaphoreGive(mutex_display);
}
void showFooterStaNr() {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    uint8_t offset = 0;
    if(TFT_CONTROLLER < 2) offset = 25;
    else offset = 33;
    clearStaNr();
    drawImage("/common/STA.bmp", _winStaNr.x, _winStaNr.y);
    tft.setFont(_fonts[1]);
    tft.setTextColor(TFT_LAVENDER);
    char buf[10];
    sprintf(buf, "%03d", _cur_station);
    tft.writeText(buf, _winStaNr.x + offset, _winStaNr.y, _winStaNr.w, _winStaNr.h);
    xSemaphoreGive(mutex_display);
}
void showFooterRSSI(boolean show) {
    static int32_t old_rssi = -1;
    int32_t        new_rssi = -1;
    int8_t         rssi = WiFi.RSSI(); // Received Signal Strength Indicator
    if(rssi < -1) new_rssi = 4;
    if(rssi < -50) new_rssi = 3;
    if(rssi < -65) new_rssi = 2;
    if(rssi < -75) new_rssi = 1;
    if(rssi < -85) new_rssi = 0;

    if(new_rssi != old_rssi) {
        old_rssi = new_rssi; // no need to draw a rssi icon if rssiRange has not changed
        if(ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO) {
            static int32_t tmp_rssi = 0;
            if((abs(rssi - tmp_rssi) > 3)) { SerialPrintfln("WiFI_info:   RSSI is " ANSI_ESC_CYAN "%d" ANSI_ESC_WHITE " dB", rssi); }
            tmp_rssi = rssi;
        }
        show = true;
    }
    if(show && !_timeCounter.timer) {
        switch(new_rssi) {
            case 4: {
                drawImage("/common/RSSI4.bmp", _winRSSID.x, _winRSSID.y + 2);
                break;
            }
            case 3: {
                drawImage("/common/RSSI3.bmp", _winRSSID.x, _winRSSID.y + 2);
                break;
            }
            case 2: {
                drawImage("/common/RSSI2.bmp", _winRSSID.x, _winRSSID.y + 2);
                break;
            }
            case 1: {
                drawImage("/common/RSSI1.bmp", _winRSSID.x, _winRSSID.y + 2);
                break;
            }
            case 0: {
                drawImage("/common/RSSI0.bmp", _winRSSID.x, _winRSSID.y + 2);
                break;
            }
        }
    }
}

void showFooterBitRate(uint16_t br) {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    clearBitRate();
    char sbr[10];
    itoa(br, sbr, 10);
    if(br < 1000) { strcat(sbr, "K"); }
    else {
        sbr[2] = sbr[1];
        sbr[1] = '.';
        sbr[3] = 'M';
        sbr[4] = '\0';
    }
    tft.setFont(_fonts[1]);
    tft.setTextColor(TFT_LAVENDER);
    uint8_t space = 2;
    if(strlen(sbr) < 4) space += 5;
    tft.writeText(sbr, _winBitRate.x + space, _winBitRate.y, _winBitRate.w, _winBitRate.h);
    xSemaphoreGive(mutex_display);
}

void updateSleepTime(boolean noDecrement) { // decrement and show new value in footer
    if(_f_sleeping) return;
    boolean sleep = false;
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    clearSleep();
    drawImage("/common/Hourglass_blue.bmp", _winSleep.x, _winSleep.y);
    uint8_t offset = 0;
    if(TFT_CONTROLLER < 2) offset = 28;
    else offset = 33;
    if(_sleeptime == 1) sleep = true;
    if(_sleeptime > 0 && !noDecrement) _sleeptime--;

    char Slt[15];
    sprintf(Slt, "%d:%02d", _sleeptime / 60, _sleeptime % 60);
    tft.setFont(_fonts[1]);
    if(!_sleeptime) {
        drawImage("/common/Hourglass_blue.bmp", _winSleep.x, _winSleep.y);
        tft.setTextColor(TFT_DEEPSKYBLUE);
    }
    else {
        drawImage("/common/Hourglass_red.bmp", _winSleep.x, _winSleep.y);
        tft.setTextColor(TFT_RED);
    }
    tft.writeText(Slt, _winSleep.x + offset, _winSleep.y, _winSleep.w, _winSleep.h);

    xSemaphoreGive(mutex_display);
    if(sleep) { // fall asleep
        fall_asleep();
        _sleeptime = 0;
    }
}
void showVolumeBar() {
    uint16_t val = tft.width() * _cur_volume / 21;
    clearVolBar();
    tft.fillRect(_winVolBar.x, _winVolBar.y + 1, val, _winVolBar.h - 2, TFT_RED);
    tft.fillRect(val + 1, _winVolBar.y + 1, tft.width() - val + 1, _winVolBar.h - 2, TFT_GREEN);
    _f_volBarVisible = true;
}

void updateVUmeter() {
    if(_state != RADIO) return;
    if(_radioSubmenue > 0) return;
    if(!_f_VUmeterIsVisible) return;
    if(_f_sleeping) return;
    if(_f_irNumberSeen) return;
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    uint8_t width = 0, height = 0, xOffs = 0, yOffs = 0, xStart = 0, yStart = 0;
#if TFT_CONTROLLER < 2 // 320 x 240px
    width = 9;
    height = 7;
    xOffs = 11;
    yOffs = 8;
    xStart = 2;
    yStart = 90;
#else // 480 x 320px
    width = 12;
    height = 8;
    xOffs = 16;
    yOffs = 10;
    xStart = 2;
    yStart = 115;
#endif

    // c99 has no inner functions, lambdas are only allowed from c11, please don't use ancient compiler
    auto drawRect = [&](uint8_t pos, uint8_t ch, bool br) { // lambda, inner function
        uint16_t color = 0, xPos = _winVUmeter.x + xStart + ch * xOffs, yPos = _winVUmeter.y + yStart - pos * yOffs;
        if(pos > 11) return;
        switch(pos) {
            case 0 ... 6: // green
                br ? color = TFT_GREEN : color = TFT_DARKGREEN;
                break;
            case 7 ... 9: // yellow
                br ? color = TFT_YELLOW : color = TFT_DARKYELLOW;
                break;
            case 10 ... 11: // red
                br ? color = TFT_RED : color = TFT_DARKRED;
                break;
        }
        tft.fillRect(xPos, yPos, width, height, color);
    };

    uint16_t vum = audioGetVUlevel();

    uint8_t left = map_l(vum >> 8, 0, 127, 0, 11);
    uint8_t right = map_l(vum & 0x00FF, 0, 127, 0, 11);

    if(left > _VUleftCh) {
        for(int32_t i = _VUleftCh; i < left; i++) { drawRect(i, 1, 1); }
    }
    if(left < _VUleftCh) {
        for(int32_t i = left; i < _VUleftCh; i++) { drawRect(i, 1, 0); }
    }
    _VUleftCh = left;

    if(right > _VUrightCh) {
        for(int32_t i = _VUrightCh; i < right; i++) { drawRect(i, 0, 1); }
    }
    if(right < _VUrightCh) {
        for(int32_t i = right; i < _VUrightCh; i++) { drawRect(i, 0, 0); }
    }
    _VUrightCh = right;
    xSemaphoreGive(mutex_display);
}

void showBrightnessBar() {
    uint16_t val = tft.width() * getBrightness() / 100;
    clearVolBar();
    tft.fillRect(_winVolBar.x, _winVolBar.y + 1, val, _winVolBar.h - 2, TFT_RED);
    tft.fillRect(val + 1, _winVolBar.y + 1, tft.width() - val + 1, _winVolBar.h - 2, TFT_GREEN);
    _f_volBarVisible = true;
}
void showFooter() { // stationnumber, sleeptime, IPaddress
    showFooterStaNr();
    updateSleepTime(true);
    showFooterIPaddr();
    showFooterRSSI(true);
    showFooterBitRate(_icyBitRate);
}
void display_info(const char* str, int32_t xPos, int32_t yPos, uint16_t color, uint16_t margin_l, uint16_t margin_r, uint16_t winWidth, uint16_t winHeight) {
    tft.fillRect(xPos, yPos, winWidth, winHeight, TFT_BLACK); // Clear the space for new info
    tft.setTextColor(color);                                  // Set the requested color
    uint16_t ch_written = tft.writeText(str, xPos + margin_l, yPos, winWidth - margin_r, winHeight, false);
    if(ch_written < strlenUTF8(str)) {
        // If this message appears, there is not enough space on the display to write the entire text,
        // a part of the text has been cut off
        SerialPrintfln("txt overflow, winHeight=" ANSI_ESC_CYAN "%d" ANSI_ESC_WHITE ", strlen=" ANSI_ESC_CYAN "%d" ANSI_ESC_WHITE ", written=" ANSI_ESC_CYAN "%d" ANSI_ESC_WHITE ", str=" ANSI_ESC_CYAN
                       "%s",
                       winHeight, strlenUTF8(str), ch_written, str);
    }
}
void showStreamTitle(const char* streamtitle) {
    if(_f_sleeping) return;
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    String ST = streamtitle;

    ST.trim();               // remove all leading or trailing whitespaces
    ST.replace(" | ", "\n"); // some stations use pipe as \n or
    ST.replace("| ", "\n");  // or
    ST.replace("|", "\n");
    switch(strlenUTF8(ST.c_str())) {
        case 0 ... 30: tft.setFont(_fonts[5]); break;
        case 31 ... 43: tft.setFont(_fonts[4]); break;
        case 44 ... 65: tft.setFont(_fonts[3]); break;
        case 66 ... 130: tft.setFont(_fonts[2]); break;
        case 131 ... 200: tft.setFont(_fonts[1]); break;
        default: tft.setFont(_fonts[0]); break;
    }
    display_info(ST.c_str(), _winSTitle.x, _winSTitle.y, TFT_CORNSILK, 2, 10, _winSTitle.w, _winSTitle.h);
    xSemaphoreGive(mutex_display);
}
void showVUmeter() {
    if(_f_sleeping) return;
    _f_VUmeterIsVisible = true;
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    drawImage("/common/level_bar.jpg", _winVUmeter.x, _winVUmeter.y);
    _VUrightCh = 0;
    _VUleftCh = 0;
    xSemaphoreGive(mutex_display);
}
void hideVUmeter() {
    _f_VUmeterIsVisible = false;
    tft.drawRect(_winVUmeter.x, _winVUmeter.y, _winVUmeter.w, _winVUmeter.h, TFT_BLACK);
}

void showLogoAndStationName() {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    clearLogoAndStationname();
    xSemaphoreGive(mutex_display);
    String SN_utf8 = "";
    if(_cur_station) { SN_utf8 = _stationName_nvs; }
    else { SN_utf8 = _stationName_air; }
    SN_utf8.trim();

    showStationName(SN_utf8);

    showStationLogo(SN_utf8);
}

void showStationName(String sn) {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    switch(strlenUTF8(sn.c_str())) {
        case 0 ... 8: tft.setFont(_fonts[7]); break;
        case 9 ... 11: tft.setFont(_fonts[6]); break;
        case 12 ... 20: tft.setFont(_fonts[5]); break;
        case 21 ... 32: tft.setFont(_fonts[4]); break;
        case 33 ... 45: tft.setFont(_fonts[3]); break;
        case 46 ... 60: tft.setFont(_fonts[2]); break;
        case 61 ... 90: tft.setFont(_fonts[1]); break;
        default: tft.setFont(_fonts[0]); break;
    }
    display_info(sn.c_str(), _winName.x, _winName.y, TFT_CYAN, 10, 0, _winName.w, _winName.h);
    xSemaphoreGive(mutex_display);
}

void showStationLogo(String ln) {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    String logo = "/logo/" + (String)ln.c_str() + ".jpg";
    if(drawImage(logo.c_str(), 0, _winName.y + 2) == false) {
        drawImage("/common/unknown.jpg", 0, _winName.y + 2); // if no draw unknown
        _f_logoUnknown = true;
    }
    xSemaphoreGive(mutex_display);
}

void showFileLogo(uint8_t state) {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    String logo;
    if(state == RADIO) {
        if(endsWith(_stationURL, "m3u8")) logo = "/common/" + (String) "M3U8" + ".jpg";
        else logo = "/common/" + (String)codecname[_cur_Codec] + ".jpg";
        drawImage(logo.c_str(), 0, _winName.y + 2);
        webSrv.send("stationLogo=", logo);
        goto exit;
    }
    else if(state == DLNA) {
        logo = "/common/DLNA.jpg";
        drawImage(logo.c_str(), 0, _winName.y + 2);
        webSrv.send("stationLogo=", logo);
        goto exit;
    }
    if(state == PLAYER) { // _state PLAYER
        if(_cur_Codec == 0) logo = "/common/AudioPlayer.jpg";
        else if(_playerSubmenue == 0) logo = "/common/AudioPlayer.jpg";
        else logo = "/common/" + (String)codecname[_cur_Codec] + ".jpg";
        drawImage(logo.c_str(), 0, _winName.y + 2);
        goto exit;
    }
exit:
    xSemaphoreGive(mutex_display);
}

void showFileName(const char* fname) {
    if(!fname) return;
    switch(strlenUTF8(fname)) {
        case 0 ... 15: tft.setFont(_fonts[5]); break;
        case 16 ... 30: tft.setFont(_fonts[4]); break;
        case 31 ... 70: tft.setFont(_fonts[3]); break;
        case 71 ... 100: tft.setFont(_fonts[2]); break;
        case 101 ... 150: tft.setFont(_fonts[1]); break;
        default: tft.setFont(_fonts[0]); break;
    }
    display_info(fname, _winName.x, _winName.y, TFT_CYAN, 0, 0, _winName.w, _winName.h);
}

void showPlsFileNumber() {
    tft.setFont(_fonts[3]);
    char buf[15];
    sprintf(buf, "%03u/%03u", _plsCurPos, _PLS_content.size());
    display_info(buf, _winFileNr.x, _winFileNr.y, TFT_ORANGE, 10, 0, _winFileNr.w, _winFileNr.h);
}

void showAudioFileNumber() {
    tft.setFont(_fonts[3]);
    char buf[15];
    sprintf(buf, "%03u/%03u", _cur_AudioFileNr, _SD_content.size());
    display_info(buf, _winFileNr.x, _winFileNr.y, TFT_ORANGE, 10, 0, _winFileNr.w, _winFileNr.h);
}

void showStationsList(uint16_t staListNr) {
    clearWithOutHeaderFooter();
    if(_sum_stations < 11) staListNr = 0;
    else if(staListNr + 9 > _max_stations) staListNr = _max_stations - 9;
    showHeadlineItem(STATIONSLIST);
    tft.setFont(_fonts[0]);
    uint8_t lineHight = _winWoHF.h / 10;
    for(uint8_t pos = 0; pos < 10; pos++) {
        if(pos + staListNr + 1 > _sum_stations) break;
        sprintf(_chbuf, "station_%03d", pos + staListNr + 1);
        String content = stations.getString(_chbuf, " #not_found");
        content.replace('#', '\0');
        sprintf(_chbuf, ANSI_ESC_YELLOW "%03d " ANSI_ESC_WHITE "%s\n", pos + staListNr + 1, content.c_str());
        tft.writeText(_chbuf, 10, _winFooter.h + (pos)*lineHight, _dispWidth - 10, lineHight, TFT_ALIGN_LEFT, true, true);
    }
    _timeCounter.timer = 10;
    _timeCounter.factor = 1.0;
}

void display_sleeptime(int8_t ud) { // set sleeptimer
    if(ud == 1) {
        switch(_sleeptime) {
            case 0 ... 14: _sleeptime = (_sleeptime / 5) * 5 + 5; break;
            case 15 ... 59: _sleeptime = (_sleeptime / 15) * 15 + 15; break;
            case 60 ... 359: _sleeptime = (_sleeptime / 60) * 60 + 60; break;
            default: _sleeptime = 360; break; // max 6 hours
        }
    }
    if(ud == -1) {
        switch(_sleeptime) {
            case 1 ... 15: _sleeptime = ((_sleeptime - 1) / 5) * 5; break;
            case 16 ... 60: _sleeptime = ((_sleeptime - 1) / 15) * 15; break;
            case 61 ... 360: _sleeptime = ((_sleeptime - 1) / 60) * 60; break;
            default: _sleeptime = 0; break; // min
        }
    }
    char tmp[10];
    sprintf(tmp, "%d%02d", _sleeptime / 60, _sleeptime % 60);
    char path[128] = "/digits_small/";

    for(uint8_t i = 0; i < 4; i++) {
        strcpy(path, "/digits_small/");
        if(i == 3) {
            if(!_sleeptime) strcat(path, "dsgn.jpg");
            else strcat(path, "dsrt.jpg");
        }
        else {
            strncat(path, (tmp + i), 1);
            if(!_sleeptime) strcat(path, "sgn.jpg");
            else strcat(path, "srt.jpg");
        }
        drawImage(path, _sleeptimeXPos[i], 48);
    }
}

boolean drawImage(const char* path, uint16_t posX, uint16_t posY, uint16_t maxWidth, uint16_t maxHeigth) {
    const char* scImg = scaleImage(path);
    if(!SD_MMC.exists(scImg)) {
        if(indexOf(scImg, "/.", 0) > 0) return false; // empty filename
        SerialPrintfln("AUDIO_info:  " ANSI_ESC_RED "file \"%s\" not found", scImg);
        return false;
    }
    if(endsWith(scImg, "bmp")) { return tft.drawBmpFile(SD_MMC, scImg, posX, posY, maxWidth, maxHeigth); }
    if(endsWith(scImg, "jpg")) { return tft.drawJpgFile(SD_MMC, scImg, posX, posY, maxWidth, maxHeigth); }
    if(endsWith(scImg, "gif")) { return tft.drawGifFile(SD_MMC, scImg, posX, posY, 0); }

    SerialPrintfln(ANSI_ESC_RED "the file \"%s\" contains neither a bmp, a gif nor a jpj graphic", scImg);
    return false; // neither jpg nor bmp
}
/*****************************************************************************************************************************************************
 *                                                   H A N D L E  A U D I O F I L E                                                                  *
 *****************************************************************************************************************************************************/
bool SD_listDir(const char* path, boolean audioFilesOnly, boolean withoutDirs) { // sort the content of an given directory and lay it in the
    File file;                                                                   // vector _SD_content, add to filename ANSI_ESC_YELLOW and file size
    vector_clear_and_shrink(_SD_content);
    if(audioFile) audioFile.close();
    if(!SD_MMC.exists(path)) {
        SerialPrintfln(ANSI_ESC_RED "SD_MMC/%s not exist", path);
        return false;
    }
    audioFile = SD_MMC.open(path);
    if(!audioFile.isDirectory()) {
        SerialPrintfln(ANSI_ESC_RED "SD_MMC/%s is not a directory", path);
        audioFile.close();
        return false;
    }
    while(true) { // get content
        file = audioFile.openNextFile();
        if(!file) break;
        if(file.isDirectory()) {
            if(!withoutDirs) {
                _chbuf[0] = 2; // ASCII: start of text, sort set dirs on first position
                sprintf(_chbuf + 1, "%s", file.name());
                _SD_content.push_back(x_ps_strdup((const char*)_chbuf));
            }
        }
        else {
            if(audioFilesOnly) {
                if(endsWith(file.name(), ".mp3") || endsWith(file.name(), ".aac") || endsWith(file.name(), ".m4a") || endsWith(file.name(), ".wav") || endsWith(file.name(), ".flac") ||
                   endsWith(file.name(), ".m3u") || endsWith(file.name(), ".opus") || endsWith(file.name(), ".ogg")) {
                    sprintf(_chbuf, "%s" ANSI_ESC_YELLOW " %d", file.name(), file.size());
                    _SD_content.push_back(x_ps_strdup((const char*)_chbuf));
                }
            }
            else {
                sprintf(_chbuf, "%s" ANSI_ESC_YELLOW " %d", file.name(), file.size());
                _SD_content.push_back(x_ps_strdup((const char*)_chbuf));
            }
        }
    }
    for(int i = 0; i < _SD_content.size(); i++) { // easy bubble sort
        for(int j = 1; j < _SD_content.size(); j++) {
            if(strcmp(_SD_content[j - 1], _SD_content[i]) > 0) { swap(_SD_content[i], _SD_content[j - 1]); }
        }
    }
    for(int i = 0; i < _SD_content.size(); i++) {
        if(_SD_content[i][0] == 2) { // remove ASCII 2
            memcpy(_SD_content[i], _SD_content[i] + 1, strlen(_SD_content[i]));
        }
    }
    audioFile.close();
    return true;
}

void showAudioFilesList(uint16_t fileListNr) { // on tft

    auto triangleUp = [&](int16_t x, int16_t y, uint8_t s) { tft.fillTriangle(x + s, y + 0, x + 0, y + 2 * s, x + 2 * s, y + 2 * s, TFT_RED); };
    auto triangleDown = [&](int16_t x, int16_t y, uint8_t s) { tft.fillTriangle(x + 0, y + 0, x + 2 * s, y + 0, x + s, y + 2 * s, TFT_RED); };

    clearWithOutHeaderFooter();
    if(_SD_content.size() < 10) fileListNr = 0;
    showHeadlineItem(AUDIOFILESLIST);
    tft.setFont(_fonts[0]);
    uint8_t lineHight = _winWoHF.h / 10;
    tft.setTextColor(TFT_ORANGE);
    tft.writeText(_curAudioFolder.c_str(), 10, _winHeader.h, _dispWidth - 10, lineHight, TFT_ALIGN_LEFT, true, true);
    tft.setTextColor(TFT_WHITE);
    for(uint8_t pos = 1; pos < 10; pos++) {
        if(pos == 1 && fileListNr > 0) {
            tft.setTextColor(TFT_AQUAMARINE);
            triangleUp(0, _winHeader.h + (pos * lineHight), lineHight / 3.5);
        }
        if(pos == 9 && fileListNr + 9 < _SD_content.size()) {
            tft.setTextColor(TFT_AQUAMARINE);
            triangleDown(0, _winHeader.h + (pos * lineHight), lineHight / 3.5);
        }
        if(fileListNr + pos > _SD_content.size()) break;
        if(indexOf(_SD_content[pos + fileListNr - 1], "\033[", 0) == -1) tft.setTextColor(TFT_GRAY); // is folder
        else tft.setTextColor(TFT_WHITE);                                                            // is file
        tft.writeText(_SD_content[pos + fileListNr - 1], 20, _winFooter.h + (pos)*lineHight, _dispWidth - 20, lineHight, TFT_ALIGN_LEFT, true, true);
    }
    _timeCounter.timer = 10;
    _timeCounter.factor = 1.0;
}

boolean isAudio(File file) {
    if(endsWith(file.name(), ".mp3") || endsWith(file.name(), ".aac") || endsWith(file.name(), ".m4a") || endsWith(file.name(), ".wav") || endsWith(file.name(), ".flac") ||
       endsWith(file.name(), ".opus") || endsWith(file.name(), ".ogg")) {
        return true;
    }
    return false;
}

boolean isAudio(const char* path) {
    if(endsWith(path, ".mp3") || endsWith(path, ".aac") || endsWith(path, ".m4a") || endsWith(path, ".wav") || endsWith(path, ".flac") || endsWith(path, ".opus") || endsWith(path, ".ogg")) {
        return true;
    }
    return false;
}

boolean isPlaylist(File file) {
    if(endsWith(file.name(), ".m3u")) { return true; }
    return false;
}

/*****************************************************************************************************************************************************
 *                                                                     P L A Y L I S T                                                               *
 *****************************************************************************************************************************************************/

bool preparePlaylistFromFile(const char* path) {
    File playlistFile = SD_MMC.open(path);
    if(!playlistFile) {
        log_e("playlistfile path not found");
        return false;
    }

    if(playlistFile.size() > 1048576) {
        log_e("Playlist too big, size > 1MB");
        playlistFile.close();
        return false;
    }
    if(_playlistPath) {
        free(_playlistPath);
        _playlistPath = NULL;
    }
    vector_clear_and_shrink(_PLS_content); // clear _PLS_content first
    char* buff1 = x_ps_malloc(2024);
    char* buff2 = x_ps_malloc(1048);
    if(!buff1 || !buff2) {
        log_e("oom");
        playlistFile.close();
        return false;
    }
    size_t bytesRead = 0;
    bool   f_EXTINF_seen = false;

    while(playlistFile.available() > 0) {
        bytesRead = playlistFile.readBytesUntil('\n', buff1, 1024);
        if(bytesRead < 5) continue; // line is # or space or nothing, smallest filename "1.mp3" < 5
        buff1[bytesRead] = '\0';
        trim(buff1);
        if(startsWith(buff1, "#EXTM3U")) continue;
        if(startsWith(buff1, "#EXTINF:")) { // #EXTINF:8,logo-1.mp3
            strcpy(buff2, buff1 + 8);
            f_EXTINF_seen = true;
            continue;
        }
        if(startsWith(buff1, "#")) continue; // all other lines
        if(f_EXTINF_seen) {
            f_EXTINF_seen = false;
            strcat(buff1, "\n");
            strcat(buff1, buff2);
        }
        _PLS_content.push_back(x_ps_strdup((const char*)buff1));
    }
    _playlistPath = strdup(playlistFile.path());
    int idx = lastIndexOf((const char*)_playlistPath, '/');
    if(idx < 0) log_e("wrong playlist path");
    _playlistPath[idx] = '\0';
    playlistFile.close();
    if(buff1) {
        free(buff1);
        buff1 = NULL;
    }
    if(buff2) {
        free(buff2);
        buff2 = NULL;
    }
    return true;
}
//____________________________________________________________________________________________________________________________________________________

bool preparePlaylistFromFolder(const char* path) { // all files whithin a folder
    if(!SD_MMC.exists(path)) {
        SerialPrintfln(ANSI_ESC_RED "SD_MMC/%s not exist", path);
        return false;
    }
    File folder = SD_MMC.open(path);
    if(!folder.isDirectory()) {
        SerialPrintfln(ANSI_ESC_RED "SD_MMC/%s is not a directory", path);
        folder.close();
        return false;
    }
    vector_clear_and_shrink(_PLS_content); // clear _PLS_content first

    while(true) { // get content
        File file = folder.openNextFile();
        if(!file) break;
        if(file.isDirectory()) continue;
        if(isAudio(file)) { _PLS_content.push_back(x_ps_strdup((const char*)file.path())); }
        file.close();
    }
    folder.close();

    for(int i = 0; i < _PLS_content.size(); i++) {
        if(_PLS_content[i][0] == 2) { // remove ASCII 2
            memcpy(_PLS_content[i], _PLS_content[i] + 1, strlen(_PLS_content[i]));
        }
    }

    if(_f_shuffle) sortPlayListRandom();
    else sortPlayListAlphabetical();

    return true;
}
//____________________________________________________________________________________________________________________________________________________

void sortPlayListAlphabetical() {
    for(int i = 0; i < _PLS_content.size(); i++) { // easy bubble sort
        for(int j = 1; j < _PLS_content.size(); j++) {
            if(strcmp(_PLS_content[j - 1], _PLS_content[i]) > 0) { swap(_PLS_content[i], _PLS_content[j - 1]); }
        }
    }
}
//____________________________________________________________________________________________________________________________________________________

void sortPlayListRandom() {
    for(int i = 0; i < _PLS_content.size(); i++) { // easy bubble sort
        uint16_t randIndex = random(0, _PLS_content.size());
        swap(_PLS_content[i], _PLS_content[randIndex]); // swapping the values
    }
}
//____________________________________________________________________________________________________________________________________________________

void processPlaylist(boolean first) {
    if(_PLS_content.size() == 0) {
        log_e("playlist is empty");
        return;
    } // guard
    int16_t idx = 0;
    boolean f_has_EXTINF = false;
    if(first) {
        _plsCurPos = 0;
        _f_playlistEnabled = true;
    }

    if(_plsCurPos == _PLS_content.size()) goto exit;

    _playerSubmenue = 1;
    if(_state != PLAYER) changeState(PLAYER);

    // now read from vector _PLS_content

    idx = indexOf(_PLS_content[_plsCurPos], "\n", 0);
    if(idx > 0) { // has additional infos: duration, title
        f_has_EXTINF = true;
        int16_t idx1 = indexOf(_PLS_content[_plsCurPos], ",", idx);
        SerialPrintfln("Playlist:    " ANSI_ESC_GREEN "Title: %s", _PLS_content[_plsCurPos] + idx1 + 1);
        clearLogo();
        showFileName(_PLS_content[_plsCurPos] + idx1 + 1);
        int8_t len = idx1 - (idx + 1);
        if(len > 0 && len < 6) { // song playtime
            char tmp[7] = {0};
            memcpy(tmp, _PLS_content[_plsCurPos] + idx + 1, len);
            tmp[len] = '\0';
            SerialPrintfln("Playlist:    " ANSI_ESC_GREEN "playtime: %is", atoi(tmp));
        }
        _PLS_content[_plsCurPos][idx] = '\0';
    }
    if(startsWith(_PLS_content[_plsCurPos], "http")) {
        SerialPrintflnCut("Playlist:    ", ANSI_ESC_YELLOW, _PLS_content[_plsCurPos]);
        showVolumeBar();
        if(!f_has_EXTINF) clearLogoAndStationname();
        webSrv.send("SD_playFile=", _PLS_content[_plsCurPos]);
        _cur_Codec = 0;
        connecttohost(_PLS_content[_plsCurPos]);
    }
    else if(startsWith(_PLS_content[_plsCurPos], "file://")) { // root
        urldecode(_PLS_content[_plsCurPos]);
        SerialPrintfln("Playlist:    " ANSI_ESC_YELLOW "%s", _PLS_content[_plsCurPos] + 7);
        webSrv.send("SD_playFile=", _PLS_content[_plsCurPos] + 7);
        SD_playFile(_PLS_content[_plsCurPos] + 7, 0, false);
    }
    else {
        urldecode(_PLS_content[_plsCurPos]);
        char* playFile = NULL;
        if(_playlistPath) { // path of m3u file
            playFile = x_ps_malloc(strlen(_playlistPath) + strlen(_PLS_content[_plsCurPos]) + 5);
            strcpy(playFile, _playlistPath);
            if(_PLS_content[_plsCurPos][0] != '/') strcat(playFile, "/");
            strcat(playFile, _PLS_content[_plsCurPos]);
        }
        else { // have no playlistpath
            playFile = x_ps_malloc(strlen(_PLS_content[_plsCurPos]) + 5);
            strcpy(playFile, _PLS_content[_plsCurPos]);
        }
        SerialPrintfln("Playlist:    " ANSI_ESC_YELLOW "%s", playFile);
        webSrv.send("SD_playFile=", playFile);
        if(f_has_EXTINF) SD_playFile(playFile, 0, false);
        else SD_playFile(playFile, 0, true);
        if(playFile) {
            free(playFile);
            playFile = NULL;
        }
    }
    _plsCurPos++;
    showPlsFileNumber();
    return;

exit:
    SerialPrintfln("Playlist:    " ANSI_ESC_BLUE "end of playlist");
    webSrv.send("SD_playFile=", "end of playlist");
    _f_playlistEnabled = false;
    _playerSubmenue = 0;
    changeState(PLAYER);
    _plsCurPos = 0;
    if(_playlistPath) {
        free(_playlistPath);
        _playlistPath = NULL;
    }
    return;
}
/*****************************************************************************************************************************************************
 *                                         C O N N E C T   TO   W I F I     /     A C C E S S P O I N T                                              *
 *****************************************************************************************************************************************************/
bool connectToWiFi() {
    String s_ssid = "", s_password = "", s_info = "";
    wifiMulti.addAP(_SSID, _PW);                        // SSID and PW in code
    if(pref.isKey("ap_ssid") && pref.isKey("ap_pw")) {  // exists?
        String ap_ssid = pref.getString("ap_ssid", ""); // credentials from accesspoint
        String ap_pw = pref.getString("ap_pw", "");
        if(ap_ssid.length() > 0 && ap_pw.length() > 0) wifiMulti.addAP(ap_ssid.c_str(), ap_pw.c_str());
    }
    WiFi.setHostname("MiniWebRadio");
    if(psramFound()) WiFi.useStaticBuffers(true);
    File file = SD_MMC.open("/networks.csv"); // try credentials given in "/networks.txt"
    if(file) {                                // try to read from SD_MMC
        String str = "";
        while(file.available()) {
            str = file.readStringUntil('\n');   // read the line
            if(str[0] == '*') continue;         // ignore this, goto next line
            if(str[0] == '\n') continue;        // empty line
            if(str[0] == ' ') continue;         // space as first char
            if(str.indexOf('\t') < 0) continue; // no tab
            str += "\t";
            uint p = 0, q = 0;
            s_ssid = "", s_password = "", s_info = "";
            for(int32_t i = 0; i < str.length(); i++) {
                if(str[i] == '\t') {
                    if(p == 0) s_ssid = str.substring(q, i);
                    if(p == 1) s_password = str.substring(q, i);
                    if(p == 2) s_info = str.substring(q, i);
                    p++;
                    i++;
                    q = i;
                }
            }
            // log_i("s_ssid=%s  s_password=%s  s_info=%s", s_ssid.c_str(), s_password.c_str(), s_info.c_str());
            if(s_ssid == "") continue;
            if(s_password == "") continue;
            wifiMulti.addAP(s_ssid.c_str(), s_password.c_str());
        }
        file.close();
    }
    wifiMulti.run();

    if(WiFi.isConnected()) {
        SerialPrintfln("WiFI_info:   Connecting WiFi...");
        WiFi.setSleep(false);
        if(!MDNS.begin("MiniWebRadio")) { SerialPrintfln("WiFI_info:   " ANSI_ESC_YELLOW "Error starting mDNS"); }
        else {
            MDNS.addService("esp32", "tcp", 80);
            SerialPrintfln("WiFI_info:   mDNS name: " ANSI_ESC_CYAN "MiniWebRadio");
        }
        return true;
    }
    else {
        SerialPrintfln("WiFI_info:   " ANSI_ESC_RED "WiFi credentials are not correct");
        return false; // can't connect to any network
    }
}

void openAccessPoint() { // if credentials are not correct open AP at 192.168.4.1
    clearAll();
    tft.setFont(_fonts[4]);
    tft.setTextColor(TFT_YELLOW);
    setTFTbrightness(80);
    _f_accessPoint = true;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.softAP("MiniWebRadio");
    IPAddress myIP = WiFi.softAPIP();
    String    AccesspointIP = myIP.toString();
    char      buf[100];
    sprintf(buf, "WiFi credentials are not correct\nAccesspoint IP: " ANSI_ESC_CYAN "%s", AccesspointIP.c_str());
    tft.writeText(buf, 0, 0, _dispWidth, _dispHeight, TFT_ALIGN_LEFT, true, false);
    SerialPrintfln("Accesspoint: " ANSI_ESC_RED "IP: %s", AccesspointIP.c_str());
    int16_t n = WiFi.scanNetworks();
    if(n == 0) {
        SerialPrintfln("setup: ....  no WiFi networks found");
        while(true) { ; }
    }
    else {
        SerialPrintfln("setup: ....  %d WiFi networks found", n);
        for(int32_t i = 0; i < n; ++i) {
            SerialPrintfln("setup: ....  " ANSI_ESC_GREEN "%s (%d)", WiFi.SSID(i).c_str(), (int16_t)WiFi.RSSI(i));
            _scannedNetworks += WiFi.SSID(i) + '\n';
        }
    }
    webSrv.begin(80, 81); // HTTP port, WebSocket port
    return;
}

/*****************************************************************************************************************************************************
 *                                                                     A U D I O                                                                     *
 *****************************************************************************************************************************************************/
void connecttohost(const char* host) {
    int32_t idx1, idx2;
    char*   url = nullptr;
    char*   user = nullptr;
    char*   pwd = nullptr;

    clearBitRate();
    _cur_Codec = 0;
    //    if(_state == RADIO) clearStreamTitle();
    _icyBitRate = 0;
    _avrBitRate = 0;

    idx1 = indexOf(host, "|", 0);
    // log_i("idx1 = %i", idx1);
    if(idx1 == -1) { // no pipe found
        _f_isWebConnected = audioConnecttohost(host);
        _f_isFSConnected = false;
        return;
    }
    else { // pipe found
        idx2 = indexOf(host, "|", idx1 + 1);
        // log_i("idx2 = %i", idx2);
        if(idx2 == -1) { // second pipe not found
            _f_isWebConnected = audioConnecttohost(host);
            _f_isFSConnected = false;
            return;
        }
        else {                         // extract url, user, pwd
            url = strndup(host, idx1); // extract url
            user = strndup(host + idx1 + 1, idx2 - idx1 - 1);
            pwd = strdup(host + idx2 + 1);
            SerialPrintfln("new host: .  %s user %s, pwd %s", url, user, pwd) _f_isWebConnected = audioConnecttohost(url, user, pwd);
            _f_isFSConnected = false;
            if(url) free(url);
            if(user) free(user);
            if(pwd) free(pwd);
        }
    }
}
void connecttoFS(const char* filename, uint32_t resumeFilePos) {
    clearBitRate();
    _icyBitRate = 0;
    _avrBitRate = 0;
    _cur_Codec = 0;
    _f_isFSConnected = audioConnecttoFS(filename, resumeFilePos);
    _f_isWebConnected = false;
    //    log_w("Filesize %d", audioGetFileSize());
    //    log_w("FilePos %d", audioGetFilePosition());
}
void stopSong() {
    audioStopSong();
    _f_isFSConnected = false;
    _f_isWebConnected = false;
    _f_playlistEnabled = false;
    _f_pauseResume = false;
    _f_playlistNextFile = false;
    _f_shuffle = false;
}

/*****************************************************************************************************************************************************
 *                                                                    S E T U P                                                                      *
 *****************************************************************************************************************************************************/
void setup() {
    Serial.begin(MONITOR_SPEED);
    Serial.print("\n\n");
    const char* chipModel = ESP.getChipModel();
    uint8_t     avMajor = ESP_ARDUINO_VERSION_MAJOR;
    uint8_t     avMinor = ESP_ARDUINO_VERSION_MINOR;
    uint8_t     avPatch = ESP_ARDUINO_VERSION_PATCH;
    Serial.printf("ESP32 Chip: %s\n", chipModel);
    Serial.printf("Arduino Version: %d.%d.%d\n", avMajor, avMinor, avPatch);
    uint8_t idfMajor = ESP_IDF_VERSION_MAJOR;
    uint8_t idfMinor = ESP_IDF_VERSION_MINOR;
    uint8_t idfPatch = ESP_IDF_VERSION_PATCH;
    Serial.printf("ESP-IDF Version: %d.%d.%d\n", idfMajor, idfMinor, idfPatch);
    Version = Version.substring(0, 30);
    Serial.printf("MiniWebRadio %s\n", Version.c_str());
    Serial.printf("ARDUINO_LOOP_STACK_SIZE %d words (32 bit)\n", CONFIG_ARDUINO_LOOP_STACK_SIZE);
    Serial.printf("FLASH size %lu bytes, speed %lu MHz\n", (long unsigned)ESP.getFlashChipSize(), (long unsigned)ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("CPU speed %lu MHz\n", (long unsigned)ESP.getCpuFreqMHz());
    Serial.printf("SDMMC speed %d MHz\n", SDMMC_FREQUENCY / 1000000);
    Serial.printf("TFT speed %d MHz\n", TFT_FREQUENCY / 1000000);
    if(!psramInit()) { Serial.printf(ANSI_ESC_RED "PSRAM not found! MiniWebRadio doesn't work properly without PSRAM!" ANSI_ESC_WHITE); }
    else {
        _f_PSRAMfound = true;
        Serial.printf("PSRAM total size: %lu bytes\n", (long unsigned)ESP.getPsramSize());
    }
    if(ESP.getFlashChipSize() > 80000000) {
        if(!FFat.begin()) {
            if(!FFat.format()) Serial.printf("FFat Mount Failed\n");
        }
        else {
            Serial.printf("FFat total space: %d bytes, free space: %d bytes", FFat.totalBytes(), FFat.freeBytes());
            _f_FFatFound = true;
        }
    }
    const char* rr = NULL;
    _resetResaon = esp_reset_reason();
    switch(_resetResaon) {
        case ESP_RST_UNKNOWN: rr = "Reset reason can not be determined"; break;
        case ESP_RST_POWERON: rr = "Reset due to power-on event"; break;
        case ESP_RST_EXT: rr = "Reset by external pin (not applicable for ESP32)"; break;
        case ESP_RST_SW: rr = "Software reset via esp_restart"; break;
        case ESP_RST_PANIC: rr = "Software reset due to exception/panic"; break;
        case ESP_RST_INT_WDT: rr = "Reset (software or hardware) due to interrupt watchdog"; break;
        case ESP_RST_TASK_WDT: rr = "Reset due to task watchdog"; break;
        case ESP_RST_WDT:
            rr = "Reset due to other watchdogs";
            _resetResaon = 1;
            break;
        case ESP_RST_DEEPSLEEP: rr = "Reset after exiting deep sleep mode"; break;
        case ESP_RST_BROWNOUT: rr = "Brownout reset (software or hardware)"; break;
        case ESP_RST_SDIO: rr = "Reset over SDIO"; break;
    }
    Serial.printf("RESET_REASON: %s", rr);
    Serial.print("\n\n");
    mutex_rtc = xSemaphoreCreateMutex();
    mutex_display = xSemaphoreCreateMutex();
    SerialPrintfln("   ");
    SerialPrintfln(ANSI_ESC_YELLOW "       ***************************    ");
    SerialPrintfln(ANSI_ESC_YELLOW "       *     MiniWebRadio V3     *    ");
    SerialPrintfln(ANSI_ESC_YELLOW "       ***************************    ");
    SerialPrintfln("   ");
    if(startsWith(chipModel, "ESP32-D")) { ; } // ESP32-D    ...  okay
    if(startsWith(chipModel, "ESP32-P")) { ; } // ESP32-PICO ...  okay
    if(startsWith(chipModel, "ESP32-S2")) {
        SerialPrintfln(ANSI_ESC_RED "MiniWebRadio does not work with ESP32-S2");
        return;
    }
    if(startsWith(chipModel, "ESP32-C3")) {
        SerialPrintfln(ANSI_ESC_RED "MiniWebRadio does not work with ESP32-C3");
        return;
    }
    if(startsWith(chipModel, "ESP32-S3")) { ; } // ESP32-S3  ...  okay
    _f_ESPfound = true;

    SerialPrintfln("setup: ....  Arduino is pinned to core " ANSI_ESC_CYAN "%d", xPortGetCoreID());
    if(TFT_CONTROLLER < 2) strcpy(_prefix, "/s");
    else strcpy(_prefix, "/m");
    stations.begin("Stations", false); // instance of preferences for stations (name, url ...)
    pref.begin("Pref", false);         // instance of preferences from AccessPoint (SSID, PW ...)

#if CONFIG_IDF_TARGET_ESP32
    tft.begin(TFT_CS, TFT_DC, VSPI, TFT_MOSI, TFT_MISO, TFT_SCK); // Init TFT interface ESP32
#else
    tft.begin(TFT_CS, TFT_DC, FSPI, TFT_MOSI, TFT_MISO, TFT_SCK); // Init TFT interface ESP32S3
#endif

    tft.setFrequency(TFT_FREQUENCY);
    tft.setRotation(TFT_ROTATION);
    tft.setBackGoundColor(TFT_BLACK);
    tp.setVersion(TP_VERSION);
    tp.setRotation(TP_ROTATION);
    tp.setMirror(TP_H_MIRROR, TP_V_MIRROR);
    tp.TP_Send(0xD0);
    tp.TP_Send(0x90); // Remove any blockage

    SerialPrintfln("setup: ....  Init SD card");
    if(IR_PIN >= 0) pinMode(IR_PIN, INPUT_PULLUP); // if ir_pin is read only, have a external resistor (~10...40KOhm)
    pinMode(SD_MMC_D0, INPUT_PULLUP);
#ifdef CONFIG_IDF_TARGET_ESP32S3
    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
#endif
    int32_t sdmmc_frequency = SDMMC_FREQUENCY / 1000; // MHz -> KHz, default is 40MHz
    if(!SD_MMC.begin("/sdcard", true, false, sdmmc_frequency)) {
        clearAll();
        tft.setFont(_fonts[6]);
        tft.setTextColor(TFT_YELLOW);
        tft.writeText("SD Card Mount Failed", 0, 50, _dispWidth, _dispHeight, TFT_ALIGN_CENTER, false, false);
        setTFTbrightness(80);
        SerialPrintfln(ANSI_ESC_RED "SD Card Mount Failed");
        return;
    }
    float cardSize = ((float)SD_MMC.cardSize()) / (1024 * 1024);
    float freeSize = ((float)SD_MMC.cardSize() - SD_MMC.usedBytes()) / (1024 * 1024);
    SerialPrintfln(ANSI_ESC_WHITE "setup: ....  SD card found, %.1f MB by %.1f MB free", freeSize, cardSize);
    _f_SD_MMCfound = true;
    if(ESP.getFlashChipSize() > 80000000) { FFat.begin(); }
    defaultsettings();                           // first init
    if(TFT_BL >= 0) ledcAttach(TFT_BL, 1200, 8); // 1200 Hz PWM and 8 bit resolution
    if(getBrightness() >= 5) setTFTbrightness(getBrightness());
    else setTFTbrightness(5);
    if(TFT_CONTROLLER > 6) SerialPrintfln(ANSI_ESC_RED "The value in TFT_CONTROLLER is invalid");
    drawImage("/common/MiniWebRadioV3.jpg", 0, 0); // Welcomescreen
    SerialPrintfln("setup: ....  seek for stations.csv");
    File file = SD_MMC.open("/stations.csv");
    if(!file) {
        clearAll();
        tft.setFont(_fonts[6]);
        tft.setTextColor(TFT_YELLOW);
        tft.writeText("stations.csv not found", 0, 50, _dispWidth, _dispHeight, TFT_ALIGN_CENTER, false, false);
        setTFTbrightness(80);
        SerialPrintfln(ANSI_ESC_RED "stations.csv not found");
        while(1) {}; // endless loop, MiniWebRadio does not work without stations.csv
    }
    file.close();
    SerialPrintfln("setup: ....  stations.csv found");
    updateSettings();
    SerialPrintfln("setup: ....  seek for WiFi networks");
    if(!connectToWiFi()) {
        openAccessPoint();
        return;
    }
    strcpy(_myIP, WiFi.localIP().toString().c_str());
    SerialPrintfln("setup: ....  connected to " ANSI_ESC_CYAN "%s" ANSI_ESC_WHITE ", IP address is " ANSI_ESC_CYAN "%s", WiFi.SSID().c_str(), _myIP);
    ftpSrv.begin(SD_MMC, FTP_USERNAME, FTP_PASSWORD); // username, password for ftp.

    setRTC(_TZString.c_str());

#if DECODER > 1 // DAC controlled by I2C
    if(!dac.begin(I2C_DATA, I2C_CLK, 400000)) { SerialPrintfln(ANSI_ESC_RED "The DAC was not be initialized"); }
#endif

    audioInit();

    audioConnectionTimeout(CONN_TIMEOUT, CONN_TIMEOUT_SSL);

    SerialPrintfln("setup: ....  Number of saved stations: " ANSI_ESC_CYAN "%d", _sum_stations);
    SerialPrintfln("setup: ....  current station number: " ANSI_ESC_CYAN "%d", _cur_station);
    SerialPrintfln("setup: ....  current volume: " ANSI_ESC_CYAN "%d", _cur_volume);
    SerialPrintfln("setup: ....  last connected host: " ANSI_ESC_CYAN "%s", _lastconnectedhost.c_str());
    SerialPrintfln("setup: ....  connection timeout: " ANSI_ESC_CYAN "%d" ANSI_ESC_WHITE " ms", CONN_TIMEOUT);
    SerialPrintfln("setup: ....  connection timeout SSL: " ANSI_ESC_CYAN "%d" ANSI_ESC_WHITE " ms", CONN_TIMEOUT_SSL);

    _state = RADIO;

    ir.begin(); // Init InfraredDecoder

    webSrv.begin(80, 81); // HTTP port, WebSocket port

    if(HP_DETECT != -1) {
        pinMode(HP_DETECT, INPUT);
        attachInterrupt(HP_DETECT, headphoneDetect, CHANGE);
    }
    if(AMP_ENABLED != -1) { // enable onboard amplifier
        pinMode(AMP_ENABLED, OUTPUT);
        digitalWrite(AMP_ENABLED, HIGH);
    }

    tft.fillScreen(TFT_BLACK); // Clear screen
    muteChanged(_f_mute);
    showHeadlineVolume();
    if(_f_mute) { SerialPrintfln("setup: ....  volume is muted: (from " ANSI_ESC_CYAN "%d" ANSI_ESC_RESET ")", _cur_volume); }
    showHeadlineItem(RADIO);
    _state = RADIO;
    setI2STone();
    showFooter();

    showVUmeter();
    ticker100ms.attach(0.1, timer100ms);
    bt_emitter.begin();
    bt_emitter.userCommand("AT+GMR?");     // get version
    bt_emitter.userCommand("AT+VOL?");     // get volume (in receiver mode 0 ... 31)
    bt_emitter.userCommand("AT+BT_MODE?"); // transmitter or receiver

    _dlnaLevel = 0;
    _dlnaHistory[0].name = strdup("Media Server");
    _dlnaHistory[0].objId = strdup("");
    _dlnaHistory[1].objId = strdup("0");
    _f_dlnaSeekServer = true;

    ArduinoOTA.setHostname("MiniWebRadio");
    ArduinoOTA.begin();

    if(_resetResaon == ESP_RST_POWERON ||   // Simply switch on the operating voltage
       _resetResaon == ESP_RST_SW ||        // ESP.restart()
       _resetResaon == ESP_RST_SDIO ||      // The boot button was pressed
       _resetResaon == ESP_RST_DEEPSLEEP) { // Wake up
        if(_cur_station > 0) setStation(_cur_station);
        else { setStationViaURL(_lastconnectedhost.c_str()); }
    }
    else { SerialPrintfln("RESET_REASON:" ANSI_ESC_RED "%s", rr); }
    placingGraphicObjects();
}
/*****************************************************************************************************************************************************
 *                                                                   C O M M O N                                                                     *
 *****************************************************************************************************************************************************/

const char* scaleImage(const char* path) {
    if((!endsWith(path, "bmp")) && (!endsWith(path, "jpg")) && (!endsWith(path, "gif"))) { // not a image
        return path;
    }
    static char pathBuff[256];
    memset(pathBuff, 0, sizeof(pathBuff));
    char* pch = strstr(path + 1, "/");
    if(pch) {
        strncpy(pathBuff, path, (pch - path));
        strcat(pathBuff, _prefix);
        strcat(pathBuff, pch);
    }
    else {
        return path; // invalid path
    }
    return pathBuff;
}

void setVolume(uint8_t vol) {
    audioSetVolume(vol);
    showHeadlineVolume();
    _cur_volume = vol;
    SerialPrintfln("action: ...  current volume is " ANSI_ESC_CYAN "%d", _cur_volume);

#if DECODER > 1 // ES8388, AC101 ...
    if(HP_DETECT == -1) {
        dac.SetVolumeSpeaker(_cur_volume * 3);
        dac.SetVolumeHeadphone(_cur_volume * 3);
    }
    else {
        if(digitalRead(HP_DETECT) == HIGH) {
            // SerialPrintfln("HP_Detect = High, volume %i", vol);
            dac.SetVolumeSpeaker(_cur_volume * 3);
            dac.SetVolumeHeadphone(0);
        }
        else {
            // SerialPrintfln("HP_Detect = Low, volume %i", vol);
            dac.SetVolumeSpeaker(1);
            dac.SetVolumeHeadphone(_cur_volume * 3);
        }
    }
#endif
}

uint8_t downvolume() {
    if(_cur_volume == 0) return _cur_volume;
    _cur_volume--;
//    setVolume(_cur_volume);
    _f_mute = false;
    muteChanged(_f_mute); // set mute off
    return _cur_volume;
}
uint8_t upvolume() {
    if(_cur_volume == _max_volume) return _cur_volume;
    _cur_volume++;
//    setVolume(_cur_volume);
    _f_mute = false;
    muteChanged(_f_mute); // set mute off
    return _cur_volume;
}

void setStation(uint16_t sta) {
    // SerialPrintfln("sta %d, _cur_station %d", sta, _cur_station );
    if(sta == 0) return;
    if(sta > _sum_stations) sta = _cur_station;
    sprintf(_chbuf, "station_%03d", sta);
    String content = stations.getString(_chbuf, " #not_found");
    //    SerialPrintfln("content %s", content.c_str());
    _stationName_nvs = content.substring(0, content.indexOf("#"));           // get stationname
    content = content.substring(content.indexOf("#") + 1, content.length()); // get URL
    content.trim();
    free(_stationURL);
    _stationURL = x_ps_strdup(content.c_str());
    _homepage = "";
    if(_state == RADIO) clearStreamTitle();

    SerialPrintfln("action: ...  switch to station " ANSI_ESC_CYAN "%d", sta);

    if(_f_isWebConnected && sta == _cur_station && _state == RADIO) { // Station is already selected
        _f_newStreamTitle = true;
    }
    else {
        _streamTitle[0] = '\0';
        _icyDescription[0] = '\0';
        _f_newStreamTitle = true;
        _f_newIcyDescription = true;
        connecttohost(_stationURL);
    }
    _cur_station = sta;
    StationsItems();
    if(_state == RADIO) showLogoAndStationName();
    showFooterStaNr();
}
void nextStation() {
    if(_cur_station >= _sum_stations) setStation(1);
    else setStation(_cur_station + 1);
}
void prevStation() {
    if(_cur_station > 1) setStation(_cur_station - 1);
    else setStation(_sum_stations);
}

void StationsItems() {
    if(_cur_station > 0) {
        webSrv.send("stationLogo=", "/logo/" + _stationName_nvs + ".jpg");
        webSrv.send("stationNr=", String(_cur_station));
        if(_stationURL) webSrv.send("stationNr=", String(_stationURL));
    }
    else {
        webSrv.send("stationLogo=", "/logo/" + _stationName_air + ".jpg");
        webSrv.send("stationNr=", String(_cur_station));
        //    webSrv.send("", "stationURL=" + _lastconnectedhost);
    }
}

void setStationViaURL(const char* url) {
    _stationName_air = "";
    _stationName_nvs = "";
    _cur_station = 0;
    free(_stationURL);
    _stationURL = x_ps_strdup(url);
    connecttohost(url);
    StationsItems();
    if(_state == RADIO) {
        clearStreamTitle();
        showLogoAndStationName();
    }
    showFooterStaNr(); // set to '000'
}

void changeBtn_pressed(uint8_t btnNr) { drawImage(_pressBtn[btnNr], btnNr * _winButton.w, _winButton.y); }
void changeBtn_released(uint8_t btnNr) { drawImage(_releaseBtn[btnNr], btnNr * _winButton.w, _winButton.y); }

void savefile(const char* fileName, uint32_t contentLength) { // save the uploadfile on SD_MMC
    char fn[256];

    if(!_f_SD_Upload && endsWith(fileName, "jpg")) {
        strcpy(fn, "/logo");
        strcat(fn, _prefix);
        if(!startsWith(fileName, "/")) strcat(fn, "/");
        strcat(fn, fileName);
        if(webSrv.uploadB64image(SD_MMC, fn, contentLength)) {
            SerialPrintfln("save image " ANSI_ESC_CYAN "%s" ANSI_ESC_WHITE " to SD card was successfully", fn);
            webSrv.sendStatus(200);
        }
        else webSrv.sendStatus(400);
    }
    else {
        _f_SD_Upload = false;
        if(!startsWith(fileName, "/")) {
            strcpy(fn, "/");
            strcat(fn, fileName);
        }
        else { strcpy(fn, fileName); }
        if(webSrv.uploadfile(SD_MMC, fn, contentLength)) {
            SerialPrintfln("save file:   " ANSI_ESC_CYAN "%s" ANSI_ESC_WHITE " to SD card was successfully", fn);
            webSrv.sendStatus(200);
        }
        else {
            SerialPrintfln("save file:   " ANSI_ESC_CYAN "%s" ANSI_ESC_WHITE " to SD failed", fn);
            webSrv.sendStatus(400);
        }
        if(strcmp(fn, "/stations.csv") == 0) saveStationsToNVS();
    }
}

String setI2STone() {
    int8_t LP = _toneLP;
    int8_t BP = _toneBP;
    int8_t HP = _toneHP;
    int8_t BAL = _toneBAL;
    audioSetTone(LP, BP, HP, BAL);
    sprintf(_chbuf, "LowPass=%i\nBandPass=%i\nHighPass=%i\nBalance=%i\n", LP, BP, HP, BAL);
    String tone = String(_chbuf);
    return tone;
}

void SD_playFile(const char* path, const char* fileName) {
    sprintf(_chbuf, "%s/%s", path, fileName);
    int32_t idx = indexOf(_chbuf, "\033[", 1);
    if(idx == -1) { ; }          // do nothing
    else { _chbuf[idx] = '\0'; } // remove color and filesize
    SD_playFile(_chbuf, 0, true);
}

void SD_playFile(const char* path, uint32_t resumeFilePos, bool showFN) {
    if(!path) return;                            // avoid a possible crash
    if(endsWith(path, "ogg")) resumeFilePos = 0; // resume only mp3, m4a, flac and wav
    if(endsWith(path, "m3u")) {
        if(SD_MMC.exists(path)) {
            preparePlaylistFromFile(path);
            processPlaylist(true);
        }
        return;
    }
    showVolumeBar();
    int32_t idx = lastIndexOf(path, '/');
    if(idx < 0) return;
    if(showFN) {
        clearLogo();
        showFileName(path + idx + 1);
    }
    _playerSubmenue = 1;
    changeState(PLAYER);
    SerialPrintfln("AUDIO_FILE:  " ANSI_ESC_MAGENTA "%s", path + idx + 1);
    connecttoFS((const char*)path, resumeFilePos);
    showPlsFileNumber();
    if(_f_isFSConnected) {
        free(_lastconnectedfile);
        _lastconnectedfile = x_ps_strdup(path);
        _resumeFilePos = 0;
    }
}

bool SD_rename(const char* src, const char* dest) {
    bool success = false;
    if(SD_MMC.exists(src)) {
        log_i("exists");
        success = SD_MMC.rename(src, dest);
    }
    return success;
}

bool SD_newFolder(const char* folderPathName) {
    bool success = false;
    success = SD_MMC.mkdir(folderPathName);
    return success;
}

bool SD_delete(const char* itemPath) {
    bool success = false;
    if(SD_MMC.exists(itemPath)) {
        File dirTest = SD_MMC.open(itemPath, "r");
        bool isDir = dirTest.isDirectory();
        dirTest.close();
        if(isDir) success = SD_MMC.rmdir(itemPath);
        else success = SD_MMC.remove(itemPath);
    }
    return success;
}

void headphoneDetect() { // called via interrupt
    _f_hpChanged = true;
}

void fall_asleep() {
    xSemaphoreTake(mutex_display, portMAX_DELAY);
    _f_sleeping = true;
    _f_playlistEnabled = false;
    _f_isFSConnected = false;
    _f_isWebConnected = false;
    audioStopSong();
    if(_state != CLOCK) {
        clearAll();
        setTFTbrightness(0);
    }
    xSemaphoreGive(mutex_display);
    WiFi.disconnect(true); // Disconnect from the network
    vTaskDelay(300);
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
    setCpuFrequencyMhz(80); // 10MHz and 40MHz seem to stop TPanel after minute or so.
    if(_state == CLOCKico) _state = CLOCK;
    SerialPrintfln("falling asleep");
}

void wake_up() {
    log_i("wake_up");
    if(_f_sleeping == true || _f_eof_alarm) { // awake
        _f_sleeping = false;
        setCpuFrequencyMhz(240);
        SerialPrintfln("awake");
        _f_mute = true;
        //    mute();
        clearAll();
        setTFTbrightness(_brightness);
        WiFi.disconnect(false); // Reconnect the network
        wifiMulti.run();
        Serial.print("START WIFI   ");
        while(WiFi.status() != WL_CONNECTED) {
            delay(100);
            Serial.print(".");
        }
        Serial.println("");
        SerialPrintfln("WiFi connected");
        if(_state == UNDEFINED) {
            changeState(RADIO);
            showFooter();
            showHeadlineTime();
            showHeadlineVolume();
            return;
        }
        connecttohost(_lastconnectedhost.c_str());
        showFooter();
        showHeadlineTime();
        showHeadlineVolume();
        if(_state == CLOCK) {
            showHeadlineItem(CLOCK);
            clk_C_green.show();
        }
        else {
            changeState(RADIO);
            showVUmeter();
            showLogoAndStationName();
            showHeadlineItem(RADIO);
        }
    }
}

void setRTC(const char* TZString) {
    rtc.stop();
    _f_rtc = rtc.begin(_TZString.c_str());
    if(!_f_rtc) {
        SerialPrintfln(ANSI_ESC_RED "connection to NTP failed, trying again");
        ESP.restart();
    }
}

void vector_clear_and_shrink(vector<char*>& vec) {
    uint size = vec.size();
    for(int32_t i = 0; i < size; i++) {
        if(vec[i]) {
            free(vec[i]);
            vec[i] = NULL;
        }
    }
    vec.clear();
    vec.shrink_to_fit();
}

boolean copySDtoFFat(const char* path) {
    if(!_f_FFatFound) return false;
    uint8_t buffer[1024];
    size_t  r = 0, w = 0;
    size_t  len = 0;
    File    file1 = SD_MMC.open(path, "r");
    File    file2 = FFat.open(path, "w");
    while(true) {
        r = file1.read(buffer, 1024);
        w = file2.write(buffer, r);
        if(r != w) {
            file1.close();
            file2.close();
            FFat.remove(path);
            return false;
        }
        len += r;
        if(r == 0) break;
    }
    log_i("file length %i, written %i", file1.size(), len);
    if(file1.size() == len) return true;
    return false;
}

void muteChanged(bool m) {
    btn_C_Mute.setValue(m);
    btn_D_Mute.setValue(m);
    btn_E_Mute.setValue(m);
    btn_P_Mute.setValue(m);
    btn_R_Mute.setValue(m);
    if(m) audioMute(0);
    else audioMute(_cur_volume);
    if(m) webSrv.send("mute=", "1");
    else webSrv.send("mute=", "0");
    showHeadlineVolume();
    updateSettings();
};

void logAlarmItems() {
    const char wd[7][11] = {"Sunday:   ", "Monday:   ", "Tuesday:  ", "Wednesday:", "Thursday: ", "Friday:   ", "Saturday: "};
    uint8_t    mask = 0b00000001;
    for(uint8_t i = 0; i < 7; i++) {
        if(_alarmdays & mask) { SerialPrintfln("AlarmTime:   " ANSI_ESC_YELLOW "%s " ANSI_ESC_CYAN "%02i:%02i", wd[i], _alarmtime[i] / 60, _alarmtime[i] % 60); }
        else { SerialPrintfln("AlarmTime:   " ANSI_ESC_YELLOW "%s No alarm is set", wd[i]); }
        mask <<= 1;
    }
}


/*         ╔═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
           ║                                                                                  M E N U E / B U T T O N S                                                                  ║
           ╚═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝   */

// clang-format off
void placingGraphicObjects() { // and initialize them
    // RADIO -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    btn_R_Mute.begin(    0 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_Mute.setOffPicturePath("/btn/Button_Mute_Green.jpg");
                                                                                        btn_R_Mute.setOnPicturePath("/btn/Button_Mute_Red.jpg");
                                                                                        btn_R_Mute.setClickedOffPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_R_Mute.setClickedOnPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_R_Mute.setValue(_f_mute);
    btn_R_volDown.begin( 1 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_volDown.setDefaultPicturePath("/btn/Button_Volume_Down_Blue.jpg");
                                                                                        btn_R_volDown.setClickedPicturePath("/btn/Button_Volume_Down_Yellow.jpg");
    btn_R_volUp.begin(   2 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_volUp.setDefaultPicturePath("/btn/Button_Volume_Up_Blue.jpg");
                                                                                        btn_R_volUp.setClickedPicturePath("/btn/Button_Volume_Up_Yellow.jpg");
    btn_R_prevSta.begin( 3 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_prevSta.setDefaultPicturePath("/btn/Button_Previous_Green.jpg");
                                                                                        btn_R_prevSta.setClickedPicturePath("/btn/Button_Previous_Yellow.jpg");
    btn_R_nextSta.begin( 4 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_nextSta.setDefaultPicturePath("/btn/Button_Next_Green.jpg");
                                                                                        btn_R_nextSta.setClickedPicturePath("/btn/Button_Next_Yellow.jpg");
    btn_R_staList.begin( 5 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_staList.setDefaultPicturePath("/btn/Button_List_Green.jpg");
                                                                                        btn_R_staList.setClickedPicturePath("/btn/Button_List_Yellow.jpg");
    btn_R_player.begin(  0 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_player.setDefaultPicturePath("/btn/Player_Green.jpg");
                                                                                        btn_R_player.setClickedPicturePath("/btn/Player_Yellow.jpg");
    btn_R_dlna.begin(    1 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_dlna.setDefaultPicturePath("/btn/Button_DLNA_Green.jpg");
                                                                                        btn_R_dlna.setClickedPicturePath("/btn/Button_DLNA_Yellow.jpg");
    btn_R_clock.begin(   2 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_clock.setDefaultPicturePath("/btn/Clock_Green.jpg");
                                                                                        btn_R_clock.setClickedPicturePath("/btn/Clock_Yellow.jpg");
    btn_R_sleep.begin(   3 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_sleep.setDefaultPicturePath("/btn/Button_Sleep_Green.jpg");
                                                                                        btn_R_sleep.setClickedPicturePath("/btn/Button_Sleep_Yellow.jpg");
    btn_R_bright.begin(  4 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_R_bright.setDefaultPicturePath("/btn/Bulb_Green.jpg");
                                                                                        btn_R_bright.setClickedPicturePath("/btn/Bulb_Yellow.jpg");
                                                                                        btn_R_bright.setInactivePicturePath("/btn/Bulb_Grey.jpg");
                                                                                        if(TFT_BL == -1) btn_R_bright.setInactive();
    btn_R_equal.begin(   5 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);      btn_R_equal.setDefaultPicturePath("/btn/Button_EQ_Green.jpg");
                                                                                        btn_R_equal.setClickedPicturePath("/btn/Button_EQ_Yellow.jpg");
    // PLAYER-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    btn_P_Mute.begin(    0 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_Mute.setOffPicturePath("/btn/Button_Mute_Green.jpg");
                                                                                        btn_P_Mute.setOnPicturePath("/btn/Button_Mute_Red.jpg");
                                                                                        btn_P_Mute.setClickedOffPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_P_Mute.setClickedOnPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_P_Mute.setValue(_f_mute);
    btn_P_volDown.begin( 1 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_volDown.setDefaultPicturePath("/btn/Button_Volume_Down_Blue.jpg");
                                                                                        btn_P_volDown.setClickedPicturePath("/btn/Button_Volume_Down_Yellow.jpg");
    btn_P_volUp.begin(   2 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_volUp.setDefaultPicturePath("/btn/Button_Volume_Up_Blue.jpg");
                                                                                        btn_P_volUp.setClickedPicturePath("/btn/Button_Volume_Up_Yellow.jpg");
    btn_P_pause.begin(   3 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_pause.setOffPicturePath("/btn/Button_Pause_Blue.jpg");
                                                                                        btn_P_pause.setOnPicturePath("/btn/Button_Right_Blue.jpg");
                                                                                        btn_P_pause.setClickedOffPicturePath("/btn/Button_Pause_Yellow.jpg");
                                                                                        btn_P_pause.setClickedOnPicturePath("/btn/Button_Right_Yellow.jpg");
                                                                                        btn_P_pause.setValue(false);
    btn_P_cancel.begin(  4 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_cancel.setDefaultPicturePath("/btn/Button_Cancel_Red.jpg");
                                                                                        btn_P_cancel.setClickedPicturePath("/btn/Button_Cancel_Yellow.jpg");
    btn_P_prevFile.begin(0 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_prevFile.setDefaultPicturePath("/btn/Button_Left_Blue.jpg");
                                                                                        btn_P_prevFile.setClickedPicturePath("/btn/Button_Left_Yellow.jpg");
    btn_P_nextFile.begin(1 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_nextFile.setDefaultPicturePath("/btn/Button_Right_Blue.jpg");
                                                                                        btn_P_nextFile.setClickedPicturePath("/btn/Button_Right_Yellow.jpg");
    btn_P_ready.begin(   2 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_ready.setDefaultPicturePath("/btn/Button_Ready_Blue.jpg");
                                                                                        btn_P_ready.setClickedPicturePath("/btn/Button_Ready_Yellow.jpg");
    btn_P_playAll.begin( 3 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_playAll.setDefaultPicturePath("/btn/Button_PlayAll_Blue.jpg");
                                                                                        btn_P_playAll.setClickedPicturePath("/btn/Button_PlayAll_Yellow.jpg");
    btn_P_shuffle.begin( 4 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_shuffle.setDefaultPicturePath("/btn/Button_Shuffle_Blue.jpg");
                                                                                        btn_P_shuffle.setClickedPicturePath("/btn/Button_Shuffle_Yellow.jpg");
    btn_P_fileList.begin(6 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_fileList.setDefaultPicturePath("/btn/Button_List_Green.jpg");
                                                                                        btn_P_fileList.setClickedPicturePath("/btn/Button_List_Yellow.jpg");
    btn_P_radio.begin(   7 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_P_radio.setDefaultPicturePath("/btn/Radio_Green.jpg");
                                                                                        btn_P_radio.setClickedPicturePath("/btn/Radio_Yellow.jpg");
    // DLNA --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    btn_D_Mute.begin(    0 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_D_Mute.setOffPicturePath("/btn/Button_Mute_Green.jpg");
                                                                                        btn_D_Mute.setOnPicturePath("/btn/Button_Mute_Red.jpg");
                                                                                        btn_D_Mute.setClickedOffPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_D_Mute.setClickedOnPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_D_Mute.setValue(_f_mute);
    btn_D_volDown.begin( 1 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_D_volDown.setDefaultPicturePath("/btn/Button_Volume_Down_Blue.jpg");
                                                                                        btn_D_volDown.setClickedPicturePath("/btn/Button_Volume_Down_Yellow.jpg");
    btn_D_volUp.begin(   2 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_D_volUp.setDefaultPicturePath("/btn/Button_Volume_Up_Blue.jpg");
                                                                                        btn_D_volUp.setClickedPicturePath("/btn/Button_Volume_Up_Yellow.jpg");
    btn_D_pause.begin(   3 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_D_pause.setOffPicturePath("/btn/Button_Pause_Blue.jpg");
                                                                                        btn_D_pause.setOnPicturePath("/btn/Button_Right_Blue.jpg");
                                                                                        btn_D_pause.setClickedOffPicturePath("/btn/Button_Pause_Yellow.jpg");
                                                                                        btn_D_pause.setClickedOnPicturePath("/btn/Button_Right_Yellow.jpg");
                                                                                        btn_D_pause.setValue(false);
    btn_D_cancel.begin(  4 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_D_cancel.setDefaultPicturePath("/btn/Button_Cancel_Red.jpg");
                                                                                        btn_D_cancel.setClickedPicturePath("/btn/Button_Cancel_Yellow.jpg");
    btn_D_fileList.begin(6 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_D_fileList.setDefaultPicturePath("/btn/Button_List_Green.jpg");
                                                                                        btn_D_fileList.setClickedPicturePath("/btn/Button_List_Yellow.jpg");
    btn_D_radio.begin(   7 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_D_radio.setDefaultPicturePath("/btn/Radio_Green.jpg");
                                                                                        btn_D_radio.setClickedPicturePath("/btn/Radio_Yellow.jpg");
    // CLOCK -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    clk_C_green.begin(       _winDigits.x, _winDigits.y, _winDigits.w, _winDigits.h);   clk_C_green.setTimeFormat(_timeFormat);
    btn_C_alarm.begin(   0 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_C_alarm.setDefaultPicturePath("/btn/Bell_Green.jpg");
                                                                                        btn_C_alarm.setClickedPicturePath("/btn/Bell_Yellow.jpg");
    btn_C_radio.begin(   1 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_C_radio.setDefaultPicturePath("/btn/Radio_Green.jpg");
                                                                                        btn_C_radio.setClickedPicturePath("/btn/Radio_Yellow.jpg");
    btn_C_Mute.begin(    2 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_C_Mute.setOffPicturePath("/btn/Button_Mute_Green.jpg");
                                                                                        btn_C_Mute.setOnPicturePath("/btn/Button_Mute_Red.jpg");
                                                                                        btn_C_Mute.setClickedOffPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_C_Mute.setClickedOnPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_C_Mute.setValue(_f_mute);
    btn_C_volDown.begin( 3 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_C_volDown.setDefaultPicturePath("/btn/Button_Volume_Down_Blue.jpg");
                                                                                        btn_C_volDown.setClickedPicturePath("/btn/Button_Volume_Down_Yellow.jpg");
    btn_C_volUp.begin(   4 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_C_volUp.setDefaultPicturePath("/btn/Button_Volume_Up_Blue.jpg");
                                                                                        btn_C_volUp.setClickedPicturePath("/btn/Button_Volume_Up_Yellow.jpg");
    // ALARM -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    clk_A_red.begin(          _winAlarm.x, _winAlarm.y, _winAlarm.w, _winAlarm.h);      clk_A_red.setAlarmTimeAndDays(&_alarmdays, _alarmtime);
    btn_A_left.begin(    0 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_A_left.setDefaultPicturePath("/btn/Button_Left_Blue.jpg");
                                                                                        btn_A_left.setClickedPicturePath("/btn/Button_Left_Yellow.jpg");
    btn_A_right.begin(   1 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_A_right.setDefaultPicturePath("/btn/Button_Right_Blue.jpg");
                                                                                        btn_A_right.setClickedPicturePath("/btn/Button_Right_Yellow.jpg");
    btn_A_up.begin(      2 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_A_up.setDefaultPicturePath("/btn/Button_Up_Blue.jpg");
                                                                                        btn_A_up.setClickedPicturePath("/btn/Button_Up_Yellow.jpg");
    btn_A_down.begin(    3 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_A_down.setDefaultPicturePath("/btn/Button_Down_Blue.jpg");
                                                                                        btn_A_down.setClickedPicturePath("/btn/Button_Down_Yellow.jpg");
    btn_A_ready.begin(   4 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_A_ready.setDefaultPicturePath("/btn/Button_Ready_Blue.jpg");
                                                                                        btn_A_ready.setClickedPicturePath("/btn/Button_Ready_Yellow.jpg");
    // EQUALIZER ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    sdr_E_lowPass.begin(  _sdrLP.x,  _sdrLP.y,  _sdrLP.w,  _sdrLP.h, -40,  6);          sdr_E_lowPass.setValue(_toneLP);
    sdr_E_bandPass.begin( _sdrBP.x,  _sdrBP.y,  _sdrBP.w,  _sdrBP.h, -40,  6);          sdr_E_bandPass.setValue(_toneBP);
    sdr_E_highPass.begin( _sdrHP.x,  _sdrHP.y,  _sdrHP.w,  _sdrHP.h, -40,  6);          sdr_E_highPass.setValue(_toneHP);
    sdr_E_balance.begin( _sdrBAL.x, _sdrBAL.y, _sdrBAL.w, _sdrBAL.h, -16, 16);          sdr_E_balance.setValue(_toneBAL);
    txt_E_lowPass.begin(  _sdrLP.x +  _sdrLP.w + 2,  _sdrLP.y, 80, _sdrLP.h);           txt_E_lowPass.setFont(_fonts[2]);
    txt_E_bandPass.begin( _sdrBP.x +  _sdrBP.w + 2,  _sdrBP.y, 80, _sdrBP.h);           txt_E_bandPass.setFont(_fonts[2]);
    txt_E_highPass.begin( _sdrHP.x +  _sdrHP.w + 2,  _sdrHP.y, 80, _sdrHP.h);           txt_E_highPass.setFont(_fonts[2]);
    txt_E_balance.begin( _sdrBAL.x + _sdrBAL.w + 2, _sdrBAL.y, 80, _sdrBAL.h);          txt_E_balance.setFont(_fonts[2]);
    btn_E_lowPass.begin( _sdrLP.x - 60, _sdrLP.y + 1, 48, 48);                           btn_E_lowPass.setDefaultPicturePath("/btn/LP_green.jpg");
                                                                                        btn_E_lowPass.setClickedPicturePath("/btn/LP_yellow.jpg");
    btn_E_bandPass.begin(_sdrBP.x - 60, _sdrBP.y + 1, 48, 48);                          btn_E_bandPass.setDefaultPicturePath("/btn/BP_green.jpg");
                                                                                        btn_E_bandPass.setClickedPicturePath("/btn/BP_yellow.jpg");
    btn_E_highPass.begin(_sdrHP.x - 60, _sdrHP.y + 1, 48, 48);                          btn_E_highPass.setDefaultPicturePath("/btn/HP_green.jpg");
                                                                                        btn_E_highPass.setClickedPicturePath("/btn/HP_yellow.jpg");
    btn_E_balance.begin(_sdrBAL.x - 60, _sdrBAL.y + 1, 48, 48);                         btn_E_balance.setDefaultPicturePath("/btn/BAL_green.jpg");
                                                                                        btn_E_balance.setClickedPicturePath("/btn/BAL_yellow.jpg");
    btn_E_Radio.begin(   0 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_E_Radio.setDefaultPicturePath("/btn/Radio_Green.jpg");
                                                                                        btn_E_Radio.setClickedPicturePath("/btn/Radio_Yellow.jpg");
    btn_E_Player.begin(  1 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_E_Player.setDefaultPicturePath("/btn/Player_Green.jpg");
                                                                                        btn_E_Player.setClickedPicturePath("/btn/Player_Yellow.jpg");
    btn_E_Mute.begin(    2 * _winButton.w, _winButton.y, _winButton.w, _winButton.h);   btn_E_Mute.setOffPicturePath("/btn/Button_Mute_Green.jpg");
                                                                                        btn_E_Mute.setOnPicturePath("/btn/Button_Mute_Red.jpg");
                                                                                        btn_E_Mute.setClickedOffPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_E_Mute.setClickedOnPicturePath("/btn/Button_Mute_Yellow.jpg");
                                                                                        btn_E_Mute.setValue(_f_mute);
}
// clang-format off

/*         ╔═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
           ║                                                                              C H A N G E    S T A T E                                                                       ║
           ╚═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝   */

// clang-format on
void changeState(int32_t state){
//    log_i("new state %i, current state %i radioSubMenue %i", state, _state, _radioSubmenue);
    switch(_state){
        case RADIO:     btn_R_Mute.disable();     btn_R_volDown.disable();  btn_R_volUp.disable();    btn_R_prevSta.disable(); btn_R_nextSta.disable();
                        btn_R_staList.disable();  btn_R_player.disable();   btn_R_dlna.disable();     btn_R_clock.disable();   btn_R_sleep.disable();
                        btn_R_bright.disable();   btn_R_equal.disable();
                        hideVUmeter(); break;
        case PLAYER:    btn_P_Mute.disable();     btn_P_volDown.disable();  btn_P_volUp.disable();    btn_P_pause.disable();   btn_P_cancel.disable();
                        btn_P_prevFile.disable(); btn_P_nextFile.disable(); btn_P_ready.disable();    btn_P_playAll.disable(); btn_P_shuffle.disable();
                        btn_P_fileList.hide();    btn_P_radio.hide();
                        break;
        case DLNA:      btn_D_Mute.disable();     btn_D_volDown.disable();  btn_D_volUp.disable();    btn_D_pause.disable();   btn_D_cancel.disable();
                        btn_D_fileList.disable(); btn_D_radio.disable();
                        break;
        case CLOCK:     btn_C_Mute.disable();     btn_C_alarm.disable();    btn_C_radio.disable();    btn_C_volDown.disable(); btn_C_volUp.disable();
                        clk_C_green.disable();
                        break;
        case ALARM:     clk_A_red.disable();      btn_A_left.disable();     btn_A_right.disable();    btn_A_up.disable();      btn_A_down.disable();
                        btn_A_ready.disable();
                        break;
        case EQUALIZER: sdr_E_lowPass.disable();  sdr_E_bandPass.disable(); sdr_E_highPass.disable(); sdr_E_balance.disable();
                        btn_E_lowPass.disable();  btn_E_bandPass.disable(); btn_E_highPass.disable(); btn_E_balance.disable(); btn_E_Radio.disable();
                        txt_E_lowPass.disable();  txt_E_bandPass.disable(); txt_E_highPass.disable(); txt_E_balance.disable(); btn_E_Player.disable();
                        btn_E_Mute.disable();
                        break;
    }
    _f_volBarVisible = false;
    if(_timeCounter.timer){
        _timeCounter.timer = 0;
        showFooterRSSI(true);
    }

    switch(state) {
        case RADIO:{
            if(_state != RADIO) clearWithOutHeaderFooter();
            if(_radioSubmenue == 0){
                showHeadlineItem(RADIO);
                if(_state != RADIO) showLogoAndStationName();
                showVUmeter();
                _f_newStreamTitle = true;
                _timeCounter.timer = 0;
            }
            if(_radioSubmenue == 1){
                clearTitle();
                hideVUmeter();
                showVolumeBar();
                btn_R_Mute.show();      btn_R_volDown.show();          btn_R_volUp.show();
                btn_R_prevSta.show();   btn_R_nextSta.show();          btn_R_staList.show();
                _timeCounter.timer = 5;
                _timeCounter.factor = 2.0;
            }
            if(_radioSubmenue == 2){
                hideVUmeter();
                clearVolBar();
                btn_R_player.show();    btn_R_dlna.show();             btn_R_clock.show();
                btn_R_sleep.show();     btn_R_bright.show();           btn_R_equal.show();
                _timeCounter.timer = 5;
                _timeCounter.factor = 2.0;
            }
            if(_state != RADIO) webSrv.send("changeState=", "RADIO");
            break;
        }

        case STATIONSLIST:{
            showStationsList(_staListNr);
            _timeCounter.timer = 10;
            _timeCounter.factor = 1.0;
            break;
        }

        case PLAYER: {
            if(_state != PLAYER) clearWithOutHeaderFooter();
            showHeadlineItem(PLAYER);
            if(_playerSubmenue == 0){
                SD_listDir(_curAudioFolder.c_str(), true, true);
                _cur_Codec = 0;
                showFileLogo(PLAYER);
                showFileName(_SD_content[_cur_AudioFileNr]);
                showAudioFileNumber();
                if(_state != PLAYER) webSrv.send("changeState=", "PLAYER");
                showAudioFileNumber();
                btn_P_prevFile.show();  btn_P_nextFile.show();  btn_P_ready.show(); btn_P_playAll.show(); btn_P_shuffle.show(); btn_P_fileList.show(); btn_P_radio.show();
            }
            if(_playerSubmenue == 1){
                btn_P_Mute.show();    btn_P_volDown.show();  btn_P_volUp.show();    btn_P_pause.show();   btn_P_cancel.show();
            }
            break;
        }
        case DLNA:{
            clearWithOutHeaderFooter();
            showFileLogo(state);
            showHeadlineItem(DLNA);
            showVolumeBar();
            btn_D_Mute.show();    btn_D_volDown.show();  btn_D_volUp.show();    btn_D_pause.show();   btn_D_cancel.show(); btn_D_fileList.show(); btn_D_radio.show();
            break;
        }
        case DLNAITEMSLIST:{
            showDlnaItemsList(_currDLNAsrvNr, "");
            _timeCounter.timer = 10;
            _timeCounter.factor = 1.0;
            break;
        }
        case CLOCK:{
            showHeadlineItem(CLOCK);
            if(_clockSubMenue == 0){
                _timeCounter.timer = 0;
                if(_state != CLOCK){
                    clearWithOutHeaderFooter();
                    clk_C_green.updateTime(rtc.getMinuteOfTheDay(), rtc.getweekday());
                }
                else{
                    btn_C_Mute.hide();     btn_C_alarm.hide();    btn_C_radio.hide();    btn_C_volDown.hide(); btn_C_volUp.hide();
                    clearVolBar();
                }
                clk_C_green.show();
            }
            if(_clockSubMenue == 1){
                btn_C_Mute.show();     btn_C_alarm.show();    btn_C_radio.show();    btn_C_volDown.show(); btn_C_volUp.show();
                _timeCounter.timer = 5;
                _timeCounter.factor = 2.0;
            }
            break;
        }
        case ALARM:{
            showHeadlineItem(ALARM);
            clearWithOutHeaderFooter();
            clk_A_red.show();          btn_A_left.show();     btn_A_right.show();    btn_A_up.show();      btn_A_down.show();
            btn_A_ready.show();
            break;
        }
        case BRIGHTNESS:{
            showHeadlineItem(BRIGHTNESS);
            _pressBtn[0] = "/btn/Button_Left_Yellow.jpg";        _releaseBtn[0] = "/btn/Button_Left_Blue.jpg";
            _pressBtn[1] = "/btn/Button_Right_Yellow.jpg";       _releaseBtn[1] = "/btn/Button_Right_Blue.jpg";
            _pressBtn[2] = "/btn/Button_Ready_Yellow.jpg";       _releaseBtn[2] = "/btn/Button_Ready_Blue.jpg";
            _pressBtn[3] = "/btn/Black.jpg";                     _releaseBtn[3] = "/btn/Black.jpg";
            _pressBtn[4] = "/btn/Black.jpg";                     _releaseBtn[4] = "/btn/Black.jpg";
            _pressBtn[5] = "/btn/Black.jpg";                     _releaseBtn[5] = "/btn/Black.jpg";
            _pressBtn[6] = "/btn/Black.jpg";                     _releaseBtn[6] = "/btn/Black.jpg";
            _pressBtn[7] = "/btn/Black.jpg";                     _releaseBtn[7] = "/btn/Black.jpg";
            drawImage("/common/Brightness.jpg", 0, _winName.y);
            showBrightnessBar();
            for(int32_t i = 0; i < 8 ; i++) {drawImage(_releaseBtn[i], i * _winButton.w, _winButton.y);}
            break;
        }
        case AUDIOFILESLIST:{
            showAudioFilesList(_fileListNr);
            _timeCounter.timer = 10;
            _timeCounter.factor = 1.0;
            break;
        }
        case SLEEP:{
            showHeadlineItem(SLEEP);
            _pressBtn[0] = "/btn/Button_Up_Yellow.jpg";          _releaseBtn[0] = "/btn/Button_Up_Blue.jpg";
            _pressBtn[1] = "/btn/Button_Down_Yellow.jpg";        _releaseBtn[1] = "/btn/Button_Down_Blue.jpg";
            _pressBtn[2] = "/btn/Button_Ready_Yellow.jpg";       _releaseBtn[2] = "/btn/Button_Ready_Blue.jpg";
            _pressBtn[3] = "/btn/Black.jpg";                     _releaseBtn[3] = "/btn/Black.jpg";
            _pressBtn[4] = "/btn/Button_Cancel_Yellow.jpg";      _releaseBtn[4] = "/btn/Button_Cancel_Blue.jpg";
            _pressBtn[5] = "/btn/Black.jpg";                     _releaseBtn[5] = "/btn/Black.jpg";
            _pressBtn[6] = "/btn/Black.jpg";                     _releaseBtn[6] = "/btn/Black.jpg";
            _pressBtn[7] = "/btn/Black.jpg";                     _releaseBtn[7] = "/btn/Black.jpg";
            clearLogoAndStationname();
            clearTitle();
            display_sleeptime();
            if(TFT_CONTROLLER < 2) drawImage("/common/Night_Gown.bmp", 198, 23);
            else                   drawImage("/common/Night_Gown.bmp", 280, 45);
            for(int32_t i = 0; i < 5 ; i++) {drawImage(_releaseBtn[i], i * _winButton.w, _winButton.y);}
            break;
        }

        case EQUALIZER:{
            if(_state != EQUALIZER) clearWithOutHeaderFooter();
            showHeadlineItem(EQUALIZER);
            sdr_E_lowPass.show(); sdr_E_bandPass.show(); sdr_E_highPass.show(); sdr_E_balance.show();
            btn_E_lowPass.show(); btn_E_bandPass.show(); btn_E_highPass.show(); btn_E_balance.show(); btn_E_Radio.show(); btn_E_Player.show(); btn_E_Mute.show();
            txt_E_lowPass.show(); txt_E_bandPass.show(); txt_E_highPass.show(); txt_E_balance.show();
            break;
        }
    }
    _state = state;
}
// clang-format on
/*****************************************************************************************************************************************************
 *                                                                D L N A                                                                            *
 *****************************************************************************************************************************************************/

void showDlnaItemsList(uint16_t itemListNr, const char* parentName) {

    uint16_t                  itemsSize = 0;
    DLNA_Client::dlnaServer_t dlnaServer = dlna.getServer();
    DLNA_Client::srvContent_t srvContent = dlna.getBrowseResult();
    if(_dlnaLevel == 0) {
        itemsSize = dlnaServer.size;
        itemListNr = 0;
    }                                     // DLNA Serverlist
    else { itemsSize = srvContent.size; } // DLNA Contentlist

    auto triangleUp = [&](int16_t x, int16_t y, uint8_t s) {  tft.fillTriangle(x + s, y + 0, x + 0, y + 2  *  s, x + 2  *  s, y + 2  *  s, TFT_RED); };
    auto triangleDown = [&](int16_t x, int16_t y, uint8_t s) {  tft.fillTriangle(x + 0, y + 0, x + 2  *  s, y + 0, x + s, y + 2  *  s, TFT_RED); };

    clearWithOutHeaderFooter();
    showHeadlineItem(DLNA);
    tft.setFont(_fonts[0]);
    uint8_t lineHight = _winWoHF.h / 10;
    tft.setTextColor(TFT_ORANGE);
    tft.writeText(_dlnaHistory[_dlnaLevel].name, 10, _winHeader.h, _dispWidth - 10, lineHight, TFT_ALIGN_LEFT, true, true);
    tft.setTextColor(TFT_WHITE);
    for(uint8_t pos = 1; pos < 10; pos++) {
        if(pos == 1 && itemListNr > 0) { triangleUp(0, _winHeader.h + (pos * lineHight), lineHight / 3.5); }
        if(pos == 9 && itemListNr + 9 < _dlnaMaxItems) { triangleDown(0, _winHeader.h + (pos * lineHight), lineHight / 3.5); }
        if(pos > 9) break;
        if(pos > itemsSize) break;
        if(_dlnaLevel == 0) { tft.writeText(dlnaServer.friendlyName[pos - 1], 20, _winFooter.h + (pos)*lineHight, _dispWidth - 20, lineHight, TFT_ALIGN_LEFT, true, true); }
        else {
            if(startsWith(srvContent.itemURL[pos - 1], "http")) {
                if(srvContent.isAudio[pos - 1] == true) {
                    if(srvContent.duration[pos - 1][0] != '?') { sprintf(_chbuf, ANSI_ESC_YELLOW "%s" ANSI_ESC_CYAN " (%s)", srvContent.title[pos - 1], srvContent.duration[pos - 1]); }
                    else { sprintf(_chbuf, ANSI_ESC_YELLOW "%s" ANSI_ESC_CYAN " (%li)", srvContent.title[pos - 1], (long int)srvContent.itemSize[pos - 1]); }
                }
                else { sprintf(_chbuf, ANSI_ESC_WHITE "%s" ANSI_ESC_CYAN " (%li)", srvContent.title[pos - 1], (long int)srvContent.itemSize[pos - 1]); }
            }
            else {
                if(srvContent.childCount[pos - 1] == 0) { sprintf(_chbuf, ANSI_ESC_WHITE "%s", srvContent.title[pos - 1]); }
                else { sprintf(_chbuf, ANSI_ESC_WHITE "%s" ANSI_ESC_CYAN " (%i)", srvContent.title[pos - 1], srvContent.childCount[pos - 1]); }
            }
            tft.writeText(_chbuf, 20, _winFooter.h + (pos)*lineHight, _dispWidth - 20, lineHight, TFT_ALIGN_LEFT, true, true);
        }
    }

    _timeCounter.timer = 10;
    _timeCounter.factor = 1.0;
}

/*****************************************************************************************************************************************************
 *                                                                 L O O P                                                                           *
 *****************************************************************************************************************************************************/
void loop() {
    if(!_f_ESPfound) return;    // Guard:  wrong chip?
    if(!_f_SD_MMCfound) return; // Guard:  SD_MMC could not be initialisized
    webSrv.loop();
    ir.loop();
    tp.loop();
    ftpSrv.handleFTP();
    ArduinoOTA.handle();
    dlna.loop();
    bt_emitter.loop();

    if(_f_dlnaBrowseServer) {
        _f_dlnaBrowseServer = false;
        dlna.browseServer(_currDLNAsrvNr, _dlnaHistory[_dlnaLevel].objId, _totalNumberReturned);
    }

    if(!_f_sleeping) {
        if(_f_newBitRate) {
            showFooterBitRate(_icyBitRate);
            _f_newBitRate = false;
        }

        if(_f_newLogoAndStation) {
            showLogoAndStationName();
            _f_newLogoAndStation = false;
        }

        if(_f_100ms) {
            _f_100ms = false;
            if(_state != ALARM) updateVUmeter();
        }
    }

    if(_f_1sec) {
        _f_1sec = false;
        clk_C_green.updateTime(rtc.getMinuteOfTheDay(), rtc.getweekday());

        {   // ALARM MANAGEMENT
            if(!_semaphore) { _f_alarm = clk_C_green.isAlarm(_alarmdays, _alarmtime); }
            if(_f_alarm)         {_semaphore++;}
            if(_semaphore)       {_semaphore++;}
            if(_semaphore >= 65) { _semaphore = 0;}
            if(_f_alarm) {
                _f_alarm = false;
                void hideVUmeter();
                clearAll();
                showHeadlineItem(ALARM);
                showHeadlineTime();
                showFooter();
                showFileName("ALARM");
                drawImage("/common/Alarm.jpg", _winLogo.x, _winLogo.y);
                setTFTbrightness(_brightness);
                SerialPrintfln(ANSI_ESC_MAGENTA "Alarm");
                audioSetVolume(21);
                muteChanged(false);
                connecttoFS("/ring/alarm_clock.mp3");
            }
            if(_f_eof_alarm) { // AFTER RINGING
                _f_eof_alarm = false;
                showVUmeter();
                audioSetVolume(_cur_volume);
                showHeadlineVolume();
                wake_up();
                _radioSubmenue = 0;
                changeState(RADIO);
                connecttohost(_lastconnectedhost.c_str());
                showLogoAndStationName();
            }
        }   // END ALARM MANAGEMENT

        if(!_f_sleeping) {
            showHeadlineTime(false);
            showFooterRSSI();
        }

        if(_timeCounter.timer) {
        //    log_w("timeCounter.timer  %i", _timeCounter.timer);
            _timeCounter.timer--;
            if(_timeCounter.timer < 10) {
                sprintf(_chbuf, "/common/tc%02d.bmp", uint8_t(_timeCounter.timer * _timeCounter.factor));
                drawImage(_chbuf, _winRSSID.x, _winRSSID.y + 2);
            }
            if(!_timeCounter.timer) {
                showFooterRSSI(true);
                if(_state == RADIO) {
                    _radioSubmenue = 0;
                    changeState(RADIO);
                }
                else if(_state == CLOCK) {
                    _clockSubMenue = 0;
                    changeState(CLOCK);
                }
                //    else if(_state == RADIO && _f_switchToClock) { changeState(CLOCK); _f_switchToClock = false; }
                else if(_state == STATIONSLIST) { changeState(RADIO); }
                else if(_state == AUDIOFILESLIST) { changeState(PLAYER); }
                else if(_state == DLNAITEMSLIST) { changeState(DLNA); }
                else { ; } // all other, do nothing
            }
        }
        if(_f_rtc == true) { // true -> rtc has the current time
            int8_t h = 0;
            String time_s;
            xSemaphoreTake(mutex_rtc, portMAX_DELAY);
            time_s = rtc.gettime_s();
            xSemaphoreGive(mutex_rtc);

            // if(_f_eof && _state == PLAYER) {
            //     if(!_f_playlistEnabled) {
            //         _f_eof = false;
            //         changeState(PLAYER);
            //     }
            // }

            if((_f_mute == false) && (!_f_sleeping)) {
                if(time_s.endsWith("59:53") && _state == RADIO) { // speech the time 7 sec before a new hour is arrived
                    String hour = time_s.substring(0, 2);         // extract the hour
                    h = hour.toInt();
                    h++;
                    if(h == 24) h = 0;
                    if(_f_timeAnnouncement) {
                        if(_timeFormat == 12)
                            if(h > 12) h -= 12;
                        sprintf(_chbuf, "/voice_time/%d_00.mp3", h);
                        SerialPrintfln("Time: ...... play Audiofile %s", _chbuf) connecttoFS(_chbuf);
                    }
                    else { SerialPrintfln("Time: ...... Announcement at %d o'clock is silent", h); }
                }
            }



            if(_f_hpChanged) {
                setVolume(_cur_volume);
                if(!digitalRead(HP_DETECT)) { SerialPrintfln("Headphone plugged in"); }
                else { SerialPrintfln("Headphone unplugged"); }
                _f_hpChanged = false;
            }
            if(audioIsRunning() && _f_isFSConnected) {
                _audioCurrentTime = audioGetCurrentTime();
                _audioFileDuration = audioGetFileDuration();
                if(_audioFileDuration) {
                    SerialPrintfcr("AUDIO_FILE:  " ANSI_ESC_GREEN "AudioCurrentTime " ANSI_ESC_GREEN "%li:%02lis, " ANSI_ESC_GREEN "AudioFileDuration " ANSI_ESC_GREEN "%li:%02lis",
                                   _audioCurrentTime / 60, _audioCurrentTime % 60, _audioFileDuration / 60, _audioFileDuration % 60);
                }
            }
        }
        if(_commercial_dur > 0) {
            _commercial_dur--;
            if((_commercial_dur == 2) && (_state == RADIO)) clearStreamTitle(); // end of commercial? clear streamtitle
        }
        if(_f_newStreamTitle && !_timeCounter.timer) {
            _f_newStreamTitle = false;
            if(_state == RADIO) {
                if(strlen(_streamTitle)) showStreamTitle(_streamTitle);
                else if(strlen(_icyDescription)) {
                    showStreamTitle(_icyDescription);
                    _f_newIcyDescription = false;
                    webSrv.send("icy_description=", _icyDescription);
                }
                else clearStreamTitle();
            }
            webSrv.send("streamtitle=", _streamTitle);
        }
        if(_f_newIcyDescription && !_timeCounter.timer) {
            if(_state == RADIO) {
                if(!strlen(_streamTitle)) showStreamTitle(_icyDescription);
            }
            webSrv.send("icy_description=", _icyDescription);
            _f_newIcyDescription = false;
        }

        if(_f_newCommercial && !_timeCounter.timer) {
            if(_state == RADIO) { showStreamTitle(_commercial); }
            webSrv.send("streamtitle=", _commercial);
            _f_newCommercial = false;
        }

        if(_cur_Codec == 0) {
            uint8_t c = audioGetCodec();
            if(c != 0 && c != 8 && c < 10) { // unknown or OGG, guard: c {1 ... 7, 9}
                _cur_Codec = c;
                SerialPrintfln("Audiocodec:  " ANSI_ESC_YELLOW "%s", codecname[c]);
                if(_state == PLAYER) showFileLogo(PLAYER);
                if(_state == RADIO && _f_logoUnknown == true) {
                    _f_logoUnknown = false;
                    showFileLogo(_state);
                }
            }
        }

        if(_f_isFSConnected) {
            //    uint32_t t = 0;
            //    uint32_t fs = audioGetFileSize();
            //    uint32_t br = audioGetBitRate();
            //    if(br) t = (fs * 8)/ br;
            //    log_w("Br %d, Dur %ds", br, t);
        }

        if(_f_dlnaSeekServer) {
            _f_dlnaSeekServer = false;
            dlna.seekServer();
        }
    }

    if(_f_10sec == true) {
        _f_10sec = false;
        if(_state == RADIO && !_icyBitRate && !_f_sleeping) {
            uint32_t ibr = audioGetBitRate() / 1000;
            if(ibr > 0) {
                if(ibr != _avrBitRate) {
                    _avrBitRate = ibr;
                    showFooterBitRate(_avrBitRate);
                }
            }
        }
        updateSettings();
    }

    if(_f_1min == true) {
        _f_1min = false;
        updateSleepTime();
    }

    if(_f_clearLogo) {
        clearLogo();
        _f_clearLogo = false;
    }

    if(_f_clearStationName) {
        clearStationName();
        _f_clearStationName = false;
    }

    if(_f_playlistEnabled) {
        if(!_f_playlistNextFile) {
            if(!audioIsRunning() && !_f_pauseResume) {
                SerialPrintfln("AUDIO_info:  " ANSI_ESC_GREEN "next playlist file");
                processPlaylist(false);
                _playlistTime = millis();
                _f_playlistNextFile = true;
            }
        }
        else {
            if(_playlistTime + 5000 < millis()) _f_playlistNextFile = false;
        }
    }

    if(Serial.available()) { // input: serial terminal
        String r = Serial.readString();
        r.replace("\n", "");
        SerialPrintfln("Terminal  :  " ANSI_ESC_YELLOW "%s", r.c_str());
        if(r.startsWith("p")) {
            bool res = audioPauseResume();
            if(res) { SerialPrintfln("Terminal   : " ANSI_ESC_YELLOW "Pause-Resume"); }
            else { SerialPrintfln("Terminal   : " ANSI_ESC_YELLOW "Pause-Resume not possible"); }
        }
        if(r.startsWith("h")) { // A hardcopy of the display is created and written to the SD card
            { SerialPrintfln("Terminal   : " ANSI_ESC_YELLOW "create hardcopy"); }
            hardcopy();
        }
        if(r.startsWith("m-")) {
            { SerialPrintfln("Terminal   : " ANSI_ESC_YELLOW "mute decrement"); }
            audioMute(true);
        }
        if(r.startsWith("m+")) {
            { SerialPrintfln("Terminal   : " ANSI_ESC_YELLOW "mute increment"); }
            audioMute(false);
        }

        if(r.toInt() != 0) { // is integer?
            if(audioSetTimeOffset(r.toInt())) { SerialPrintfln("Terminal   : " ANSI_ESC_YELLOW "TimeOffset %li", r.toInt()); }
            else { SerialPrintfln("Terminal   : " ANSI_ESC_YELLOW "TimeOffset not possible"); }
        }
    }
}

/*         ╔═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
           ║                                                                                  E V E N T S                                                                                ║
           ╚═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝   */

// Events from audioI2S library
void audio_info(const char* info) {
    if(startsWith(info, "Request"))                {SerialPrintflnCut("AUDIO_info:  ", ANSI_ESC_RED, info); return;}
    if(startsWith(info, "FLAC"))                   {SerialPrintflnCut("AUDIO_info:  ", ANSI_ESC_GREEN, info); return;}
    if(endsWith(  info, "Stream lost"))            {SerialPrintflnCut("AUDIO_info:  ", ANSI_ESC_RED, info); return;}
    if(startsWith(info, "authent"))                {SerialPrintflnCut("AUDIO_info:  ", ANSI_ESC_GREEN, info); return;}
    if(startsWith(info, "StreamTitle="))           {return;}
    if(startsWith(info, "HTTP/") && info[9] > '3') {SerialPrintflnCut("AUDIO_info:  ", ANSI_ESC_RED, info); return;}
    if(CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_WARN) {SerialPrintflnCut("AUDIO_info:  ", ANSI_ESC_GREEN, info); return;} // all other
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_showstation(const char* info) {
    _stationName_air = info;
    if(strlen(info)) SerialPrintfln("StationName: " ANSI_ESC_MAGENTA "%s", info);
    if(!_cur_station) _f_newLogoAndStation = true;
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_showstreamtitle(const char* info) {
    strcpy(_streamTitle, info);
    if(!_f_irNumberSeen) _f_newStreamTitle = true;
    SerialPrintfln("StreamTitle: " ANSI_ESC_YELLOW "%s", info);
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void show_ST_commercial(const char* info) {
    _commercial_dur = atoi(info) / 1000; // info is the duration of advertising in ms
    char cdur[10];
    itoa(_commercial_dur, cdur, 10);
    if(_f_newCommercial) return;
    strcpy(_commercial, "Advertising: ");
    strcat(_commercial, cdur);
    strcat(_commercial, "s");
    _f_newCommercial = true;
    SerialPrintfln("StreamTitle: %s", info);
}
void audio_commercial(const char* info) { show_ST_commercial(info); }
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_eof_mp3(const char* info) { // end of mp3 file (filename)
    if(startsWith(info, "alarm")) _f_eof_alarm = true;
    SerialPrintflnCut("end of file: ", ANSI_ESC_YELLOW, info);
    if(_state == PLAYER) {
        if(!_f_playlistEnabled) {
            _f_clearLogo = true;
            _f_clearStationName = true;
        }
    }
    webSrv.send("SD_playFile=", "end of audiofile");
    _f_eof = true;
    _f_isFSConnected = false;
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_eof_stream(const char* info) {
    _f_isWebConnected = false;
    SerialPrintflnCut("end of file: ", ANSI_ESC_YELLOW, info);
    if(_state == PLAYER) {
        if(!_f_playlistEnabled) {
            _f_clearLogo = true;
            _f_clearStationName = true;
        }
    }
    if(_state == DLNA) { showFileName(""); }
    if(_state == RADIO) { clearWithOutHeaderFooter(); }
    _f_eof = true;
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_lasthost(const char* info) { // really connected URL
    if(_f_playlistEnabled) return;
    _lastconnectedhost = info;
    SerialPrintflnCut("lastURL: ..  ", ANSI_ESC_WHITE, _lastconnectedhost.c_str());
    webSrv.send("stationURL=", _lastconnectedhost);
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_icyurl(const char* info) { // if the Radio has a homepage, this event is calling
    if(strlen(info) > 5) {
        SerialPrintflnCut("icy-url: ..  ", ANSI_ESC_WHITE, info);
        _homepage = String(info);
        if(!_homepage.startsWith("http")) _homepage = "http://" + _homepage;
    }
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_icylogo(const char* info) { // if the Radio has a homepage, this event is calling
    if(strlen(info) > 5) { SerialPrintflnCut("icy-logo:    ", ANSI_ESC_WHITE, info); }
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_id3data(const char* info) { SerialPrintfln("id3data: ..  " ANSI_ESC_GREEN "%s", info); }
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_id3image(File& audiofile, const size_t APIC_pos, const size_t APIC_size) { SerialPrintfln("CoverImage:  " ANSI_ESC_GREEN "Position %i, Size %i bytes", APIC_pos, APIC_size); }
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_oggimage(File& audiofile, std::vector<uint32_t> vec) { // OGG blockpicture
    SerialPrintfln("oggimage:..  " ANSI_ESC_GREEN "---------------------------------------------------------------------------");
    SerialPrintfln("oggimage:..  " ANSI_ESC_GREEN "ogg metadata blockpicture found:");
    for(int i = 0; i < vec.size(); i += 2) { SerialPrintfln("oggimage:..  " ANSI_ESC_GREEN "segment %02i, pos %07ld, len %05ld", i / 2, (long unsigned int)vec[i], (long unsigned int)vec[i + 1]); }
    SerialPrintfln("oggimage:..  " ANSI_ESC_GREEN "---------------------------------------------------------------------------");
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_icydescription(const char* info) {
    strcpy(_icyDescription, info);
    _f_newIcyDescription = true;
    if(strlen(info)) SerialPrintfln("icy-descr:   %s", info);
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void audio_bitrate(const char* info) {
    if(!strlen(info)) return; // guard
    _icyBitRate = str2int(info) / 1000;
    _f_newBitRate = true;
    SerialPrintfln("bitRate:     " ANSI_ESC_CYAN "%iKbit/s", _icyBitRate);
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void ftp_debug(const char* info) {
    if(startsWith(info, "File Name")) return;
    SerialPrintfln("ftpServer:   %s", info);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
void RTIME_info(const char* info) { SerialPrintfln("rtime_info:  %s", info); }

// Events from tft library
void tft_info(const char* info) { SerialPrintfln("tft_info: .  %s", info); }

//----------------------------------------------------------------------------------------------------------------------------------------------------
// Events from IR Library
void ir_code(uint8_t addr, uint8_t cmd) {
    SerialPrintfln("ir_code: ..  " ANSI_ESC_YELLOW "IR address " ANSI_ESC_BLUE "0x%02x, " ANSI_ESC_YELLOW "IR command " ANSI_ESC_BLUE "0x%02x", addr, cmd);
    char buf[20];
    sprintf(buf, "0x%02x", addr);
    webSrv.send("IR_address=", buf);
    sprintf(buf, "0x%02x", cmd);
    webSrv.send("IR_command=", buf);
}

void ir_res(uint32_t res) {
    _f_irNumberSeen = false;
    if(_state != RADIO) return;
    if(_f_sleeping == true) return;
    tft.fillRect(_winLogo.x, _winLogo.y, _dispWidth, _winName.h + _winTitle.h, TFT_BLACK);
    SerialPrintfln("ir_result:   " ANSI_ESC_YELLOW "Stationnumber " ANSI_ESC_BLUE "%lu", (long unsigned)res);
    if(res != 0) { setStation(res); } // valid between 1 ... 999
    else { setStation(_cur_station); }
    showVUmeter();
    return;
}
void ir_number(uint16_t num) {
    if(_state != RADIO) return;
    if(_f_sleeping) return;
    _f_irNumberSeen = true;
    tft.fillRect(_winLogo.x, _winLogo.y, _dispWidth, _winName.h + _winTitle.h, TFT_BLACK);
    tft.setFont(_fonts[8]);
    tft.setTextColor(TFT_GOLD);
    char buf[10];
    itoa(num, buf, 10);
    tft.writeText(buf, 0, _irNumber_y, _dispWidth, _dispHeight, TFT_ALIGN_CENTER, false, true);
}
void ir_key(uint8_t key) {
    if(_f_sleeping == true && key != 10) return;
    if(_f_sleeping == true && key == 10) {
        wake_up();
        return;
    } // awake

    switch(key) {
        case 15:
            if(_state == SLEEP) {
                updateSleepTime(true);
                changeState(RADIO);
                break;
            } // CLOCK <-> RADIO
            if(_state == RADIO) {
                changeState(CLOCK);
                break;
            }
            if(_state == CLOCK) {
                changeState(RADIO);
                break;
            }
            break;
        case 11:
            upvolume(); // VOLUME+
            break;
        case 12:
            downvolume(); // VOLUME-
            break;
        case 14:
            if(_state == RADIO) {
                nextStation();
                break;
            } // NEXT STATION
            if(_state == CLOCK) {
                nextStation();
                changeState(RADIO);
                _f_switchToClock = true;
                break;
            }
            if(_state == SLEEP) {
                display_sleeptime(1);
                break;
            }
            break;
        case 13:
            if(_state == RADIO) {
                prevStation();
                break;
            } // PREV STATION
            if(_state == CLOCK) {
                prevStation();
                changeState(RADIO);
                _f_switchToClock = true;
                break;
            }
            if(_state == SLEEP) {
                display_sleeptime(-1);
                break;
            }
            break;
        case 10:
            //   mute(); break; // MUTE
            break;
        case 16:
            if(_state == RADIO) {
                changeState(SLEEP);
                break;
            } // OFF TIMER
            if(_state == SLEEP) {
                changeState(RADIO);
                break;
            }
            break;
        default: break;
    }
}
void ir_long_key(int8_t key) {
    log_i("long pressed key nr: %i", key);
    if(key == 10) fall_asleep(); // long mute
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
// Event from TouchPad
// clang-format off
void tp_pressed(uint16_t x, uint16_t y) {
    // SerialPrintfln("tp_pressed, state is: %i", _state);
    //  SerialPrintfln(ANSI_ESC_YELLOW "Touchpoint  x=%d, y=%d", x, y);
    enum : int8_t {none = -1,  ALARM_1, BRIGHTNESS_1, CLOCK_1,
                               CLOCKico_1, ALARM_2, SLEEP_1, DLNA_1, DLNAITEMSLIST_1, STATIONSLIST_1, AUDIOFILESLIST_1
    };
    int8_t yPos = none;
    int8_t btnNr = none;     // buttonnumber

    if(_f_sleeping) return;  // awake in tp_released()

    switch(_state) {
        case RADIO:
                if(_radioSubmenue == 1){
                    if(btn_R_Mute.positionXY(x, y)) return;
                    if(btn_R_volDown.positionXY(x, y)) return;
                    if(btn_R_volUp.positionXY(x, y)) return;
                    if(btn_R_prevSta.positionXY(x, y)) return;
                    if(btn_R_nextSta.positionXY(x, y)) return;
                    if(btn_R_staList.positionXY(x, y)) return;
                }
                if(_radioSubmenue == 2){
                    if(btn_R_player.positionXY(x, y)) return;
                    if(btn_R_dlna.positionXY(x, y)) return;
                    if(btn_R_clock.positionXY(x, y)) return;
                    if(btn_R_sleep.positionXY(x, y)) return;
                    if(btn_R_bright.positionXY(x, y)) return;
                    if(btn_R_equal.positionXY(x, y)) return;
                }
                _radioSubmenue++;
                if(_radioSubmenue == 3) _radioSubmenue = 0;
                changeState(RADIO);
                return;
            break;
        case PLAYER:
                if(_playerSubmenue == 0){
                    if(btn_P_prevFile.positionXY(x, y)) return;
                    if(btn_P_nextFile.positionXY(x, y)) return;
                    if(btn_P_ready.positionXY(x, y)) return;;
                    if(btn_P_playAll.positionXY(x, y)) return;
                    if(btn_P_shuffle.positionXY(x, y)) return;
                    if(btn_P_fileList.positionXY(x, y)) return;
                    if(btn_P_radio.positionXY(x, y)) return;
                }
                if(_playerSubmenue == 1){
                    if(btn_P_Mute.positionXY(x, y)) return;
                    if(btn_P_volDown.positionXY(x, y)) return;
                    if(btn_P_volUp.positionXY(x, y)) return;
                    if(btn_P_pause.positionXY(x, y)) return;
                    if(btn_P_cancel.positionXY(x, y)) return;
                }
                break;
        case DLNA:
                if(btn_D_Mute.positionXY(x, y)) return;
                if(btn_D_pause.positionXY(x, y)) return;
                if(btn_D_volDown.positionXY(x, y)) return;
                if(btn_D_volUp.positionXY(x, y)) return;
                if(btn_D_radio.positionXY(x, y)) return;
                if(btn_D_fileList.positionXY(x, y)) return;
                if(btn_D_cancel.positionXY(x, y)) return;
            break;
        case CLOCK:
                if(_clockSubMenue == 0){
                    if(clk_C_green.positionXY(x, y)) return;
                }
                if(_clockSubMenue == 1){
                    if(btn_C_Mute.positionXY(x, y)) return;
                    if(btn_C_alarm.positionXY(x, y)) return;
                    if(btn_C_radio.positionXY(x, y)) return;
                    if(btn_C_volDown.positionXY(x, y)) return;
                    if(btn_C_volUp.positionXY(x, y)) return;
                }
            break;
        case ALARM:
                if(clk_A_red.positionXY(x, y)) return;
                if(btn_A_left.positionXY(x, y)) return;
                if(btn_A_right.positionXY(x, y)) return;
                if(btn_A_up.positionXY(x, y)) return;
                if(btn_A_down.positionXY(x, y)) return;
                if(btn_A_ready.positionXY(x, y)) return;
            break;

        case SLEEP:
            if((y > _winButton.y) && (y < _winButton.y + _winButton.h)) {
                yPos = SLEEP_1;
                btnNr = x / _winButton.w;
            }
            break;
        case BRIGHTNESS:
            if((y > _winButton.y) && (y < _winButton.y + _winButton.h)) {
                yPos = BRIGHTNESS_1;
                btnNr = x / _winButton.w;
            }
            break;

        case STATIONSLIST:
            if(y -_winHeader.h >= 0 && y -_winHeader.h <= _winWoHF.h){
                btnNr = (y -_winHeader.h)  / (_winWoHF.h / 10);
                yPos = STATIONSLIST_1;
            }
            else if(y > _winFooter.y){
                if(x > _winRSSID.x && x < (_winRSSID.x + _winRSSID.w)){
                    yPos = STATIONSLIST_1;
                    btnNr = 100;
                }
            }
            break;
        case AUDIOFILESLIST:
            if(y -_winHeader.h >= 0 && y -_winHeader.h <= _winWoHF.h){
                btnNr = (y -_winHeader.h)  / (_winWoHF.h / 10);
                yPos = AUDIOFILESLIST_1;
            }
            else if(y > _winFooter.y){
                if(x > _winRSSID.x && x < (_winRSSID.x + _winRSSID.w)){
                    yPos = AUDIOFILESLIST_1;
                    btnNr = 100;
                }
            }
            break;
        case DLNAITEMSLIST:
            if(y -_winHeader.h >= 0 && y -_winHeader.h <= _winWoHF.h){
                btnNr = (y -_winHeader.h)  / (_winWoHF.h / 10);
                yPos = DLNAITEMSLIST_1;
            }
            else if(y > _winFooter.y){
                if(x > _winRSSID.x && x < (_winRSSID.x + _winRSSID.w)){
                    yPos = DLNAITEMSLIST_1;
                    btnNr = 100;
                }
            }
            break;
        case EQUALIZER:
            if(sdr_E_lowPass.positionXY(x,y)) return;
            if(sdr_E_bandPass.positionXY(x,y)) return;
            if(sdr_E_highPass.positionXY(x,y)) return;
            if(sdr_E_balance.positionXY(x,y)) return;
            if(btn_E_lowPass.positionXY(x, y)) return;
            if(btn_E_bandPass.positionXY(x, y)) return;
            if(btn_E_highPass.positionXY(x, y)) return;
            if(btn_E_balance.positionXY(x, y)) return;
            if(txt_E_lowPass.positionXY(x, y)) return;
            if(txt_E_bandPass.positionXY(x, y)) return;
            if(txt_E_highPass.positionXY(x, y)) return;
            if(txt_E_balance.positionXY(x, y)) return;
            if(btn_E_Radio.positionXY(x, y)) return;
            if(btn_E_Player.positionXY(x,y)) return;
            if(btn_E_Mute.positionXY(x, y)) return;

        default:
            break;
    }
    if(yPos == none) {
        SerialPrintfln(ANSI_ESC_YELLOW "Touchpoint not valid x=%d, y=%d", x, y);
        return;
    }

    switch(yPos){

        case ALARM_2:       if(btnNr == 0){_releaseNr = 30;} // left
                            if(btnNr == 1){_releaseNr = 31;} // right
                            if(btnNr == 2){_releaseNr = 32;} // up
                            if(btnNr == 3){_releaseNr = 33;} // down
                            if(btnNr == 4){_releaseNr = 34;} // ready (return to CLOCK)
                            changeBtn_pressed(btnNr); break;
        case ALARM_1:       if(btnNr == 0){_releaseNr = 60;} // mon
                            if(btnNr == 1){_releaseNr = 61;} // tue
                            if(btnNr == 2){_releaseNr = 62;} // wed
                            if(btnNr == 3){_releaseNr = 63;} // thu
                            if(btnNr == 4){_releaseNr = 64;} // fri
                            if(btnNr == 5){_releaseNr = 65;} // sat
                            if(btnNr == 6){_releaseNr = 66;} // sun
                            break;
        case SLEEP_1:       if(btnNr == 0){_releaseNr = 70;} // sleeptime up
                            if(btnNr == 1){_releaseNr = 71;} // sleeptime down
                            if(btnNr == 2){_releaseNr = 72;} // display_sleeptime(0, true);} // ready, return to RADIO
                            if(btnNr == 3){_releaseNr = 73;} // unused
                            if(btnNr == 4){_releaseNr = 74;} // return to RADIO without saving sleeptime
                            changeBtn_pressed(btnNr); break;
        case BRIGHTNESS_1:  if(btnNr == 0){_releaseNr = 80;} // darker
                            if(btnNr == 1){_releaseNr = 81;} // brighter
                            if(btnNr == 2){_releaseNr = 82;} // okay
                            changeBtn_pressed(btnNr); break;
        case STATIONSLIST_1:if(btnNr == none) break;
                            _releaseNr = 100;
                            _staListPos = btnNr;

                            break;
        case AUDIOFILESLIST_1: if(btnNr == none) break;
                            _releaseNr = 110;
                            if(btnNr >= 0 && btnNr < 100) _fileListPos = btnNr;
                            else if (btnNr == 100){_timeCounter.timer = 1;} // leave the list faster
                            vTaskDelay(100);
                            break;
        case DLNAITEMSLIST_1: if(btnNr == none) break;
                            _releaseNr = 120;
                            if(btnNr >= 0 && btnNr < 100) _fileListPos = btnNr;
                            else if (btnNr == 100){_timeCounter.timer = 1;} // leave the list faster
                            _itemListPos = btnNr;
                            vTaskDelay(100);
                            break;
        default:            break;
    }
}
void tp_long_pressed(uint16_t x, uint16_t y){
    // log_w("long pressed %i  %i", x, y);
    if((_releaseNr == 0 || _releaseNr == 22 || _releaseNr == 50 || _releaseNr == 90) && _f_mute) {
        fall_asleep();
    }
    if(_releaseNr == 110){
        uint8_t btnNr = (y -_winHeader.h)  / (_winWoHF.h / 10);
        log_i("longPressed X %i, Y %i, btnNr %i", x, y, btnNr);
    }
}
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void tp_released(uint16_t x, uint16_t y){
    switch(_state){
        case RADIO:
            if(_radioSubmenue == 1){
                btn_R_Mute.released();        btn_R_volDown.released();     btn_R_volUp.released();       btn_R_prevSta.released();     btn_R_nextSta.released();     btn_R_staList.released();
            }
            if(_radioSubmenue == 2){
                btn_R_player.released();      btn_R_dlna.released();        btn_R_clock.released();       btn_R_sleep.released();       btn_R_bright.released();      btn_R_equal.released();
            }
            break;
        case PLAYER:
            if(_playerSubmenue == 0){
                btn_P_prevFile.released();    btn_P_nextFile.released();    btn_P_ready.released();       btn_P_playAll.released();     btn_P_shuffle.released();     btn_P_fileList.released();
                btn_P_radio.released();
            }
            if(_playerSubmenue == 1){
                btn_P_Mute.released();        btn_P_volDown.released();     btn_P_volUp.released();       btn_P_pause.released();       btn_P_cancel.released();
            }
            break;
        case DLNA:
                btn_D_Mute.released();        btn_D_pause.released();       btn_D_volDown.released();     btn_D_volUp.released();       btn_D_radio.released();       btn_D_fileList.released();
                btn_D_cancel.released();
            break;
        case CLOCK:
                btn_C_Mute.released();        btn_C_alarm.released();       btn_C_radio.released();       btn_C_volDown.released();     btn_C_volUp.released();       clk_C_green.released();
            break;
        case ALARM:
                clk_A_red.released();         btn_A_left.released();        btn_A_right.released();       btn_A_up.released();          btn_A_down.released();        btn_A_ready.released();
            break;
        case EQUALIZER:
                sdr_E_lowPass.released();     sdr_E_bandPass.released();    sdr_E_highPass.released();    sdr_E_balance.released();     btn_E_lowPass.released();     btn_E_bandPass.released();
                btn_E_highPass.released();    btn_E_balance.released();     txt_E_lowPass.released();     txt_E_bandPass.released();    txt_E_highPass.released();    txt_E_balance.released();
                btn_E_Radio.released();       btn_E_Player.released();      btn_E_Mute.released();
            break;
    }
    // SerialPrintfln("tp_released, state is: %i", _state);
    if(_f_sleeping){ wake_up(); return;}   // if sleeping

    switch(_releaseNr){

        /* SLEEP ******************************************/
        case 70:    display_sleeptime(1);  changeBtn_released(0); break;
        case 71:    display_sleeptime(-1); changeBtn_released(1); break;
        case 72:    updateSleepTime(true);
                    changeBtn_released(2);
                    changeState(RADIO); break;
        case 73:    changeBtn_released(3); break; // unused
        case 74:    _sleeptime = 0;
                    changeBtn_released(4);
                    changeState(RADIO); break;

        /* BRIGHTNESS ************************************/
        case 80:    downBrightness(); changeBtn_released(0); break;
        case 81:    upBrightness();   changeBtn_released(1); break;
        case 82:    changeState(RADIO); break;

        /* STATIONSLIST *********************************/
        case 100:   if(y -_winHeader.h >= 0 && y -_winHeader.h <= _winWoHF.h){
                        uint8_t staListPos = (y -_winHeader.h)  / (_winWoHF.h / 10);
                        if(_staListPos + 2 < staListPos){               // wipe down
                            if(_staListNr == 0) break;
                            if(_staListNr >  9) _staListNr -= 9;
                            else _staListNr = 0;
                            showStationsList(_staListNr);
                        }
                        else if(staListPos + 2 < _staListPos){          // wipe up
                            if(_staListNr + 9 >= _sum_stations) break;
                            _staListNr += 9;
                            showStationsList(_staListNr);
                        }
                        else if(staListPos == _staListPos){
                            uint16_t staNr = _staListNr + staListPos + 1;
                            if(staNr > _sum_stations){
                                SerialPrintfln(ANSI_ESC_YELLOW "Touchpoint not valid x=%d, y=%d", x, y);
                                break;
                            }
                            if(y -_winHeader.h >= 0 && y -_winHeader.h <= _winWoHF.h){
                                tft.setFont(_fonts[0]);
                                uint8_t staListPos = (y -_winHeader.h)  / (_winWoHF.h / 10);
                                uint16_t staNr = _staListNr + staListPos + 1;
                                if(staNr > _sum_stations){
                                    SerialPrintfln(ANSI_ESC_YELLOW "Touchpoint not valid x=%d, y=%d", x, y);
                                    break;
                                }
                                uint8_t lineHight = _winWoHF.h / 10;
                                sprintf(_chbuf, "station_%03d", staNr);
                                String content = stations.getString(_chbuf, " #not_found");
                                int32_t idx = content.indexOf("#");
                                sprintf(_chbuf, ANSI_ESC_YELLOW"%03d " ANSI_ESC_CYAN "%s\n",staNr, content.substring(0, idx).c_str());
                                tft.writeText(_chbuf, 10, _winHeader.h + (staListPos * lineHight), _dispWidth - 10, lineHight, TFT_ALIGN_LEFT, true, true);
                                vTaskDelay(500);
                            }
                            _timeCounter.timer = 0;
                            showFooterRSSI(true);
                            setStation(staNr);
                            changeState(RADIO);
                        }
                        else log_i("unknown gesture");
                    } break;
        /* AUDIOFILESLIST *******************************/
        case 110:   if(y -_winHeader.h >= 0 && y -_winHeader.h <= _winWoHF.h){
                        uint8_t fileListPos = (y -_winHeader.h)  / (_winWoHF.h / 10);
                        if(_fileListPos + 2 < fileListPos){               // wipe down
                            if(_fileListNr == 0) break;
                            if(_fileListNr >  9) _fileListNr -= 9;
                            else _fileListNr = 0;
                            showAudioFilesList(_fileListNr);
                        }
                        else if(fileListPos + 2 < _fileListPos){          // wipe up
                            if(_fileListNr + 9 >= _SD_content.size()) break;
                            _fileListNr += 9;
                            showAudioFilesList(_fileListNr);
                        }
                        else if(fileListPos == _fileListPos){
                            uint16_t fileNr = _fileListNr + fileListPos;
                            if(fileNr > _SD_content.size()){
                                SerialPrintfln(ANSI_ESC_YELLOW "Touchpoint not valid x=%d, y=%d", x, y);
                                break;
                            }
                            uint8_t lineHight = _winWoHF.h / 10;
                            if(fileListPos == 0) {
                                int32_t idx = _curAudioFolder.lastIndexOf("/");
                                if(idx > 1){ // not the first '/'
                                    _curAudioFolder = _curAudioFolder.substring(0, idx);
                                    _fileListNr = 0;
                                    SD_listDir(_curAudioFolder.c_str(), true, false);
                                    showAudioFilesList(_fileListNr);
                                    break;
                                }
                            }
                            else{
                                if(fileNr > _SD_content.size()) break;
                                tft.setTextColor(TFT_CYAN);
                                _cur_AudioFileNr = fileNr - 1;
                                tft.setFont(_fonts[0]);
                                tft.writeText(_SD_content[_cur_AudioFileNr], 20, _winFooter.h + (fileListPos) * lineHight, _dispWidth - 20, lineHight, TFT_ALIGN_LEFT, true, true);
                                vTaskDelay(500);
                                sprintf(_chbuf, "%s/%s", _curAudioFolder.c_str() ,_SD_content[_cur_AudioFileNr]);
                                int32_t idx = indexOf(_chbuf, "\033[", 1);
                                if(idx == -1){ // is folder
                                    _curAudioFolder += "/" + (String)_SD_content[_cur_AudioFileNr];
                                    _fileListNr = 0;
                                    SD_listDir(_curAudioFolder.c_str(), true, false);
                                    showAudioFilesList(_fileListNr);
                                    break;
                                }
                                else {
                                    _chbuf[idx] = '\0';  // remove color and filesize
                                    clearStreamTitle();
                                    changeState(PLAYER);
                                    log_i("fn %s", _chbuf);
                                    SD_playFile(_chbuf, 0, true);
                                    showAudioFileNumber();
                                }
                            }
                            _timeCounter.timer = 0;
                            showFooterRSSI(true);
                        }
                        else log_i("unknown gesture");
                    } break;
        /* DLNAITEMSLIST *********************************/
        case 120:   if(y -_winHeader.h >= 0 && y -_winHeader.h <= _winWoHF.h){
                        DLNA_Client::dlnaServer_t dlnaServer = dlna.getServer();
                        DLNA_Client::srvContent_t srvContent = dlna.getBrowseResult();
                        uint16_t itemSize = 0;
                        if(_dlnaLevel == 0) itemSize = dlnaServer.size;
                        else                itemSize = srvContent.size;
                        uint8_t itemListPos = (y -_winHeader.h)  / (_winWoHF.h / 10);
                        if(_itemListPos + 2 < itemListPos){               // wipe down
                            if(_dlnaItemNr == 0) break;
                            if(_dlnaItemNr >  9) _dlnaItemNr -= 9;
                            else _dlnaItemNr = 0;
                            dlna.browseServer(_currDLNAsrvNr, _dlnaHistory[_dlnaLevel].objId, _dlnaItemNr , 9);
                            _f_dlnaWaitForResponse = true;
                            break;
                        }
                        else if(itemListPos + 2 < _itemListPos){          // wipe up
                            if(_dlnaItemNr + 9 >= _dlnaMaxItems) break;
                            _dlnaItemNr += 9;
                            dlna.browseServer(_currDLNAsrvNr, _dlnaHistory[_dlnaLevel].objId, _dlnaItemNr , 9);
                            _f_dlnaWaitForResponse = true;
                            break;
                        }
                        else if(itemListPos == _itemListPos){            // no wipe
                            uint16_t itemNr = _dlnaItemNr + itemListPos;
                            if(itemNr % 9 > itemSize){
                                SerialPrintfln(ANSI_ESC_YELLOW "Touchpoint not valid x=%d, y=%d", x, y);
                                break;
                            }
                            uint8_t lineHight = _winWoHF.h / 10;
                            if(itemListPos == 0) {
                                if(_dlnaLevel == 0) break;
                                tft.setFont(_fonts[0]);
                                tft.setTextColor(TFT_CYAN);
                                tft.writeText(_dlnaHistory[_dlnaLevel].name, 10, _winFooter.h, _dispWidth - 20, lineHight, TFT_ALIGN_LEFT, true, true);
                                _dlnaLevel--;
                                dlna.browseServer(_currDLNAsrvNr, _dlnaHistory[_dlnaLevel].objId, 0 , 9);
                                _f_dlnaWaitForResponse = true;
                                break;
                            }
                            else{
                                if(itemListPos > itemSize) break;
                                tft.setTextColor(TFT_CYAN);
                                uint8_t pos = itemListPos;
                                tft.setFont(_fonts[0]);
                                if(_dlnaLevel == 0){  // server list
                                    tft.writeText(dlnaServer.friendlyName[pos - 1], 20, _winFooter.h + (pos) * lineHight, _dispWidth - 20, lineHight, TFT_ALIGN_LEFT, true, true);
                                    _currDLNAsrvNr = pos - 1;
                                    _dlnaLevel++;
                                    if(_dlnaHistory[_dlnaLevel].name){free(_dlnaHistory[_dlnaLevel].name); _dlnaHistory[_dlnaLevel].name = NULL;}
                                    _dlnaHistory[_dlnaLevel].name = strdup(dlnaServer.friendlyName[pos - 1]);
                                    dlna.browseServer(_currDLNAsrvNr, "0", 0 , 9);
                                    _f_dlnaWaitForResponse = true;
                                }
                                else {  // content list
                                    if(startsWith(srvContent.itemURL[pos - 1], "http")){ // is file
                                        if(srvContent.isAudio[pos - 1]){
                                            sprintf(_chbuf, "%s",srvContent.title[pos - 1]);
                                            tft.writeText(_chbuf, 20, _winFooter.h + (pos) * lineHight, _dispWidth - 20, lineHight, TFT_ALIGN_LEFT, true, true);
                                            connecttohost(srvContent.itemURL[pos - 1]);
                                            changeState(DLNA);
                                            showFileName(srvContent.title[pos - 1]);
                                        }
                                    }
                                    else{ // is folder
                                        if(srvContent.childCount[pos - 1] == 0){
                                            sprintf(_chbuf, "%s",srvContent.title[pos - 1]);
                                        }
                                        else{
                                            sprintf(_chbuf, "%s (%d)",srvContent.title[pos - 1], srvContent.childCount[pos - 1]);
                                        }
                                        tft.writeText(_chbuf, 20, _winFooter.h + (pos) * lineHight, _dispWidth - 20, lineHight, TFT_ALIGN_LEFT, true, true);
                                        _dlnaLevel++;
                                        if(_dlnaHistory[_dlnaLevel].objId){free(_dlnaHistory[_dlnaLevel].objId); _dlnaHistory[_dlnaLevel].objId = NULL;}
                                        _dlnaHistory[_dlnaLevel].objId = strdup(srvContent.objectId[pos -1]);
                                        if(_dlnaHistory[_dlnaLevel].name){free(_dlnaHistory[_dlnaLevel].name); _dlnaHistory[_dlnaLevel].name = NULL;}
                                        _dlnaHistory[_dlnaLevel].name = strdup(srvContent.title[pos - 1]);
                                        _dlnaItemNr = 0; // new folder? reset dlnaItemNr
                                        dlna.browseServer(_currDLNAsrvNr, _dlnaHistory[_dlnaLevel].objId, 0 , 9);
                                        _f_dlnaWaitForResponse = true;
                                    }
                                }
                            }
                            _timeCounter.timer = 0;
                            showFooterRSSI(true);
                        }
                    } break;
    }
    _releaseNr = -1;
}

void tp_long_released(){
    // log_w("long released)");
    if(_releaseNr == 0 || _releaseNr == 22 || _releaseNr == 50) {return;}
    tp_released(0, 0);
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void tp_positionXY(uint16_t x, uint16_t y){
    if(_state == EQUALIZER){
        if(sdr_E_lowPass.positionXY(x, y)) return;
        if(sdr_E_bandPass.positionXY(x, y)) return;
        if(sdr_E_highPass.positionXY(x, y)) return;
        if(sdr_E_balance.positionXY(x, y)) return;
    }
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//Events from websrv
void WEBSRV_onCommand(const String cmd, const String param, const String arg){  // called from html

    if(CORE_DEBUG_LEVEL == ARDUHAL_LOG_LEVEL_WARN){
        SerialPrintfln("WS_onCmd:    " ANSI_ESC_YELLOW "cmd=\"%s\", params=\"%s\", arg=\"%s\"",
                                                        cmd.c_str(),param.c_str(), arg.c_str());
    }

    String  str;

    if(cmd == "ping"){              webSrv.send("pong"); return;}                                                                                     // via websocket

    if(cmd == "index.html"){        if(_f_accessPoint) {SerialPrintfln("Webpage:     " ANSI_ESC_ORANGE "accesspoint.html");                           // via XMLHttpRequest
                                                        webSrv.show(accesspoint_html, webSrv.TEXT);}
                                    else               {SerialPrintfln("Webpage:     " ANSI_ESC_ORANGE "index.html");
                                                        webSrv.show(index_html, webSrv.TEXT);      }
                                    return;}

    if(cmd == "index.js"){          SerialPrintfln("Script:      " ANSI_ESC_ORANGE "index.js");                                                       // via XMLHttpRequest
                                    webSrv.show(index_js, webSrv.JS); return;}

    if(cmd == "favicon.ico"){       webSrv.streamfile(SD_MMC, "/favicon.ico"); return;}                                                               // via XMLHttpRequest

    if(cmd == "test"){              sprintf(_chbuf, "free heap: %lu, Inbuff filled: %lu, Inbuff free: %lu, PSRAM filled %lu, PSRAM free %lu",
                                        (long unsigned)ESP.getFreeHeap(), (long unsigned)audioInbuffFilled(), (long unsigned)audioInbuffFree(),
                                        (long unsigned) (ESP.getPsramSize() - ESP.getFreePsram()), (long unsigned)ESP.getFreePsram());
                                    webSrv.send("test=", _chbuf);
                                    SerialPrintfln("audiotask .. stackHighWaterMark: %lu bytes", (long unsigned)audioGetStackHighWatermark() * 4);
                                    SerialPrintfln("looptask ... stackHighWaterMark: %lu bytes", (long unsigned)uxTaskGetStackHighWaterMark(NULL) * 4);
                                    return;}

    if(cmd == "getmute"){           if(_f_mute) webSrv.send("mute=", "1");
                                    else        webSrv.send("mute=", "0");
                                    return;}

    if(cmd == "setmute"){           _f_mute = !_f_mute; muteChanged(_f_mute); return;}

    if(cmd == "upvolume"){          webSrv.send("volume=", (String)upvolume());  return;}                                                            // via websocket

    if(cmd == "downvolume"){        webSrv.send("volume=", (String)downvolume()); return;}                                                           // via websocket

    if(cmd == "homepage"){          webSrv.send("homepage=", _homepage);
                                    return;}

    if(cmd == "to_listen"){         StationsItems(); // via websocket, return the name and number of the current station
                                    return;}

    if(cmd == "gettone"){           webSrv.send("settone=", setI2STone());
                                    return;}

    if(cmd == "getstreamtitle"){    webSrv.reply(_streamTitle, webSrv.TEXT);
                                    return;}

    if(cmd == "LowPass"){           _toneLP = param.toInt();                           // audioI2S tone
                                    char lp[30] = "Lowpass set to "; strcat(lp, param.c_str()); strcat(lp, "dB");
                                    webSrv.send("tone=", lp); setI2STone(); return;}

    if(cmd == "BandPass"){          _toneBP = param.toInt();                           // audioI2S tone
                                    char bp[30] = "Bandpass set to "; strcat(bp, param.c_str()); strcat(bp, "dB");
                                    webSrv.send("tone=", bp); setI2STone(); return;}

    if(cmd == "HighPass"){          _toneHP = param.toInt();                           // audioI2S tone
                                    char hp[30] = "Highpass set to "; strcat(hp, param.c_str()); strcat(hp, "dB");
                                    webSrv.send("tone=", hp); setI2STone(); return;}

    if(cmd == "Balance"){           _toneBAL = param.toInt();
                                    char bal[30] = "Balance set to "; strcat(bal, param.c_str());
                                    webSrv.send("tone=", bal); setI2STone(); return;}

    if(cmd == "uploadfile"){        _filename = param;  return;}

    if(cmd == "prev_station"){      prevStation(); return;}                                                                                           // via websocket

    if(cmd == "next_station"){      nextStation(); return;}                                                                                           // via websocket

    if(cmd == "set_station"){       setStation(param.toInt()); return;}                                                                               // via websocket

    if(cmd == "stationURL"){        setStationViaURL(param.c_str()); return;}                                                                         // via websocket

    if(cmd == "getnetworks"){       webSrv.send("networks=", WiFi.SSID()); return;}                                                  // via websocket

    if(cmd == "get_tftSize"){       if(_tftSize){webSrv.send("tftSize=", "m");} else{webSrv.send("tftSize=", "s");} return;};

    if(cmd == "getTimeZones"){      webSrv.streamfile(SD_MMC, "/timezones.csv"); return;}

    if(cmd == "setTimeZone"){       _TZName = param;  _TZString = arg;
                                    SerialPrintfln("Timezone: .. " ANSI_ESC_BLUE "%s, %s", param.c_str(), arg.c_str());
                                    setRTC(_TZString.c_str());
                                    updateSettings(); // write new TZ items to settings.json
                                    return;}

    if(cmd == "getTimeZoneName"){   webSrv.reply(_TZName, webSrv.TEXT); return;}

    if(cmd == "change_state"){      if(_state != CLOCK){
                                        if     (!strcmp(param.c_str(), "RADIO") && _state != RADIO) {setStation(_cur_station); changeState(RADIO);  return;}
                                        else if(!strcmp(param.c_str(), "PLAYER")&& _state != PLAYER){stopSong(); changeState(PLAYER); return;}
                                        else if(!strcmp(param.c_str(), "DLNA")  && _state != DLNA)  {stopSong(); changeState(DLNA);   return;}
                                        else return;
                                    }}
    if(cmd == "stopfile"){          _resumeFilePos = audioStopSong(); webSrv.send("stopfile=", "audiofile stopped");
                                    return;}

    if(cmd == "resumefile"){        if(!_lastconnectedfile) webSrv.send("resumefile=", "nothing to resume");
                                    else {
                                        SD_playFile(_lastconnectedfile, _resumeFilePos);
                                        webSrv.send("resumefile=", "audiofile resumed");
                                    }
                                    return;}

    if(cmd == "get_alarmdays"){     webSrv.send("alarmdays=", String(_alarmdays, 10)); return;}

    if(cmd == "set_alarmdays"){     _alarmdays = param.toInt(); updateSettings(); return;}

    if(cmd == "get_alarmtime"){     return;} // not used yet

    if(cmd == "set_alarmtime"){     return;}

    if(cmd == "get_timeAnnouncement"){ if(_f_timeAnnouncement) webSrv.send("timeAnnouncement=", "1");
                                    if(  !_f_timeAnnouncement) webSrv.send("timeAnnouncement=", "0");
                                    return;}

    if(cmd == "set_timeAnnouncement"){ if(param == "true" ) _f_timeAnnouncement = true;
                                    if(   param == "false") _f_timeAnnouncement = false;
                                    return;}

    if(cmd == "DLNA_getServer")  {  webSrv.send("DLNA_Names=", dlna.stringifyServer()); _currDLNAsrvNr = -1; return;}

    if(cmd == "DLNA_getRoot")    {  _currDLNAsrvNr = param.toInt(); dlna.browseServer(_currDLNAsrvNr, "0"); return;}

    if(cmd == "DLNA_getContent") {  if(param.startsWith("http")) {connecttohost(param.c_str()); showFileName(arg.c_str()); return;}
                                    if(_dlnaHistory[_dlnaLevel].objId){free(_dlnaHistory[_dlnaLevel].objId); _dlnaHistory[_dlnaLevel].objId = NULL;} _dlnaHistory[_dlnaLevel].objId = strdup(param.c_str());
                                    _totalNumberReturned = 0;
                                    dlna.browseServer(_currDLNAsrvNr, _dlnaHistory[_dlnaLevel].objId);
                                    return;}

    if(cmd == "AP_ready"){          webSrv.send("networks=", _scannedNetworks); return;}                                                              // via websocket

    if(cmd == "credentials"){       String AP_SSID = param.substring(0, param.indexOf("\n"));                                                         // via websocket
                                    String AP_PW =   param.substring(param.indexOf("\n") + 1);
                                    SerialPrintfln("credentials: SSID " ANSI_ESC_BLUE "%s" ANSI_ESC_WHITE ", PW " ANSI_ESC_BLUE "%s",
                                    AP_SSID.c_str(), AP_PW.c_str());
                                    pref.putString("ap_ssid", AP_SSID);
                                    pref.putString("ap_pw", AP_PW);
                                    ESP.restart();}

    if(cmd.startsWith("SD/")){      String str = cmd.substring(2);                                                                                    // via XMLHttpRequest
                                    if(!webSrv.streamfile(SD_MMC, scaleImage(str.c_str()))){
                                        SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "File not found " ANSI_ESC_RED "\"%s\"", str.c_str());
                                        webSrv.streamfile(SD_MMC, scaleImage("/common/unknown.jpg"));}
                                    return;}

    if(cmd == "SD_Download"){       webSrv.streamfile(SD_MMC, param.c_str());
                                    SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "Download  " ANSI_ESC_ORANGE "\"%s\"", param.c_str());
                                    return;}

    if(cmd == "SD_GetFolder"){      webSrv.reply(SD_stringifyDirContent(param), webSrv.JS);
                                    SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "GetFolder " ANSI_ESC_ORANGE "\"%s\"", param.c_str());             // via XMLHttpRequest
                                    return;}

    if(cmd == "SD_newFolder"){      bool res = SD_newFolder(param.c_str());
                                    if(res) webSrv.sendStatus(200); else webSrv.sendStatus(400);
                                    SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "NewFolder " ANSI_ESC_ORANGE "\"%s\"", param.c_str());             // via XMLHttpRequest
                                    return;}

    if(cmd == "SD_playFile"){       webSrv.reply("SD_playFile=" + param, webSrv.TEXT);
                                    SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "Play " ANSI_ESC_ORANGE "\"%s\"", param.c_str());                  // via XMLHttpRequest
                                    SD_playFile(param.c_str());
                                    return;}

    if(cmd == "SD_playAllFiles"){   webSrv.send("SD_playFolder=", "" + param);
                                    SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "Play Folder" ANSI_ESC_ORANGE "\"%s\"", param.c_str());
                                    preparePlaylistFromFolder(param.c_str());
                                    processPlaylist(true);
                                    return;}

    if(cmd == "SD_rename"){         SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "Rename " ANSI_ESC_ORANGE "old \"%s\" new \"%s\"",                 // via XMLHttpRequest
                                    param.c_str(), arg.c_str());
                                    bool res = SD_rename(param.c_str(), arg.c_str());
                                    if(res) webSrv.reply("refresh", webSrv.TEXT);
                                    else webSrv.sendStatus(400);
                                    return;}

    if(cmd == "SD_delete"){         bool res = SD_delete(param.c_str());
                                    if(res) webSrv.sendStatus(200); else webSrv.sendStatus(400);
                                    SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "Delete " ANSI_ESC_ORANGE "\"%s\"", param.c_str());                // via XMLHttpRequest
                                    return;}

    if(cmd == "SD_Upload"){        _filename = param;
                                   _f_SD_Upload = true;
                                   SerialPrintfln("webSrv: ...  " ANSI_ESC_YELLOW "Upload  " ANSI_ESC_ORANGE "\"%s\"", param.c_str());
                                   return;}

    if(cmd == "setIRcmd"){         int32_t command = (int32_t)strtol(param.c_str(), NULL, 16);
                                   int32_t btnNr = (int32_t)strtol(arg.c_str(), NULL, 10);
                                   SerialPrintfln("set_IR_cmd:  " ANSI_ESC_YELLOW "IR command " ANSI_ESC_BLUE "0x%02lx, "
                                   ANSI_ESC_YELLOW "IR Button Number " ANSI_ESC_BLUE "0x%02lx", (long signed)command, (long signed)btnNr);
                                   ir.set_irButtons(btnNr,  command);
                                   return;}
    if(cmd == "setIRadr"){         SerialPrintfln("set_IR_adr:  " ANSI_ESC_YELLOW "IR address " ANSI_ESC_BLUE "%s",
                                   param.c_str());
                                   int32_t address = (int32_t)strtol(param.c_str(), NULL, 16);
                                   ir.set_irAddress(address);
                                   return;}

    if(cmd == "saveIRbuttons"){    saveIRbuttonsToNVS(); return;}

    if(cmd == "getTimeFormat"){    webSrv.send("timeFormat=", String(_timeFormat, 10));
                                   return;}

    if(cmd == "setTimeFormat"){    _timeFormat = param.toInt();
                                   clk_C_green.setTimeFormat(_timeFormat);
                                   if(_state == CLOCK){
                                        clearWithOutHeaderFooter();
                                   }
                                   SerialPrintfln("TimeFormat:  " ANSI_ESC_YELLOW "new time format: " ANSI_ESC_BLUE "%sh", param.c_str());
                                   return;}

    if(cmd == "loadIRbuttons"){    loadIRbuttonsFromNVS(); // update IR buttons in ir.cpp
                                   char buf[150];
                                   uint8_t* buttons = ir.get_irButtons();
                                   sprintf(buf,"0x%02x,", ir.get_irAddress());
                                   for(uint8_t i = 0; i< 20; i++){
                                        sprintf(buf + 5 + 5 * i, "0x%02x,", buttons[i]);
                                   }
                                   buf[5 + 5 * 20] = '\0';
                                   webSrv.reply(buf, webSrv.TEXT); return;}

    if(cmd == "DLNA_GetFolder"){   webSrv.sendStatus(306); return;}  // todo
    if(cmd == "KCX_BT_connected"){ if(bt_emitter.isConnected()) webSrv.send("KCX_BT_connected=", "1"); else webSrv.send("KCX_BT_connected=", "0"); return;}
    if(cmd == "KCX_BT_clearItems"){bt_emitter.deleteVMlinks(); return;}
    if(cmd == "KCX_BT_addName"){   bt_emitter.addLinkName(param.c_str()); return;}
    if(cmd == "KCX_BT_addAddr"){   bt_emitter.addLinkAddr(param.c_str()); return;}
    if(cmd == "KCX_BT_mem"){       bt_emitter.getVMlinks(); return;}
    if(cmd == "KCX_BT_scanned"){   webSrv.send("KCX_BT_SCANNED=", bt_emitter.stringifyScannedItems()); return;}
    if(cmd == "KCX_BT_getMode"){   webSrv.send("KCX_BT_MODE=", bt_emitter.getMode()); return;}
    if(cmd == "KCX_BT_changeMode"){bt_emitter.changeMode(); return;}
    if(cmd == "KCX_BT_pause"){     bt_emitter.pauseResume(); return;}
    if(cmd == "KCX_BT_downvolume"){bt_emitter.downvolume(); return;}
    if(cmd == "KCX_BT_upvolume"){  bt_emitter.upvolume(); return;}

    if(cmd == "hardcopy") {SerialPrintfln("Webpage: ... " ANSI_ESC_YELLOW "create a display hardcopy"); hardcopy(); webSrv.send("hardcopy=", "/hardcopy.bmp"); return;}

    SerialPrintfln(ANSI_ESC_RED "unknown HTMLcommand %s, param=%s", cmd.c_str(), param.c_str());
    webSrv.sendStatus(400);
}
// clang-format on
void WEBSRV_onRequest(const String request, uint32_t contentLength) {
    if(CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_INFO) { SerialPrintfln("WS_onReq:    " ANSI_ESC_YELLOW "%s contentLength %lu", request.c_str(), (long unsigned)contentLength); }

    if(request.startsWith("------")) return;     // uninteresting WebKitFormBoundaryString
    if(request.indexOf("form-data") > 0) return; // uninteresting Info
    if(request == "fileUpload") {
        savefile(_filename.c_str(), contentLength);
        return;
    }
    if(request.startsWith("Content")) return; // suppress Content-Disposition and Content-Type

    SerialPrintfln(ANSI_ESC_RED "unknown request: %s", request.c_str());
}
void WEBSRV_onInfo(const char* info) {
    if(startsWith(info, "WebSocket")) return;      // suppress WebSocket client available
    if(!strcmp("ping", info)) return;              // suppress ping
    if(!strcmp("to_listen", info)) return;         // suppress to_isten
    if(startsWith(info, "Command client")) return; // suppress Command client available
    if(startsWith(info, "Content-D")) return;      // Content-Disposition
    if(CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG) {
        SerialPrintfln("HTML_info:   " ANSI_ESC_YELLOW "\"%s\"", info); // infos for debug
    }
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  Events from DLNA
void dlna_info(const char* info) {
    if(endsWith(info, "is not responding after request")) { // timeout
        _f_dlnaBrowseServer = false;
        if(_dlnaLevel > 0) _dlnaLevel--;
        showDlnaItemsList(_dlnaItemNr, _dlnaHistory[_dlnaLevel].name);
    }
    SerialPrintfln("DLNA_info:    %s", info);
}

void dlna_server(uint8_t serverId, const char* IP_addr, uint16_t port, const char* friendlyName, const char* controlURL) {
    SerialPrintfln("DLNA_server: [%d] " ANSI_ESC_CYAN "%s:%d " ANSI_ESC_YELLOW " %s", serverId, IP_addr, port, friendlyName);
}

void dlna_seekReady(uint8_t numberOfServer) { SerialPrintfln("DLNA_server: %i media server found", numberOfServer); }

void dlna_browseResult(const char* objectId, const char* parentId, uint16_t childCount, const char* title, bool isAudio, uint32_t itemSize, const char* duration, const char* itemURL) {
    SerialPrintfln("DLNA_server: " ANSI_ESC_YELLOW "title %s, childCount %d, itemSize %ld, duration %s", title, childCount, (long unsigned int)itemSize, duration);
}

void dlna_browseReady(uint16_t numberReturned, uint16_t totalMatches) {
    SerialPrintfln("DLNA_server: returned %i from %i", numberReturned + _totalNumberReturned, totalMatches);
    _dlnaMaxItems = totalMatches;
    if(numberReturned == 50) { // next round
        _totalNumberReturned += numberReturned;
        if(_totalNumberReturned < totalMatches && _totalNumberReturned < 500) { _f_dlnaBrowseServer = true; }
    }
    if(_f_dlnaWaitForResponse) {
        _f_dlnaWaitForResponse = false;
        showDlnaItemsList(_dlnaItemNr, _dlnaHistory[_dlnaLevel].name);
    }
    else { webSrv.send("dlnaContent=", dlna.stringifyContent()); }
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void kcx_bt_info(const char* info, const char* val) { SerialPrintfln("BT-Emitter:  %s " ANSI_ESC_YELLOW "%s", info, val); }

void kcx_bt_status(bool status) { // is always called when the status changes fron disconnected to connected and vice versa
    if(status) { webSrv.send("KCX_BT_connected=", "1"); }
    else { webSrv.send("KCX_BT_connected=", "0"); }
}

void kcx_bt_memItems(const char* jsonItems) { // Every time an item (name or address) was added, a JSON string is passed here
    // SerialPrintfln("bt_memItems %s", jsonItems);
    webSrv.send("KCX_BT_MEM=", jsonItems);
}

void kcx_bt_scanItems(const char* jsonItems) { // Every time an item (name and address) was scanned, a JSON string is passed here
    // SerialPrintfln("bt_scanItems %s", jsonItems);
    webSrv.send("KCX_BT_SCANNED=", jsonItems);
}

void kcx_bt_modeChanged(const char* m) { // Every time the mode has changed
    if(strcmp("RX", m) == 0) {
        webSrv.send("KCX_BT_MODE=RX");
        //    if(_f_mute == false) mute();
    }
    if(strcmp("TX", m) == 0) {
        webSrv.send("KCX_BT_MODE=TX");
        //   if(_f_mute == true) mute();
    }
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// clang-format off
void graphicObjects_OnChange(const char* name, int32_t arg1) {
    char c[10];
    if(strcmp(name, "sdr_E_LP") == 0)  {itoa(arg1, c, 10); strcat(c, " dB"); txt_E_lowPass.writeText(c);  _toneLP = arg1;  webSrv.send("settone=", setI2STone()); return;}
    if(strcmp(name, "sdr_E_BP") == 0)  {itoa(arg1, c, 10); strcat(c, " dB"); txt_E_bandPass.writeText(c); _toneBP = arg1;  webSrv.send("settone=", setI2STone()); return;}
    if(strcmp(name, "sdr_E_HP") == 0)  {itoa(arg1, c, 10); strcat(c, " dB"); txt_E_highPass.writeText(c); _toneHP = arg1;  webSrv.send("settone=", setI2STone()); return;}
    if(strcmp(name, "sdr_E_BAL") == 0) {itoa(arg1, c, 10); strcat(c, " ");   txt_E_balance.writeText(c);  _toneBAL = arg1; webSrv.send("settone=", setI2STone()); return;}

    log_d("unused event: graphicObject %s was changed, val %li", name, arg1);
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void graphicObjects_OnClick(const char* name) {
    if(_state == RADIO) {
        if(strcmp(name, "btn_R_Mute") == 0)    {_timeCounter.timer = 5; return;}
        if(strcmp(name, "btn_R_volDown") == 0) {_timeCounter.timer = 5; return;}
        if(strcmp(name, "btn_R_volUp") == 0)   {_timeCounter.timer = 5; return;}
        if(strcmp(name, "btn_R_prevSta") == 0) {clearVolBar();          return;}
        if(strcmp(name, "btn_R_nextSta") == 0) {clearVolBar();          return;}
        if(strcmp(name, "btn_R_staList") == 0) {return;}
        if(strcmp(name, "btn_R_player") == 0)  {return;}
        if(strcmp(name, "btn_R_dlna") == 0)    {return;}
        if(strcmp(name, "btn_R_clock") == 0)   {return;}
        if(strcmp(name, "btn_R_sleep") == 0)   {return;}
        if(strcmp(name, "btn_R_bright") == 0)  {return;}
        if(strcmp(name, "btn_R_equal") == 0)   {return;}
    }
    if(_state == PLAYER) {
        if(strcmp(name, "btn_P_Mute") == 0)    {return;}
        if(strcmp(name, "btn_P_volDown") == 0) {return;}
        if(strcmp(name, "btn_P_volUp") == 0)   {return;}
        if(strcmp(name, "btn_P_pause") == 0)   {return;}
        if(strcmp(name, "btn_P_cancel") == 0)  {return;}
        if(strcmp(name, "btn_P_prevFile") == 0){if(_cur_AudioFileNr > 0) {_cur_AudioFileNr--; showFileName(_SD_content[_cur_AudioFileNr]); showAudioFileNumber();} return;}
        if(strcmp(name, "btn_P_nextFile") == 0){if(_cur_AudioFileNr + 1 < _SD_content.size()) {_cur_AudioFileNr++; showFileName(_SD_content[_cur_AudioFileNr]); showAudioFileNumber();} return;}
        if(strcmp(name, "btn_P_ready") == 0)   {return;}
        if(strcmp(name, "btn_P_playAll") == 0) {return;}
        if(strcmp(name, "btn_P_shuffle") == 0) {return;}
        if(strcmp(name, "btn_P_fileList") == 0){return;}
        if(strcmp(name, "btn_P_radio") == 0)   {return;}
    }
    if(_state == DLNA) {
        if(strcmp(name, "btn_D_Mute") == 0)    {return;}
        if(strcmp(name, "btn_D_pause") == 0)   {return;}
        if(strcmp(name, "btn_D_volDown") == 0) {return;}
        if(strcmp(name, "btn_D_volUp") == 0)   {return;}
        if(strcmp(name, "btn_D_radio") == 0)   {return;}
        if(strcmp(name, "btn_D_fileList") == 0){return;}
        if(strcmp(name, "btn_D_cancel") == 0)  {return;}
    }
    if(_state == CLOCK) {
        if(strcmp(name, "btn_C_Mute") == 0)    {return;}
        if(strcmp(name, "btn_C_alarm") == 0)   {return;}
        if(strcmp(name, "btn_C_radio") == 0)   {return;}
        if(strcmp(name, "btn_C_volDown") == 0) {return;}
        if(strcmp(name, "btn_C_volUp") == 0)   {return;}
        if(strcmp(name, "clk_C_green") == 0)   {return;}
    }
    if(_state == ALARM) {
        if(strcmp(name, "clk_A_red") == 0)     {return;}
        if(strcmp(name, "btn_A_left") == 0)    {return;}
        if(strcmp(name, "btn_A_right") == 0)   {return;}
        if(strcmp(name, "btn_A_up") == 0)      {return;}
        if(strcmp(name, "btn_A_down") == 0)    {return;}
        if(strcmp(name, "btn_A_ready") == 0)   {return;}
    }
    if(_state == EQUALIZER) {
        if(strcmp(name, "btn_E_LP") == 0)      {sdr_E_lowPass.setValue(0);  return;}
        if(strcmp(name, "btn_E_BP") == 0)      {sdr_E_bandPass.setValue(0); return;}
        if(strcmp(name, "btn_E_HP") == 0)      {sdr_E_highPass.setValue(0); return;}
        if(strcmp(name, "btn_E_BAL") == 0)     {sdr_E_balance.setValue(0);  return;}
        if(strcmp(name, "btn_E_Radio") == 0)   {return;}
        if(strcmp(name, "btn_E_Player") == 0)  {return;}
        if(strcmp(name, "btn_E_Mute") == 0)    {return;}
    }
    log_d("unused event: graphicObject %s was clicked", name);
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void graphicObjects_OnRelease(const char* name) {

    if(_state == RADIO) {
        if(strcmp(name, "btn_R_Mute") == 0)     {_f_mute = btn_R_Mute.getValue(); muteChanged(_f_mute); return;}
        if(strcmp(name, "btn_R_volDown") == 0)  {downvolume(); showVolumeBar(); return;}
        if(strcmp(name, "btn_R_volUp") == 0)    {upvolume(); showVolumeBar(); return;}
        if(strcmp(name, "btn_R_prevSta") == 0)  {_radioSubmenue = 0; prevStation(); showFooterStaNr(); return;}
        if(strcmp(name, "btn_R_nextSta") == 0)  {_radioSubmenue = 0; nextStation(); showFooterStaNr(); return;}
        if(strcmp(name, "btn_R_staList") == 0)  {_radioSubmenue = 0; changeState(STATIONSLIST); return;}
        if(strcmp(name, "btn_R_player") == 0)   {_radioSubmenue = 0; stopSong(); changeState(PLAYER); return;}
        if(strcmp(name, "btn_R_dlna") == 0)     {_radioSubmenue = 0; stopSong(); changeState(DLNA); return;}
        if(strcmp(name, "btn_R_clock") == 0)    {_radioSubmenue = 0; changeState(CLOCK); return;}
        if(strcmp(name, "btn_R_sleep") == 0)    {_radioSubmenue = 0; changeState(SLEEP); return;}
        if(strcmp(name, "btn_R_bright") == 0)   {_radioSubmenue = 0; changeState(BRIGHTNESS); return;}
        if(strcmp(name, "btn_R_equal") == 0)    {_radioSubmenue = 0; changeState(EQUALIZER); return;}
    }
    if(_state == PLAYER) {
        if(strcmp(name, "btn_P_Mute") == 0)     {_f_mute = btn_P_Mute.getValue(); muteChanged(_f_mute); return;}
        if(strcmp(name, "btn_P_volDown") == 0)  {downvolume(); showVolumeBar(); return;}
        if(strcmp(name, "btn_P_volUp") == 0)    {upvolume(); showVolumeBar(); return;}
        if(strcmp(name, "btn_P_pause") == 0)    {if(_f_isFSConnected) {audioPauseResume();} return;}
        if(strcmp(name, "btn_P_cancel") == 0)   {_playerSubmenue = 0; stopSong(); changeState(PLAYER); return;}
        if(strcmp(name, "btn_P_prevFile") == 0) {return;}
        if(strcmp(name, "btn_P_nextFile") == 0) {return;}
        if(strcmp(name, "btn_P_ready") == 0)    {_playerSubmenue = 1; SD_playFile(_curAudioFolder.c_str(), _SD_content[_cur_AudioFileNr]); changeState(PLAYER); showAudioFileNumber(); return;}
        if(strcmp(name, "btn_P_playAll") == 0)  {_playerSubmenue = 1; _f_shuffle = false; preparePlaylistFromFolder(_curAudioFolder.c_str()); processPlaylist(true); changeState(PLAYER); return;}
        if(strcmp(name, "btn_P_shuffle") == 0)  {_playerSubmenue = 1; _f_shuffle = true; preparePlaylistFromFolder(_curAudioFolder.c_str()); processPlaylist(true); changeState(PLAYER); return;}
        if(strcmp(name, "btn_P_fileList") == 0) {_playerSubmenue = 1; SD_listDir(_curAudioFolder.c_str(), true, false); changeState(AUDIOFILESLIST); return;}
        if(strcmp(name, "btn_P_radio") == 0)    {_playerSubmenue = 0; setStation(_cur_station); changeState(RADIO); return;}
    }
    if(_state == DLNA) {
        if(strcmp(name, "btn_D_Mute") == 0)     {_f_mute = btn_D_Mute.getValue(); muteChanged(_f_mute); return;}
        if(strcmp(name, "btn_D_pause") == 0)    {audioPauseResume(); return;}
        if(strcmp(name, "btn_D_volDown") == 0)  {downvolume(); showVolumeBar(); return;}
        if(strcmp(name, "btn_D_volUp") == 0)    {upvolume(); showVolumeBar(); return;}
        if(strcmp(name, "btn_D_radio") == 0)    {setStation(_cur_station); changeState(RADIO); return;}
        if(strcmp(name, "btn_D_fileList") == 0) {changeState(DLNAITEMSLIST); return;}
        if(strcmp(name, "btn_D_cancel") == 0)   {stopSong(); return;}
    }
    if(_state == CLOCK) {
        if(strcmp(name, "btn_C_Mute") == 0)     {_f_mute = btn_C_Mute.getValue(); muteChanged(_f_mute); return;}
        if(strcmp(name, "btn_C_alarm") == 0)    {changeState(ALARM); return;}
        if(strcmp(name, "btn_C_radio") == 0)    {_clockSubMenue = 0; changeState(RADIO); return;}
        if(strcmp(name, "btn_C_volDown") == 0)  {downvolume(); showVolumeBar(); return;}
        if(strcmp(name, "btn_C_volUp") == 0)    {upvolume(); showVolumeBar(); return;}
        if(strcmp(name, "clk_C_green") == 0)    {_clockSubMenue = 1; changeState(CLOCK); return;}
    }
    if(_state == ALARM) {
        if(strcmp(name, "clk_A_red") == 0)      {return;}
        if(strcmp(name, "btn_A_left") == 0)     {clk_A_red.shiftLeft(); return;}
        if(strcmp(name, "btn_A_right") == 0)    {clk_A_red.shiftRight(); return;}
        if(strcmp(name, "btn_A_up") == 0)       {clk_A_red.digitUp(); return;}
        if(strcmp(name, "btn_A_down") == 0)     {clk_A_red.digitDown(); return;}
        if(strcmp(name, "btn_A_ready") == 0)    {updateSettings(); _clockSubMenue = 0; changeState(CLOCK); logAlarmItems(); return;}
    }
    if(_state == EQUALIZER) {
        if(strcmp(name, "btn_E_Radio") == 0)    {setStation(_cur_station); changeState(RADIO); return;}
        if(strcmp(name, "btn_E_Player") == 0)   {changeState(PLAYER); return;}
        if(strcmp(name, "btn_E_Mute") == 0)     {_f_mute = btn_E_Mute.getValue(); muteChanged(_f_mute); return;}
    }
    log_d("unused event: graphicObject %s was released", name);
}
// clang-format on