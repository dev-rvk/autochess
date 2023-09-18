//***********************************************
//
//            AUTOMATIC CHESSBOARD
//
//***********************************************

//******************************  INCLUDING FILES
#include "global.h"
#include "Micro_Max.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x20, 16, 2);

//****************************************  SETUP
void setup() {
  Serial.begin(9600);

  //  Electromagnet
  pinMode (MAGNET, OUTPUT);

  //  Motor
  pinMode (MOTOR_WHITE_STEP, OUTPUT);
  pinMode (MOTOR_WHITE_DIR, OUTPUT);
  pinMode (MOTOR_BLACK_STEP, OUTPUT);
  pinMode (MOTOR_BLACK_DIR, OUTPUT);

  //  Multiplexer
  for (byte i = 0; i < 4; i++) {
    pinMode (MUX_ADDR [i], OUTPUT);
    digitalWrite(MUX_ADDR [i], LOW);
    pinMode (MUX_SELECT [i], OUTPUT);
    digitalWrite(MUX_SELECT [i], HIGH);
  }
  pinMode (MUX_OUTPUT, INPUT_PULLUP);

  //  Set the reed switches status
  for (byte i = 2; i < 6; i++) {
    for (byte j = 0; j < 8; j++) {
      reed_sensor_status[i][j] = 1;
      reed_sensor_status_memory[i][j] = 1;
    }
  }

  //  MicroMax
  lastH[0] = 0;

  //  LCD
  lcd.init();

  //  Countdown
  timer = millis();

  //  Arcade button - Limit Switch
  pinMode (BUTTON_WHITE_SWITCH_MOTOR_WHITE, INPUT_PULLUP);
  pinMode (BUTTON_BLACK_SWITCH_MOTOR_BLACK, INPUT_PULLUP);

  lcd_display();
}

//*****************************************  LOOP
void loop() {

  switch (sequence) {
    case start:
      lcd_display();
      if (button(WHITE) == true) {  // HvsH Mode
        game_mode = HvsH;
        sequence = player_white;
      }
      else if (button(BLACK) == true) {  // HvsC Mode
        game_mode = HvsC;
        sequence = calibration;
      }
      break;

    case calibration:
      lcd_display();
      calibrate();
      sequence = player_white;
      break;

    case player_white:
      if (millis() - timer > 995) {  // Display the white player clock
        countdown();
        lcd_display();
      }
      detect_human_movement();
      if (button(WHITE) == true) {  // White player end turn
        new_turn_countdown = true;
        player_displacement();
        if (game_mode == HvsH) {
          AI_HvsH();  // Chekc is movement is valid
          if (no_valid_move == false) sequence = player_black;
          else lcd_display();
        }
        else if (game_mode == HvsC) {
          AI_HvsC();
          sequence = player_black;
        }
        break;

      case player_black:
        //  Game mode HvsH
        if (game_mode == HvsH) {  // Display the black player clock
          if (millis() - timer > 995) {
            countdown();
            lcd_display();
          }
          detect_human_movement();
          if (button(BLACK) == true) {  // Black human player end turn
            new_turn_countdown = true;
            player_displacement();
            AI_HvsH();  // Chekc is movement is valid
            if (no_valid_move == false) sequence = player_white;
            else lcd_display();
          }
        }
        //  Game mode HvsC
        else if (game_mode == HvsC) {
          black_player_movement();  //  Move the black chess piece
          sequence = player_white;
        }
        break;
      }
  }
}

//***************************************  SWITCH
boolean button(byte type) {

  if (type == WHITE && digitalRead(BUTTON_WHITE_SWITCH_MOTOR_WHITE) != HIGH) {
    delay(250);
    return true;
  }
  if (type == BLACK && digitalRead(BUTTON_BLACK_SWITCH_MOTOR_BLACK) != HIGH) {
    delay(250);
    return true;
  }
  return false;
}

//************************************  CALIBRATE
void calibrate() {

  //  Slow displacements up to touch the limit switches
  while (digitalRead(BUTTON_WHITE_SWITCH_MOTOR_WHITE) == HIGH) motor(B_T, SPEED_SLOW, calibrate_speed);
  while (digitalRead(BUTTON_BLACK_SWITCH_MOTOR_BLACK) == HIGH) motor(L_R, SPEED_SLOW, calibrate_speed);
  delay(500);

  //  Rapid displacements up to the Black start position (e7)
  motor(R_L, SPEED_FAST, TROLLEY_START_POSITION_X);
  motor(T_B, SPEED_FAST, TROLLEY_START_POSITION_Y);
  delay(500);
}

//****************************************  MOTOR
void motor(byte direction, int speed, float distance) {

  float step_number = 0;

  //  Calcul the distance
  if (distance == calibrate_speed) step_number = 4;
  else if (direction == LR_BT || direction == RL_TB || direction == LR_TB || direction == RL_BT) step_number = distance * SQUARE_SIZE * 1.44; //  Add an extra length for the diagonal
  else step_number = distance * SQUARE_SIZE;

  //  Direction of the motor rotation
  if (direction == R_L || direction == T_B || direction == RL_TB) digitalWrite(MOTOR_WHITE_DIR, HIGH);
  else digitalWrite(MOTOR_WHITE_DIR, LOW);
  if (direction == B_T || direction == R_L || direction == RL_BT) digitalWrite(MOTOR_BLACK_DIR, HIGH);
  else digitalWrite(MOTOR_BLACK_DIR, LOW);

  //  Active the motors
  for (int x = 0; x < step_number; x++) {
    if (direction == LR_TB || direction == RL_BT) digitalWrite(MOTOR_WHITE_STEP, LOW);
    else digitalWrite(MOTOR_WHITE_STEP, HIGH);
    if (direction == LR_BT || direction == RL_TB) digitalWrite(MOTOR_BLACK_STEP, LOW);
    else digitalWrite(MOTOR_BLACK_STEP, HIGH);
    delayMicroseconds(speed);
    digitalWrite(MOTOR_WHITE_STEP, LOW);
    digitalWrite(MOTOR_BLACK_STEP, LOW);
    delayMicroseconds(speed);
  }
}

// *******************************  ELECTROMAGNET
void electromagnet(boolean state) {

  if (state == true)  {
    digitalWrite(MAGNET, HIGH);
    delay(600);
  }
  else  {
    delay(600);
    digitalWrite(MAGNET, LOW);
  }
}

// ***********************************  COUNTDONW
void countdown() {

  //  Set the time of the current player
  if (new_turn_countdown == true ) {
    new_turn_countdown = false;
    if (sequence == player_white) {
      second = second_white;
      minute = minute_white;
    }
    else if (sequence == player_black) {
      second = second_black;
      minute = minute_black;
    }
  }

  //  Countdown
  timer = millis();
  second = second - 1;
  if (second < 1) {
    second = 60;
    minute = minute - 1;
  }

  //  Record the white player time
  if (sequence == player_white) {
    second_white = second;
    minute_white = minute;
  }
  //  Record the black player time
  else if (sequence == player_black) {
    second_black = second;
    minute_black = minute;
  }
}

// ***********************  BLACK PLAYER MOVEMENT
void black_player_movement() {

  //  Convert the AI characters in variables
  int departure_coord_X = lastM[0] - 'a' + 1;
  int departure_coord_Y = lastM[1] - '0';
  int arrival_coord_X = lastM[2] - 'a' + 1;
  int arrival_coord_Y = lastM[3] - '0';
  byte displacement_X = 0;
  byte displacement_Y = 0;

  //  Trolley displacement to the starting position
  int convert_table [] = {0, 7, 6, 5, 4, 3, 2, 1, 0};
  byte white_capturing = 1;
  if (reed_sensor_status_memory[convert_table[arrival_coord_Y]][arrival_coord_X - 1] == 0) white_capturing = 0;

  for (byte i = white_capturing; i < 2; i++) {
    if (i == 0) {
      displacement_X = abs(arrival_coord_X - trolley_coordinate_X);
      displacement_Y = abs(arrival_coord_Y - trolley_coordinate_Y);
    }
    else if (i == 1) {
      displacement_X = abs(departure_coord_X - trolley_coordinate_X);
      displacement_Y = abs(departure_coord_Y - trolley_coordinate_Y);
    }
    if (departure_coord_X > trolley_coordinate_X) motor(T_B, SPEED_FAST, displacement_X);
    else if (departure_coord_X < trolley_coordinate_X) motor(B_T, SPEED_FAST, displacement_X);
    if (departure_coord_Y > trolley_coordinate_Y) motor(L_R, SPEED_FAST, displacement_Y);
    else if (departure_coord_Y < trolley_coordinate_Y) motor(R_L, SPEED_FAST, displacement_Y);
    if (i == 0) {
      electromagnet(true);
      motor(R_L, SPEED_SLOW, 0.5);
      motor(B_T, SPEED_SLOW, arrival_coord_X - 0.5);
      electromagnet(false);
      motor(L_R, SPEED_FAST, 0.5);
      motor(T_B, SPEED_FAST, arrival_coord_X - 0.5);
      trolley_coordinate_X = arrival_coord_X;
      trolley_coordinate_Y = arrival_coord_Y;
    }
  }
  trolley_coordinate_X = arrival_coord_X;
  trolley_coordinate_Y = arrival_coord_Y;

  //  Move the Black chess piece to the arrival position
  displacement_X = abs(arrival_coord_X - departure_coord_X);
  displacement_Y = abs(arrival_coord_Y - departure_coord_Y);

  electromagnet(true);
  //  Bishop displacement
  if (displacement_X == 1 && displacement_Y == 2 || displacement_X == 2 && displacement_Y == 1) {
    if (displacement_Y == 2) {
      if (departure_coord_X < arrival_coord_X) {
        motor(T_B, SPEED_SLOW, displacement_X * 0.5);
        if (departure_coord_Y < arrival_coord_Y) motor(L_R, SPEED_SLOW, displacement_Y);
        else motor(R_L, SPEED_SLOW, displacement_Y);
        motor(T_B, SPEED_SLOW, displacement_X * 0.5);
      }
      else if (departure_coord_X > arrival_coord_X) {
        motor(B_T, SPEED_SLOW, displacement_X * 0.5);
        if (departure_coord_Y < arrival_coord_Y) motor(L_R, SPEED_SLOW, displacement_Y);
        else motor(R_L, SPEED_SLOW, displacement_Y);
        motor(B_T, SPEED_SLOW, displacement_X * 0.5);
      }
    }
    else if (displacement_X == 2) {
      if (departure_coord_Y < arrival_coord_Y) {
        motor(L_R, SPEED_SLOW, displacement_Y * 0.5);
        if (departure_coord_X < arrival_coord_X) motor(T_B, SPEED_SLOW, displacement_X);
        else motor(B_T, SPEED_SLOW, displacement_X);
        motor(L_R, SPEED_SLOW, displacement_Y * 0.5);
      }
      else if (departure_coord_Y > arrival_coord_Y) {
        motor(R_L, SPEED_SLOW, displacement_Y * 0.5);
        if (departure_coord_X < arrival_coord_X) motor(T_B, SPEED_SLOW, displacement_X);
        else motor(B_T, SPEED_SLOW, displacement_X);
        motor(R_L, SPEED_SLOW, displacement_Y * 0.5);
      }
    }
  }
  //  Diagonal displacement
  else if (displacement_X == displacement_Y) {
    if (departure_coord_X > arrival_coord_X && departure_coord_Y > arrival_coord_Y) motor(RL_BT, SPEED_SLOW, displacement_X);
    else if (departure_coord_X > arrival_coord_X && departure_coord_Y < arrival_coord_Y) motor(LR_BT, SPEED_SLOW, displacement_X);
    else if (departure_coord_X < arrival_coord_X && departure_coord_Y > arrival_coord_Y) motor(RL_TB, SPEED_SLOW, displacement_X);
    else if (departure_coord_X < arrival_coord_X && departure_coord_Y < arrival_coord_Y) motor(LR_TB, SPEED_SLOW, displacement_X);
  }
  //  Kingside castling
  else if (departure_coord_X == 5 && departure_coord_Y == 8 && arrival_coord_X == 7 && arrival_coord_Y == 8) {  //  Kingside castling
    motor(R_L, SPEED_SLOW, 0.5);
    motor(T_B, SPEED_SLOW, 2);
    electromagnet(false);
    motor(T_B, SPEED_FAST, 1);
    motor(L_R, SPEED_FAST, 0.5);
    electromagnet(true);
    motor(B_T, SPEED_SLOW, 2);
    electromagnet(false);
    motor(T_B, SPEED_FAST, 1);
    motor(R_L, SPEED_FAST, 0.5);
    electromagnet(true);
    motor(L_R, SPEED_SLOW, 0.5);
  }
  else if (departure_coord_X == 5 && departure_coord_Y == 8 && arrival_coord_X == 3 && arrival_coord_Y == 8) {  //  Queenside castling
    motor(R_L, SPEED_SLOW, 0.5);
    motor(B_T, SPEED_SLOW, 2);
    electromagnet(false);
    motor(B_T, SPEED_FAST, 2);
    motor(L_R, SPEED_FAST, 0.5);
    electromagnet(true);
    motor(T_B, SPEED_SLOW, 3);
    electromagnet(false);
    motor(B_T, SPEED_FAST, 1);
    motor(R_L, SPEED_FAST, 0.5);
    electromagnet(true);
    motor(L_R, SPEED_SLOW, 0.5);
  }
  //  Horizontal displacement
  else if (displacement_Y == 0) {
    if (departure_coord_X > arrival_coord_X) motor(B_T, SPEED_SLOW, displacement_X);
    else if (departure_coord_X < arrival_coord_X) motor(T_B, SPEED_SLOW, displacement_X);
  }
  //  Vertical displacement
  else if (displacement_X == 0) {
    if (departure_coord_Y > arrival_coord_Y) motor(R_L, SPEED_SLOW, displacement_Y);
    else if (departure_coord_Y < arrival_coord_Y) motor(L_R, SPEED_SLOW, displacement_Y);
  }
  electromagnet(false);

  //  Upadte the reed sensors states with the Balck move
  reed_sensor_status_memory[convert_table[departure_coord_Y]][departure_coord_X - 1] = 1;
  reed_sensor_status_memory[convert_table[arrival_coord_Y]][arrival_coord_X - 1] = 0;
  reed_sensor_status[convert_table[departure_coord_Y]][departure_coord_X - 1] = 1;
  reed_sensor_status[convert_table[arrival_coord_Y]][arrival_coord_X - 1] = 0;
}

//**********************************  LCD DISPLAY
void lcd_display() {

  lcd.backlight();

  if (no_valid_move == true) {
    lcd.setCursor(0, 0);
    lcd.print("  NO VALID MOVE  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    delay(2000);
    no_valid_move = false;
    return;
  }

  switch (sequence) {
    case start_up:
      lcd.setCursor(0, 0);
      lcd.print("   AUTOMATIC    ");
      lcd.setCursor(0, 1);
      lcd.print("   CHESSBOARD   ");
      sequence = start;
      delay(4000);
    case start:
      lcd.setCursor(0, 0);
      lcd.print(" PRESS A - HvsH ");
      lcd.setCursor(0, 1);
      lcd.print(" PRESS B - HvsC ");
      break;
    case calibration:
      lcd.setCursor(0, 0);
      lcd.print("  CALIBRATION   ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      break;
    case player_white:
      lcd.setCursor(0, 0);
      lcd.print("     WHITE      ");
      lcd.setCursor(0, 1);
      lcd.print("     " + String(minute) + " : " + String(second) + "     ");
      break;
    case player_black:
      lcd.setCursor(0, 0);
      lcd.print("     BLACK      ");
      lcd.setCursor(0, 1);
      lcd.print("     " + String(minute) + " : " + String(second) + "     ");
      break;
  }
}

//************************  DETECT HUMAN MOVEMENT
void detect_human_movement() {

  //  Record the reed switches status
  byte column = 6;
  byte row = 0;

  for (byte i = 0; i < 4; i++) {
    digitalWrite(MUX_SELECT[i], LOW);
    for (byte j = 0; j < 16; j++) {
      for (byte k = 0; k < 4; k++) {
        digitalWrite(MUX_ADDR [k], MUX_CHANNEL [j][k]);
      }
      reed_sensor_record[column][row] = digitalRead(MUX_OUTPUT);
      row++;
      if (j == 7) {
        column++;
        row = 0;
      }
    }
    for (byte l = 0; l < 4; l++) {
      digitalWrite(MUX_SELECT[l], HIGH);
    }
    if (i == 0) column = 4;
    if (i == 1) column = 2;
    if (i == 2) column = 0;
    row = 0;
  }
  for (byte i = 0; i < 8; i++) {
    for (byte j = 0; j < 8; j++) {
      reed_sensor_status_memory[7 - i][j] = reed_sensor_record[i][j];
    }
  }

  //  Compare the old and new status of the reed switches

  for (byte i = 0; i < 8; i++) {
    for (byte j = 0; j < 8; j++) {
      if (reed_sensor_status[i][j] != reed_sensor_status_memory[i][j]) {
        if (reed_sensor_status_memory[i][j] == 1) {
          reed_colone[0] = i;
          reed_line[0] = j;
        }
        if (reed_sensor_status_memory[i][j] == 0) {
          reed_colone[1] = i;
          reed_line[1] = j;
        }
      }
    }
  }
  //  Set the new status of the reed sensors
  for (byte i = 0; i < 8; i++) {
    for (byte j = 0; j < 8; j++) {
      reed_sensor_status[i][j] = reed_sensor_status_memory[i][j];
    }
  }
}

//**************************  PLAYER DISPLACEMENT
void player_displacement() {

  //  Convert the reed sensors switches coordinates in characters
  char table1[] = {'8', '7', '6', '5', '4', '3', '2', '1'};
  char table2[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

  mov[0] = table2[reed_line[0]];
  mov[1] = table1[reed_colone[0]];
  mov[2] = table2[reed_line[1]];
  mov[3] = table1[reed_colone[1]];
}
