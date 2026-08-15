#include <ultrasonic.h> 

#define PIEZO_TRANSMITTER 9
#define PIEZO_RECEIVER A0
#define ANALYZER_MILK_LEVEL 3
#define VALVE0 5
#define VALVE1 6

long beep_start; // variable to store the instant the sound analysis started

static bool analyzer_milk_level = false;
static bool analysis_completed = false;
static bool sampling_active = false;

float amplitude_sum = 0;
long sample_count = 0;

float lr; // variable to store lactometer reading
float Aa; // variable to store the amplitude of received sound wave
float fat; // variable to store fat value
float snf; // variable to store snf value

volatile bool flow = false;

void setup() {
  
  delay(1000);
  Serial.begin(9600); // setting baud rate to communicate with ESP32

  setup_ultrasonic(); // declaring pins for ultrasonic sensor

  //pin declarations
  pinMode(PIEZO_TRANSMITTER, OUTPUT);

  pinMode(ANALYZER_MILK_LEVEL, INPUT);
  
  pinMode(VALVE0, OUTPUT);
  pinMode(VALVE1, OUTPUT);

  // setting the initial state of valves
  digitalWrite(VALVE0, HIGH);
  digitalWrite(VALVE1, LOW);

}

void loop() {

  if (digitalRead(ANALYZER_MILK_LEVEL) == HIGH && !analyzer_milk_level) { // once the sample tube is filled , the valve1 will be closed which stops the milk flowing into the sample tube

    delay(1500);
    digitalWrite(VALVE1,HIGH);

    analyzer_milk_level = true;
    sampling_active = true;
    analysis_completed = false;

    beep_start = millis(); // storing the instant fat analysis starts

    amplitude_sum = 0;
    sample_count = 0;

    tone(PIEZO_TRANSMITTER, 3000); // producing square pulse of 3KHz to agitate the piezo transducer
  }

  else if (digitalRead(ANALYZER_MILK_LEVEL) == LOW) {

    analyzer_milk_level = false;
  }

  if (sampling_active) {

    if (millis() - beep_start <= 10000) { // analysing for 10 seconds

      amplitude_sum += analogRead(PIEZO_RECEIVER); // the piezo transducer in the other end will receive the sound and generate signals
      sample_count++; // counting the no of samples taken in 10 seconds
    }

    else {

      noTone(PIEZO_TRANSMITTER); // stopping the square wave pulse

      sampling_active = false;

      if (sample_count > 0) {

      // milk parameters calculation

        float avg_amplitude = amplitude_sum / sample_count;

        float distance = read_ultrasonic_sensor(); // reading how much the lactometer is displaced upward

        lr = -10.526 * distance + 80;

        Aa = 1023 - avg_amplitude; // magnitude of attenuation

        fat = (0.1016 * Aa * Aa) + (0.2288 * Aa) + 0.4918;

        snf = lr / 4 + 0.2 * fat + 0.36;

        analysis_completed = true;

      }
    }
  }


  if (analysis_completed) {
  // sending the milk parameters values serially to ESP32
        Serial.print(fat, 2);
        Serial.print(",");
        Serial.println(snf, 2);

    digitalWrite(VALVE0, LOW); // opening the valve0 so the sampled milk can drain into the storage
      delay(15000);

    digitalWrite(VALVE0, HIGH); // closing the valve0


    analysis_completed = false;
  }
}
