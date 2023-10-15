// RECEPTOR NANO

#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//* DEFINICION PINES
#define ENA 6
#define IN1 5
#define IN2 7
#define IN3 2
#define IN4 4
#define ENB 3

// Configuracion RF
RF24 radio(9, 10);          // CE, CSN
uint8_t canalDeseado = 125; // Cambia esto al canal deseado (0-125)
const byte address[6] = "00001";

// Movimiento
char text[3];
char text_ant;
bool forward = true;
bool left = false;
bool stop = true;
bool giro = false;
int speed;

// Emergencia perdida señal
bool conexion_state(void);
unsigned long timeSinceNotice = 0, timeLastNotice = 0;

void setup()
{
  Serial.begin(9600);

  // RF
  radio.begin();
  radio.setChannel(canalDeseado); // Configura el canal
  radio.openReadingPipe(0, address);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  // MOTORES
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  // pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
}

void loop()
{
  // COMPROBACIÓN CONEXION
  if (conexion_state() == false)
  {
    //Serial.println("EMERGENCY");
    stop = true;
    giro = false;
  }

  //* RECEPCIÓN DE COMANDOS RF

  if ((text[0] == 'F') || (text[0] == 'B') || (text[0] == 'S'))
    text_ant = text[0];

  if (radio.available())
  {
    radio.read(&text, sizeof(text));
    speed = text[1] - '0';
    speed = map(speed, 0, 9, 50, 255);
    if (text[0] == 'N')
    {
      timeLastNotice = millis();
      Serial.println("NOTICE");
    }
    else if (text[0] == 'S')
    {
      stop = true;
      Serial.println("STOP");
    }
    else if (text[0] == 'G')
    {
      giro = false;
      Serial.println("NO GIRO");
    }
    else if (text[0] == 'F')
    {
      forward = true;
      stop = false;
      Serial.println("FORWARD");
    }
    else if (text[0] == 'B')
    {
      forward = false;
      stop = false;
      Serial.println("BACKWARD");
    }
    else if (text[0] == 'R')
    {
      left = false;
      giro = true;
      Serial.println("RIGHT");
    }
    else if (text[0] == 'L')
    {
      left = true;
      giro = true;
      Serial.println("LEFT");
    }
  }

  //* ACTUACIÓN EN MOTORES

  //! DIRECCION (MOTOR DELANTERO)
  if (giro) // SE DARA GIRO
  {
    // SENTIDO
    if (left) // IZQUIERDA
    {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    }
    else // DERECHA
    {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    }
    digitalWrite(ENA, HIGH);
  }
  else // NO SE DARA GIRO
  {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(ENA, LOW);
  }

  //! TRACCION (MOTOR TRASERO)
  if (!stop) // ACTIVADO
  {
    // SENTIDO
    if (forward) // AVANCE
    {
      if (text_ant == 'B')
      {
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        delay(200);
      }
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    }
    else // RETROCESO
    {
      if (text_ant == 'F')
      {
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        delay(200);
      }
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
    Serial.println(speed);
    analogWrite(ENB, speed);
  }
  else // PARADO
  {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);
  }
}

bool conexion_state(void)
{
  timeSinceNotice = millis() - timeLastNotice;
  if (timeSinceNotice < 1500)
    return true;
  else
    return false;
}