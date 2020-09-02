#include<string.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <mpu6050_esp32.h>
#include <WiFi.h> 
#include <math.h>
#ifndef CardSelect_h
#define CardSelect_h
#include "Arduino.h"

class CardSelect
{
  private:
    TFT_eSPI* tft;
    
    //color constants
    #define BACKGROUND TFT_BLACK
    #define RED TFT_RED
    #define BLACK TFT_BLUE
    #define SELECT TFT_WHITE
  
    //button constants
    uint8_t BUTTON_PIN_1; 
    uint8_t BUTTON_PIN_2;
  
    //state transition constants
    #define IDLE 0
    #define PUSHED 1
    #define RELEASE 2
    #define CHANGE_SCREEN 3
    #define PLAYED 4
    uint8_t select_state;
    uint8_t play_state;

    //http variables
    char body[2000]; //for body
    const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
    const int POSTING_PERIOD = 6000; //periodicity of getting a number fact.
    const int GETTING_PERIOD = 60000;//one get once every 60 seconds
    static const uint16_t IN_BUFFER_SIZE = 10000; //size of buffer to hold HTTP request
    static const uint16_t OUT_BUFFER_SIZE = 10000; //size of buffer to hold HTTP response
    char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
    char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
  
    //imu constants
    MPU6050* imu;
  
    //self information
    char username[10];
    char game_id[10];
    char cards[500]; 
    //each card takes 4 characters, representing value and suit, _: cursor is on that cards, +: selected to be played
    bool is_landlord_self=false;
    bool is_turn_self=false;
    bool is_last_player_self=false;

    //other gaming information
    char cards_last_played[500];
    char landlord[10];
    char peasant1[10];
    char peasant2[10];
    char num_cards_left_landlord[10];
    char num_cards_left_peasant1[10];
    char num_cards_left_peasant2[10];
    char current_player[10];
    char last_player[10];
    uint32_t update_timer=millis();

    //other player 1 information
    char other_user1[10];
    int num_cards_left1;
    bool is_landlord1=false;
    bool is_turn1=false;
    bool is_last_player1=false;

    //other player 2 information
    char other_user2[10];
    int num_cards_left2;
    bool is_landlord2=false;
    bool is_turn2=false;
    bool is_last_player2=false;
    
    //draw board constants
    int timer_down = NULL;
    int timer_up = NULL;
    int timer_left = NULL;
    int timer_right = NULL;
    int timer1=millis();
    int current_line = 0;
    int current_spot = 0;
    int prev_line = NULL;
    int prev_spot = NULL;
    int screen_width = 127;
    int screen_height = 159;
    int screen_num = 1; // 0 for displaying cards, 1 for looking at other player's info
  public:
    CardSelect(TFT_eSPI *tft_to_use, MPU6050 *imu_to_use, uint8_t button_pin1_to_use, uint8_t button_pin2_to_use);
    void iterate();
    bool completePlay();
    void reInit();
    void draw_start_screen();
    void set_userInfo(char* username_to_use,char* game_id_to_use);
    void get_initial_info();
    char winner_user_name[20];
  private:
    void draw_screen();
    void change_state1(int input);
    void change_state2(int input);
    void detect_imu();
    void get_game_updates();
    void draw_hearts();
    void draw_clubs();
    void draw_diamonds();
    void draw_spades();
    void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial);
    uint8_t char_append(char* buff, char c, uint16_t buff_size);
};
#endif
