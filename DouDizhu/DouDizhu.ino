#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <mpu6050_esp32.h>
#include <WiFi.h> //Connect to WiFi Network
#include <math.h>
#include "CardSelect.h"
#include "UserLogin.h"
#include "WaitingRoom.h"
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

//color constants
#define BACKGROUND TFT_BLACK

#define USERLOGIN 0
#define WAITING 1
#define PLAYCARD 2
#define TERMINATE 3
uint8_t state;
uint8_t old_state;

//constants for WIFI access
char network[] = "3-JOHN";  //SSID
char password[] = "28af64df55cf"; //Password

//button constants
const uint8_t BUTTON_PIN_1=16;
const uint8_t BUTTON_PIN_2=5;

MPU6050 imu;

//user-related variables
char* username;
char* game_id;
UserLogin userLogin(&tft,&imu,BUTTON_PIN_1,BUTTON_PIN_2);
CardSelect cardSelect(&tft,&imu,BUTTON_PIN_1,BUTTON_PIN_2);
WaitingRoom waitingRoom(&tft,BUTTON_PIN_1,BUTTON_PIN_2);

void setup() {
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND);
  delay(100);
  Serial.begin(115200); //for debugging if needed.

  //setup wifi
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }

  //setup pin
  pinMode(BUTTON_PIN_1, INPUT_PULLUP);
  pinMode(BUTTON_PIN_2, INPUT_PULLUP);

  //setup IMU
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  state=USERLOGIN;
  old_state=USERLOGIN;
  userLogin.draw_start_screen();
}

void loop() {
  switch(state){
    case USERLOGIN:
      userLogin.iterate();
      if(userLogin.completeLogin()){
        Serial.println("change from login to wait");
        state=WAITING;
        username=userLogin.get_username();
        Serial.println(username);
      }
      break;
    case WAITING:
      waitingRoom.set_username(username);
      waitingRoom.set_wait();
      waitingRoom.drawWaiting();
      waitingRoom.iterate();
      
      if(waitingRoom.startPlay()){
        Serial.println("change from wait to play");
        game_id = waitingRoom.get_game_id();
        state=PLAYCARD;
        old_state=WAITING;
      }
      if (waitingRoom.cancelPlay()) {
        Serial.println("cancelled");
        state = TERMINATE;
        tft.fillScreen(BACKGROUND);
      }
      break;
    case PLAYCARD:
      if(old_state==WAITING||old_state=TERMINATE){
        cardSelect.set_userInfo(username,game_id);
        cardSelect.get_initial_info();
        cardSelect.draw_start_screen();
        old_state=PLAYCARD;
      }
      if (!cardSelect.completePlay()) cardSelect.iterate();
      else {
        tft.setTextColor(TFT_WHITE, BACKGROUND);
        tft.fillScreen(BACKGROUND);
        if (strstr(cardSelect.winner_user_name, username)) {
          tft.drawString("Congratulations!", 0, 0, 2);
          tft.drawString("You WON!", 0, 20, 2);
          while()
        }
        else {
          tft.setTextColor(TFT_WHITE, BACKGROUND);
          tft.fillScreen(BACKGROUND);
          tft.drawString("You Lost :(", 0, 0, 2);
        }
        state=TERMINATE;  
        tft.fillScreen(BACKGROUND);  
      }
      break;
    case TERMINATE:
      Serial.println("state changed to terminate here");
      //TO DO: add more terminate things here
      tft.setTextColor(TFT_WHITE, BACKGROUND);
      tft.drawString("Do you wish to play again?\nButton 1 for YES, Button 2 for NO", 0, 0, 2);
      bool pressed_1 = !digitalRead(BUTTON_PIN_1); //if we have pressed first button
      bool pressed_2 = !digitalRead(BUTTON_PIN_2);
      if (pressed_1){
        const int RESPONSE_TIMEOUT=6000; //ms to wait for response from host
        static const uint16_t IN_BUFFER_SIZE=1000; //size of buffer to hold HTTP request
        static const uint16_t OUT_BUFFER_SIZE=1000; //size of buffer to hold HTTP response
        char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
        char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
        char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
        char request[500];
        char body[200];
        sprintf(body, "action=end&username=%s", username);
        int body_len = strlen(body);
        sprintf(request_buffer, "POST http://608dev-2.net/sandbox/sc/team018/week04/user_handler.py?action=end&username=%s HTTP/1.1\r\n", username);
        strcat(request_buffer, "Host: 608dev-2.net\r\n");
        strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        strcat(request_buffer, "\r\n"); //new line from header to body
        strcat(request_buffer, body); //body
        strcat(request_buffer, "\r\n"); //header
        do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
        state = WAITING;
        old_state=TERMINATE;
        cardSelect.reInit();
        tft.fillScreen(BACKGROUND);

      }
      if (pressed_2){
        tft.fillScreen(BACKGROUND);
        tft.setTextColor(TFT_WHITE, BACKGROUND);
        tft.drawString("Thanks for playing", 0, 0, 2);
        ESP.restart();  
      }
      break;
  }
  
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
void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
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
      char_append(response, client.read(), 1000);
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
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}
