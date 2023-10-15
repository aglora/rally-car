// EMISOR - ESP8266

#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include "PCF8591.h"

// CONEXIONADO SPI NRF24:

// CE == D0
// CSN == D8
// SCK == D5
// MO == D7
// MI == D6

RF24 radio(16, 15);         // CE, CSN
uint8_t canalDeseado = 125; // Cambia esto al canal deseado (0-125)

const byte address[6] = "00001";

char command, command_ant_dir, command_ant_trac;
int speed, speed_ant;
char envio[3];
unsigned long lastNotice = 0;

PCF8591 pcf8591(0x48, 0);
uint8_t A0_trac, A0_trac_ant, A1_dir, A1_dir_ant;

void envio_command(char command, int speed);

void setup()
{
  Serial.begin(115200);
  // RF
  radio.begin();
  radio.setChannel(canalDeseado); // Configura el canal
  radio.openWritingPipe(address);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  // I2C
  Wire.begin();
  pcf8591.begin();

  command_ant_dir = 'G';
  command_ant_trac = 'S';
  speed = 0;
  speed_ant = 0;
}

void loop()
{
  // ENVIO NOTICE
  if((millis() - lastNotice)>500){
    lastNotice = millis();
    command = 'N';
    envio_command(command, speed);
  }

  // DIRECCIÓN
  A1_dir = pcf8591.adc_raw_read(1);
  if ((A1_dir < 230) && (A1_dir > 20) && (command_ant_dir != 'G'))
  {
    command = 'G';
    envio_command(command, speed);
    command_ant_dir = command;
  }
  else if ((A1_dir >= 230) && (command_ant_dir != 'L'))
  {
    command = 'L';
    envio_command(command, speed);
    command_ant_dir = command;
  }
  else if ((A1_dir <= 30) && (command_ant_dir != 'R'))
  {
    command = 'R';
    envio_command(command, speed);
    command_ant_dir = command;
  }
  delay(20);

  // TRACCIÓN
  A0_trac = pcf8591.adc_raw_read(0);
  if ((A0_trac < 145) && (A0_trac > 110) && (command_ant_trac != 'S'))
  {
    command = 'S';
    envio_command(command, 0);
    command_ant_trac = command;
  }
  else if ((A0_trac >= 145))
  {
    command = 'F';
    speed = map(A0_trac, 145, 255, 0, 9);
    if ((speed != speed_ant) || (command_ant_trac != 'F'))
    {
      speed_ant = speed;
      envio_command(command, speed);
      command_ant_trac = command;
    }
  }
  else if ((A0_trac <= 110))
  {
    command = 'B';
    speed = map(A0_trac, 0, 110, 9, 0);
    if ((speed != speed_ant) || (command_ant_trac != 'B'))
    {
      speed_ant = speed;
      envio_command(command, speed);
      command_ant_trac = command;
    }
  }
  delay(20);
}

void envio_command(char command, int speed)
{
  // ENVÍO COMANDO
  envio[0] = command;
  envio[1] = char(speed + '0');
  Serial.print("Send: ");
  Serial.println(envio);
  radio.write(&envio, sizeof(envio));
}