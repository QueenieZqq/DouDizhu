#include<string.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <mpu6050_esp32.h>
#include <WiFi.h> 
#include <math.h>
#include "CardSelect.h"
#include "Arduino.h"


CardSelect::CardSelect(TFT_eSPI* tft_to_use,MPU6050* imu_to_use,uint8_t button_pin1_to_use,uint8_t button_pin2_to_use){
  tft=tft_to_use;
  imu=imu_to_use;
  BUTTON_PIN_1=button_pin1_to_use;
  BUTTON_PIN_2=button_pin2_to_use;
  
  //initialize states and timer
  select_state = IDLE;
  play_state = IDLE;
  timer_down = millis();
  timer_up = millis();
  timer_left = millis();
  timer_right = millis();
  current_line = 0;
  current_spot = 0;
}

void CardSelect::set_userInfo(char* username_to_use, char* game_id_to_use){
  memset(username, 0, 10);
  memset(game_id, 0, 10);
  for(int i = 0; i < strlen(username_to_use); i++) {
    username[i] = username_to_use[i];
  }
  for(int i = 0; i < strlen(game_id_to_use); i++) {
    game_id[i] = game_id_to_use[i];
  }
}

void CardSelect::reInit(){
  select_state = IDLE;
  play_state = IDLE;
  timer_down = millis();
  timer_up = millis();
  timer_left = millis();
  timer_right = millis();
  current_line = 0;
  current_spot = 0;
}

void CardSelect::get_initial_info(){
  int body_len = strlen(body); //calculate body length (for header reporting)
  sprintf(request_buffer, "GET http://608dev-2.net/sandbox/sc/team018/week04/game_handler.py?username=%s&game_ID=%s HTTP/1.1\r\n", username, game_id);
  strcat(request_buffer, "Host: 608dev-2.net\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n"); //new line from header to body
  strcat(request_buffer, body); //body
  strcat(request_buffer, "\r\n"); //header
  do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  memset(body,0,strlen(body));

  //get cards in hand
  int j = 0;
  int i = 0;
  while(response_buffer[i] != '%'){
    cards[j] = response_buffer[i];
    cards[j+1] = response_buffer[i+1];
    cards[j+2] = ' ';
    cards[j+3] = ' ';
    cards[j+4]=',';
    i = i + 3;
    j = j + 5;
  }
  cards[2] = '_';
  
  while(response_buffer[i] == '%') {
    i++;
  }

  // get player info
  memset(landlord, 0, 10);
  j = 0;
  while(response_buffer[i] != '%') {
    landlord[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;
  memset(peasant1, 0, 10);
  j = 0;
  while(response_buffer[i] != '%') {
    peasant1[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;
  memset(peasant2, 0, 10);
  j = 0;
  while(response_buffer[i] != '%') {
    peasant2[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;

  //get current player
  memset(current_player, 0, 10);
  j = 0;
  while(response_buffer[i] != '%') {
    current_player[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;

  //set some player info
  if (strcmp(landlord, username) == 0) {
    is_landlord_self = true;
    is_landlord1 = false;
    is_landlord2 = false;
    is_turn_self = true;
    is_turn1 = false;
    is_turn2 = false;
    num_cards_left1 = 17;
    num_cards_left2 = 17;
    strcpy(other_user1, peasant1);
    strcpy(other_user2, peasant2);
  }
  else {
    is_landlord_self = false;
    is_landlord1 = true;
    is_landlord2 = false;
    is_turn_self = false;
    is_turn1 = true;
    is_turn2 = false;
    num_cards_left1 = 20;
    num_cards_left2 = 17;
    strcpy(other_user1, landlord);
    if (strcmp(peasant1, username) == 0) {
      strcpy(other_user2, peasant2);
    }
    else {
      strcpy(other_user2, peasant1);
    }
  }
}

void CardSelect::iterate() {
  if(millis()-update_timer>1000){
    get_game_updates();
    update_timer=millis();
  }
  detect_imu();
  change_state1(digitalRead(BUTTON_PIN_1));
  change_state2(digitalRead(BUTTON_PIN_2));
}


void CardSelect::get_game_updates(){
//  Serial.println("in updates");
  bool need_draw_screen = false;
  int body_len = strlen(body); //calculate body length (for header reporting)
  sprintf(request_buffer, "GET http://608dev-2.net/sandbox/sc/team018/week04/game_handler.py?username=%s&game_ID=%s HTTP/1.1\r\n", username, game_id);
  strcat(request_buffer, "Host: 608dev-2.net\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n"); //new line from header to body
  strcat(request_buffer, body); //body
  strcat(request_buffer, "\r\n"); //header
  do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  memset(body,0,strlen(body));

  //get cards in hand
  int j = 0;
  int i = 0;
  char cards_[500];
  memset(cards_, 0, 500);
  while(response_buffer[i] != '%'){
    if (i == 0) {
      i = i - 1;
    }
    i = i + 1;
    cards_[j] = response_buffer[i];
    cards_[j+1] = response_buffer[i+1];
    cards_[j+2] = ' ';
    cards_[j+3] = ' ';
    cards_[j+4]=',';
    i = i + 2;
    j = j + 5;
  }
  cards_[2] = '_';
  i++;

  //get cards last played
  memset(cards_last_played, 0, 500);
  j = 0;
  while(response_buffer[i] != '%') {
    cards_last_played[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;

  //get last player info
  memset(last_player, 0, 10);
  j = 0;
  while(response_buffer[i] != '%') {
    last_player[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;
  if (strcmp(last_player, username) == 0) {
    if (is_last_player_self != true) {
      need_draw_screen = true;
    }
    is_last_player_self = true;
    is_last_player1 = false;
    is_last_player2 = false;   
  }
  else if (strcmp(last_player, other_user1) == 0) {
    if (is_last_player1 != true) {
      need_draw_screen = true;
    }
    is_last_player_self = false;
    is_last_player1 = true;
    is_last_player2 = false;   
  }
  else if (strcmp(last_player, other_user2) == 0) {
    if (is_last_player2 != true) {
      need_draw_screen = true;
    }
    is_last_player_self = false;
    is_last_player1 = false;
    is_last_player2 = true;   
  }
  else {
    is_last_player_self = false;
    is_last_player1 = false;
    is_last_player2 = false;   
  }

  // get player info
  while(response_buffer[i] != '%') {
    i++;
    j++;
  }
  i++;
  while(response_buffer[i] != '%') {
    i++;
    j++;
  }
  i++;
  while(response_buffer[i] != '%') {
    i++;
    j++;
  }
  i++;

  //get current player
  memset(current_player, 0, 10);
  j = 0;
  while(response_buffer[i] != '%') {
    current_player[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;
  if (strcmp(current_player, username) == 0) {
    if (is_turn_self != true) {
      need_draw_screen = true;
    }
    is_turn_self = true;
    is_turn1 = false;
    is_turn2 = false;
  }
  else if (strcmp(current_player, other_user1) == 0) {
    if (is_turn1 != true) {
      need_draw_screen = true;
    }
    is_turn_self = false;
    is_turn1 = true;
    is_turn2 = false;
  }
  else if (strcmp(current_player, other_user2) == 0) {
    if (is_turn2 != true) {
      need_draw_screen = true;
    }
    is_turn_self = false;
    is_turn1 = false;
    is_turn2 = true;
  }
  else {
    is_turn_self = false;
    is_turn1 = false;
    is_turn2 = false;
  }

  // set num cards left for landlord
  memset(num_cards_left_landlord, 0, 10);
  j = 0;
  while(response_buffer[i] != '%') {
    num_cards_left_landlord[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;
  if (strcmp(landlord, username) == 0) {
    // do nothing
  }
  else if (strcmp(landlord, other_user1) == 0) {
    num_cards_left1 = atoi(num_cards_left_landlord);
  }
  else {
    num_cards_left2 = atoi(num_cards_left_landlord);
  }

  // set num cards left for peasant1
  memset(num_cards_left_peasant1, 0, 10);
  j = 0;
  while(response_buffer[i] != '%') {
    num_cards_left_peasant1[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;
  if (strcmp(peasant1, username) == 0) {
    // do nothing
  }
  else if (strcmp(peasant1, other_user1) == 0) {
    num_cards_left1 = atoi(num_cards_left_peasant1);
  }
  else {
    num_cards_left2 = atoi(num_cards_left_peasant1);
  }

  // set num cards left for peasant2
  memset(num_cards_left_peasant2, 0, 10);
  j = 0;
  
  while(i < strlen(response_buffer)) {
    num_cards_left_peasant2[j] = response_buffer[i];
    i++;
    j++;
  }
  i++;
  if (strcmp(peasant2, username) == 0) {
    // do nothing
  }
  else if (strcmp(peasant2, other_user1) == 0) {
    num_cards_left1 = atoi(num_cards_left_peasant2);
  }
  else {
    num_cards_left2 = atoi(num_cards_left_peasant2);
  }

  if (need_draw_screen) {
    screen_num = 1;
    strcpy(cards, cards_);
    current_spot = 0;
    current_line = 0;
    prev_line = 0;
    prev_spot = 0;
    draw_screen();
  }
}

void CardSelect::draw_start_screen(){
  tft->fillScreen(BACKGROUND);
  tft->setTextColor(SELECT, BACKGROUND);
  tft->drawString("Let's start playing!", 0, 0, 2);
  char tmp[100];
  sprintf(tmp, "The landlord is %s!", landlord);
  tft->drawString(tmp, 0, 20, 2);

    //display self
    sprintf(tmp, " me %02d ", strlen(cards) / 5);
    if (is_landlord_self) {
      tft->fillRect(64 - 6 * (strlen(tmp) + 2) / 2, 145, 6 * (strlen(tmp) + 2) - 2, 15, RED);
      tft->setTextColor(SELECT, RED);
    }
    else {
      tft->setTextColor(SELECT, BACKGROUND); 
    }
    tft->drawString(tmp, 64 - 6 * (strlen(tmp) + 2) / 2, 145, 2);
    if (is_turn_self) {
      tft->drawRect(64 - 6 * (strlen(tmp) + 2) / 2, 145, 6 * (strlen(tmp) + 2) - 2, 15, SELECT);
    }

    //display other user 1
    if (is_landlord1) {
      tft->setTextColor(SELECT, RED);
      tft->fillRect(0, 85, 40, 31, RED);
    }
    else{
      tft->setTextColor(SELECT, BACKGROUND);
    }
    sprintf(tmp, " %s", other_user1);
    tft->drawString(tmp, 0, 85, 2);
    sprintf(tmp, " %02d", num_cards_left1);
    tft->drawString(tmp, 0, 100, 2);
    if (is_turn1) {
      tft->drawRect(0, 85, 40, 31, SELECT);
    }

    //display other user 2
    if (is_landlord2) {
      tft->setTextColor(SELECT, RED);
      tft->fillRect(screen_width - 43, 85, 43, 31, RED);
    }
    else{
      tft->setTextColor(SELECT, BACKGROUND);
    }
    sprintf(tmp, " %s", other_user2);
    tft->drawString(tmp, screen_width - 43, 85, 2);
    sprintf(tmp, " %02d", num_cards_left2);
    tft->drawString(tmp, screen_width - 43, 100, 2);
    if (is_turn2) {
      tft->drawRect(screen_width - 43, 85, 43, 31, SELECT);
    }
}


bool CardSelect::completePlay(){
  return play_state==PLAYED;
}

void CardSelect::change_state1(int input) {
  //for select/de-select cards
  switch(select_state) {
    case IDLE:
    if (input == 0) {
      select_state = PUSHED;
      timer1=millis();
    }
    break;
    case PUSHED:
    if (input == 1) {
      if(millis()-timer1>1000){
        select_state=CHANGE_SCREEN;
      }else{
        select_state = RELEASE;
      } 
    }
    break;
    case RELEASE:
    for (int i = 0; i < strlen(cards); i = i + 5) {
        if (cards[i + 2] == '_') {
          if (cards[i + 3] == '+') {
              cards[i + 3] = ' ';
          }
          else {
            cards[i + 3] = '+';
          }
        }
    }
    draw_screen();
    select_state = IDLE;
    break;

    case CHANGE_SCREEN:
    screen_num=1-screen_num;
    draw_screen();
    select_state=IDLE;
    break;
  }
}
    
void CardSelect::change_state2(int input) {
  // for playing cards
  switch(play_state) {
    case IDLE:
      if (input == 0) {
        play_state = PUSHED;
      }
      break;
    case PUSHED:
      if (input == 1) {
        play_state = RELEASE;
      }
      break;
    case RELEASE:{
      int count = 0;
      bool found = false;
      char cards_[500];
      memset(cards_, 0, 500);
      uint8_t j = 0;
      for (int i = 0; i < strlen(cards); i = i + 5) {
          if (cards[i + 3] == '+') {
              cards_[j++] = cards[i];
              cards_[j++] = cards[i+1];
              cards_[j++] = ',';
          }
      }
      cards_[strlen(cards_)-1]='\0';
      int body_len = strlen(body); //calculate body length (for header reporting)
      sprintf(request_buffer, "POST http://608dev-2.net/sandbox/sc/team018/week04/game_handler.py?username=%s&game_ID=%s&cards=%s HTTP/1.1\r\n", username, game_id, cards_);
      strcat(request_buffer, "Host: 608dev-2.net\r\n");
      strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
      sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
      strcat(request_buffer, "\r\n"); //new line from header to body
      strcat(request_buffer, body); //body
      strcat(request_buffer, "\r\n"); //header
      do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);

      if (strstr(response_buffer, "turn")){
        play_state = IDLE;
        tft->fillScreen(BACKGROUND);
        tft->setTextColor(SELECT,BACKGROUND);
        tft->drawString("Not Your Turn!",0,0,2);
        delay(3000);
        screen_num = 1;
        draw_screen();
      }
      else if (strstr(response_buffer, "legal")){
        play_state = IDLE;
        tft->fillScreen(BACKGROUND);
        tft->setTextColor(SELECT,BACKGROUND);
        tft->drawString("Illegal Hand!",0,0,2);
        delay(3000);
        screen_num = 1;
        draw_screen();
      }
      else if (strstr(response_buffer, "valid")){
        play_state=IDLE;
        tft->fillScreen(BACKGROUND);
        tft->setTextColor(BLACK,BACKGROUND);
        tft->drawString("Invalid Play!",0,0,2);
        delay(3000);
        screen_num = 1;
        draw_screen();
      }
      else if(strstr(response_buffer,"unknown")){
        draw_screen();
        play_state=IDLE;
      }
      else{
        if (strstr(response_buffer, "peasants")) {
          sprintf(winner_user_name, "%s and %s", peasant1, peasant2);
        }
        else if (strstr(response_buffer, "landlord")) {
          sprintf(winner_user_name, "%s", landlord);
        }
        play_state = PLAYED;
      }
      
      // update cards
      get_game_updates();
      break;
      }
    case PLAYED:
    delay(100);
    play_state=IDLE;
      break;
  }
}
    
void CardSelect::detect_imu() {
 if (screen_num == 0) {
    for (int i=0; i <  strlen(cards); i=i+5) {
      if (cards[i+2] == '_') {
        cards[i+2] = ' ';
      }
    }
    prev_line = current_line;
    prev_spot = current_spot;
    imu->readAccelData(imu->accelCount);//read imu
    float imu_x = -imu->accelCount[0] * imu->aRes * 15.0;
    float imu_y = -imu->accelCount[1] * imu->aRes * 15.0;
  
    int timer_offset = 400;
    int max_line;
    if (strlen(cards) / 5 == 10) {
      max_line = 1;
    }
    else if (strlen(cards) / 5 == 20) {
      max_line = 2;
    }
    else {
      max_line = strlen(cards) / 5 / 10 + 1;
    }
    int max_spot;
    if (current_line != max_line - 1) {
      max_spot = 10;
    }
    else {
      if (strlen(cards) / 5 == 10 || strlen(cards) / 5 == 20) {
        max_spot = 10;
      }
      else {
        max_spot = strlen(cards) / 5 % 10;
      }
    }
    
    if (imu_x > 5 && (timer_down == NULL || millis() - timer_down > timer_offset)) {
        current_line += 1;
        if (current_line >= max_line || (current_line * 10 + current_spot) * 5 + 2 >= strlen(cards)) {
          current_line = current_line - 1;    
        }
        timer_down = millis();
    }
    else if (imu_x < -5 && (timer_up == NULL || millis() - timer_up > timer_offset)) {
        current_line -= 1;
        if (current_line < 0) {
          current_line = 0;
        }
        timer_up = millis();
    }
    
    if (imu_y > 5 && (timer_right == NULL || millis() - timer_right > timer_offset)) {
        current_spot += 1;
        if (current_spot >= max_spot) {
          current_spot = max_spot - 1;
        }
        timer_right = millis();
    }
    else if (imu_y < -5 && (timer_left == NULL || millis() - timer_left > timer_offset)) {
        current_spot -= 1;
        if (current_spot < 0) {
          current_spot = 0;
        }
        timer_left = millis();
    } 
    cards[(current_line * 10 + current_spot) * 5 + 2] = '_';
    if (prev_line != current_line || prev_spot != current_spot) {
      draw_screen();
    }
  }
}
    
void CardSelect::draw_screen() {
  if(screen_num==0){
//    Serial.println(cards);
    tft->fillScreen(BACKGROUND);
    int x = 6;
    int y = 10;
    char out[10] = "";
    for (int i = 0; i < strlen(cards); i = i + 5) {
      if (cards[i + 1] == 'S' || cards[i + 1] == 'D') {
        if (cards[i + 3] == '+') {
          tft->setTextColor(BLACK, SELECT); 
        }
        else {
          tft->setTextColor(BLACK, BACKGROUND);
        }
      }
      else if (cards[i + 1] == 'H' || cards[i + 1] == 'C') {
        if (cards[i + 3] == '+') {
          tft->setTextColor(RED, SELECT); 
        }
        else {
          tft->setTextColor(RED, BACKGROUND);
        }
      }
      else {
        if (cards[i + 3] == '+') {
          tft->setTextColor(BACKGROUND, SELECT); 
        }
        else {
          tft->setTextColor(SELECT, BACKGROUND);
        }
      }
      memset(out, 0, 10);
      out[0] = cards[i];
      tft->drawString(out, x, y, 2);
      if (cards[i + 2] == '_') {
        tft->drawRect(x - 1, y, 10, 17, SELECT);
        if (cards[i + 1] == 'S' || cards[i + 1] == 'D') {
          tft->setTextColor(BLACK, BACKGROUND);
        }
        else {
          tft->setTextColor(RED, BACKGROUND);
        }
        tft->drawRect(15, 70, 105, 70, SELECT);
        memset(out, 0, 10);
        if (cards[i] == 'X') {
          out[0] = '1';
          out[1] = '0';
          tft->drawString(out, 30, 95, 4);
        }
        else if (cards[i] == 'o') {
          tft->setTextColor(SELECT, BACKGROUND);
          tft->drawString("J", 29, 95, 4);
          tft->drawString("K", 59, 95, 4);
          tft->drawString("R", 90, 95, 4);
          tft->drawString("O", 41, 95, 4);
          tft->drawString("E", 75, 95, 4);
        }
        else if (cards[i] == 'O') {
          tft->setTextColor(RED, BACKGROUND);
          tft->drawString("J", 29, 95, 4);
          tft->drawString("K", 59, 95, 4);
          tft->drawString("R", 90, 95, 4);
          tft->setTextColor(BLACK, BACKGROUND);
          tft->drawString("O", 41, 95, 4);
          tft->drawString("E", 75, 95, 4);
        }
        else {
          out[0] = cards[i];
          tft->drawString(out, 40, 95, 4);
        }
        if (cards[i + 1] == 'C') {
          draw_clubs();
        }
        if (cards[i + 1] == 'H') {
          draw_hearts();
        }
        if (cards[i + 1] == 'D') {
          draw_diamonds();
        }
        if (cards[i + 1] == 'S') {
          draw_spades();
        }
      }
      if (x + 12 < 115) {
        x = x + 12;
      }
      else {
        y = y + 20;
        x = 6;
      }
    }
  }else{
    tft->fillScreen(BACKGROUND);
    tft->setTextColor(SELECT, BACKGROUND);
    tft->drawString("Cards last played", 0, 0, 2);
    char out[10];
    if (strlen(last_player) == 0){
      sprintf(out, "is none");
    }
    else {
      sprintf(out, " by %s:", last_player);
    }
    tft->drawString(out, 0, 20, 2);
    int x = 6;
    int y = 40;
    for (int i = 0; i < strlen(cards_last_played); i = i + 3) {
      if (cards_last_played[i + 1] == 'S' || cards_last_played[i + 1] == 'D') {
        tft->setTextColor(BLACK, BACKGROUND);
      }
      else if (cards_last_played[i + 1] == 'H' || cards_last_played[i + 1] == 'C') {
        tft->setTextColor(RED, BACKGROUND);
      }
      else {
        tft->setTextColor(SELECT, BACKGROUND); 
      }
      
      memset(out, 0, 10);
      out[0] = cards_last_played[i];
      tft->drawString(out, x, y, 2);
      if (x + 12 < 115) {
        x = x + 12;
      }
      else {
        y = y + 20;
        x = 6;
      }
    }

    //display self
    char tmp[100];
    sprintf(tmp, " me %02d ", strlen(cards) / 5);
    if (is_landlord_self) {
      tft->fillRect(64 - 6 * (strlen(tmp) + 2) / 2, 145, 6 * (strlen(tmp) + 2) - 2, 15, RED);
      tft->setTextColor(SELECT, RED);
    }
    else {
      tft->setTextColor(SELECT, BACKGROUND); 
    }
    tft->drawString(tmp, 64 - 6 * (strlen(tmp) + 2) / 2, 145, 2);
    if (is_turn_self) {
      tft->drawRect(64 - 6 * (strlen(tmp) + 2) / 2, 145, 6 * (strlen(tmp) + 2) - 2, 15, SELECT);
    }

    //display other user 1
    if (is_landlord1) {
      tft->setTextColor(SELECT, RED);
      tft->fillRect(0, 85, 40, 31, RED);
    }
    else{
      tft->setTextColor(SELECT, BACKGROUND);
    }
    sprintf(tmp, " %s", other_user1);
    tft->drawString(tmp, 0, 85, 2);
    sprintf(tmp, " %02d", num_cards_left1);
    tft->drawString(tmp, 0, 100, 2);
    if (is_turn1) {
      tft->drawRect(0, 85, 40, 31, SELECT);
    }

    //display other user 2
    if (is_landlord2) {
      tft->setTextColor(SELECT, RED);
      tft->fillRect(screen_width - 43, 85, 43, 31, RED);
    }
    else{
      tft->setTextColor(SELECT, BACKGROUND);
    }
    sprintf(tmp, " %s", other_user2);
    tft->drawString(tmp, screen_width - 43, 85, 2);
    sprintf(tmp, " %02d", num_cards_left2);
    tft->drawString(tmp, screen_width - 43, 100, 2);
    if (is_turn2) {
      tft->drawRect(screen_width - 43, 85, 43, 31, SELECT);
    }
  }
}
    
void CardSelect::draw_hearts() {
  int l = 70;
  int r = 110;
  int t_tri = 100;
  int b_tri = 130;
  int radius = (r - l) / 4;
  tft->fillCircle(l + radius, t_tri, radius, RED);
  tft->fillCircle(r - radius, t_tri, radius, RED);
  tft->fillRect(l, t_tri, r - l, radius, BACKGROUND);
  tft->fillTriangle(l, t_tri, r, t_tri, (l + r)/ 2, b_tri, RED);
  
}
    
void CardSelect::draw_clubs() {
  int l = 70;
  int r = 110;
  int t = 80;
  int b = 130;
  int m = (t + b) / 2;
  tft->fillTriangle(l, m, r, m, (l + r) / 2, b, RED); 
  tft->fillTriangle(l, m, r, m, (l + r) / 2, t, RED); 
}
    
void CardSelect::draw_diamonds() {
  int l = 70;
  int r = 110;
  int t_tri = 110;
  int b_tri = 80;
  int radius = (r - l) / 4;
  tft->fillCircle(l + radius, t_tri, radius, BLACK);
  tft->fillCircle(r - radius, t_tri, radius, BLACK);
  tft->fillRect(l, t_tri - radius, r - l, radius, BACKGROUND);
  tft->fillTriangle(l, t_tri, r, t_tri, (l + r)/ 2, b_tri, BLACK);  
  tft->fillRect((l + r) / 2 - 1, t_tri, 3, 20, BLACK);
}
    
void CardSelect::draw_spades() {
  int radius = 10;
  int l = 70 + radius;
  int r = 110 - radius;
  int t = 80 + int(1.5 * radius);
  int b = 110;
  tft->fillCircle(l, b, radius, BLACK);
  tft->fillCircle(r, b, radius, BLACK);
  tft->fillCircle((l + r) / 2, t, radius, BLACK);
  tft->fillRect((l + r) / 2 - 1, t, 3, 37, BLACK);
}

/*----------------------------------
   do_http_request Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
void CardSelect::do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  serial = false;
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

/*----------------------------------
  char_append Function:
  Arguments:
     char* buff: pointer to character array which we will append a
     char c:
     uint16_t buff_size: size of buffer buff

  Return value:
     boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t CardSelect::char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}
