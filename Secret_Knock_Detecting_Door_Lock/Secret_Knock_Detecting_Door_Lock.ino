#include <Servo.h>

// Pin definitions
const int knockSensor = 1;         // Piezo sensor on pin 1.
Servo knockServo;
const int greenLED = 7;

// Tuning constants. 
const int threshold = 10;           // Minimum signal from the piezo to register as a knock
const int rejectValue = 30;        // If an individual knock is off by this percentage of a knock we don't unlock..
const int averageRejectValue = 20; // If the average timing of the knocks is off by this percent we don't unlock.
const int knockFadeTime = 150;     // milliseconds we allow a knock to fade before we listen for another one. 

const int maximumKnocks = 11;       // Maximum number of knocks to listen for.
const int knockComplete = 3000;     // Longest time to wait for a knock before we assume that it's finished.


// Variables.
int secretCode[maximumKnocks] = {100, 100, 55,55,100,100,100,50,40,50,30}; // Initial setup
int knockReadings[maximumKnocks];   // When someone knocks this array fills with delays between knocks.
int knockSensorValue = 0;           // Last reading of the knock sensor.
int lockState = false;              

void setup() {
  knockServo.attach(5);
  knockServo.write(70);
  pinMode(greenLED, OUTPUT);
  Serial.begin(9600);                     
  Serial.println("Program start.");       
}

void loop() {
  // Listen for any knock at all.
  knockSensorValue = analogRead(knockSensor);
  Serial.println(knockSensorValue);

  if (knockSensorValue >= threshold) {
    listenToSecretKnock();
  }
}

// Records the timing of knocks.
void listenToSecretKnock() {
  Serial.println("knock starting");

  int i = 0;
  // First lets reset the listening array.
  for (i = 0; i < maximumKnocks; i++) {
    knockReadings[i] = 0;
  }

  int currentKnockNumber = 0;             // Incrementer for the array.
  int startTime = millis();               // Reference for when this knock started.
  int now = millis();

  delay(knockFadeTime);                                 // wait for this peak to fade before we listen to the next one.

  do {
    //listen for the next knock or wait for it to timeout.
    knockSensorValue = analogRead(knockSensor);

    if (knockSensorValue >= threshold) {                 //got another knock...
      //record the delay time.
      Serial.println("knock.");
      digitalWrite(greenLED, HIGH);
      now = millis();
      knockReadings[currentKnockNumber] = now - startTime;
      currentKnockNumber ++;                             //increment the counter
      startTime = now;

      delay(knockFadeTime);                              // again, a little delay to let the knock decay.
      digitalWrite(greenLED, LOW);
    }

    now = millis();

    //did we timeout or run out of knocks?
  } while ((now - startTime < knockComplete) && (currentKnockNumber < maximumKnocks));

  //lets see if it's valid
  if (validateKnock() == true && lockState == false) {
    triggerDoorLock();
    lockState =! lockState;
  } else if (validateKnock() == true && lockState == true) {
    triggerDoorUnlock();
    lockState =! lockState;
  } else {
    Serial.println("Secret knock failed.");
    delay(1000);
  }
}

// Runs the servo motor to unlock the door.
void triggerDoorUnlock() {
  Serial.println("Door unlocked!");
   knockServo.write(70);
  delay (1000);                    // Wait a bit.
}

// Runs the servo motor to lock the door.
void triggerDoorLock() {
  Serial.println("Door locked!");
   knockServo.write(0);
  delay (1000);                    // Wait a bit.
}


// Sees if our knock matches the secret.
// returns true if it's a good knock, false if it's not.
boolean validateKnock() {
  int i = 0;

  // simplest check first: Did we get the right number of knocks?
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;               // We use this later to normalize the times.

  for (i = 0; i < maximumKnocks; i++) {
    if (knockReadings[i] > 0) {
      currentKnockCount++;
    }
    if (secretCode[i] > 0) {          
      secretKnockCount++;
    }

    if (knockReadings[i] > maxKnockInterval) {  // collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];

    }

  }

  if (currentKnockCount != secretKnockCount) {
    return false;
  }

  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (if you do the same pattern slow or fast it should still open the door.)   
  */
  int totaltimeDifferences = 0;
  int timeDiff = 0;
  for (i = 0; i < maximumKnocks; i++) { // Normalize the times
    knockReadings[i] = map(knockReadings[i], 0, maxKnockInterval, 0, 100);
    timeDiff = abs(knockReadings[i] - secretCode[i]);

    if (timeDiff > rejectValue) { // Individual value too far out of whack
      return false;
    }
    totaltimeDifferences += timeDiff;
  }

  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences / secretKnockCount > averageRejectValue) {
    return false;
  }
  return true;

}
