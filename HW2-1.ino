const int RledPin = 9;
const int GledPin = 10;
const int BledPin = 11;
const int buttonPin = 2;

const bool COMMON_ANODE = true;

int mood = 10;
const int neutralMood = 10;

bool buttonPressed = false;
unsigned long pressStart = 0;
bool hugged = false;

unsigned long touchedTimer = 0;
unsigned long reducedTimer = 0;
const long unTouchInterval = 5000;
const long reducedInterval = 1000;

unsigned long spamWindowStart = 0;
int spamCount = 0;
const int spamLimit = 8;
const long spamWindow = 2000;

enum Effect { NONE, RAINBOW, ANNOYED, HUG };
Effect effect = NONE;
unsigned long effectEnd = 0;
bool celebrated = false;

void setRGB(int r, int g, int b) {
  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);
  if (COMMON_ANODE) { r = 255 - r; g = 255 - g; b = 255 - b; }
  analogWrite(RledPin, r);
  analogWrite(GledPin, g);
  analogWrite(BledPin, b);
}

void moodColor(int m, int &r, int &g, int &b) {
  if (m >= neutralMood) {
    float t = (m - neutralMood) / 10.0;
    r = 0; g = 255 * (1 - t); b = 255 * t;
  } else {
    float t = (neutralMood - m) / 10.0;
    r = 255 * t; g = 255 * (1 - t); b = 0;
  }
}

void wheel(byte pos, int &r, int &g, int &b) {
  if (pos < 85)       { r = 255 - pos * 3; g = pos * 3; b = 0; }
  else if (pos < 170) { pos -= 85;  r = 0; g = 255 - pos * 3; b = pos * 3; }
  else                { pos -= 170; r = pos * 3; g = 0; b = 255 - pos * 3; }
}

void startEffect(Effect e, unsigned long duration) {
  effect = e;
  effectEnd = millis() + duration;
}

void showLED() {
  unsigned long now = millis();
  int r, g, b;

  if (effect != NONE && now >= effectEnd) effect = NONE;

  if (effect == RAINBOW) {
    wheel((now / 4) % 256, r, g, b);
    setRGB(r, g, b);
    return;
  }
  if (effect == ANNOYED) {
    if ((now / 100) % 2 == 0) setRGB(255, 0, 0);
    else setRGB(0, 0, 0);
    return;
  }
  if (effect == HUG) {
    setRGB(255, 60, 120);
    return;
  }

  moodColor(mood, r, g, b);
  long period = map(mood, 0, 20, 4000, 1000);
  float phase = (now % period) / (float)period * 2 * PI;
  float minLevel = map(mood, 0, 20, 10, 30) / 100.0;
  float level = minLevel + (1.0 - minLevel) * (sin(phase) + 1) / 2;
  setRGB(r * level, g * level, b * level);
}

void addMood(int delta) {
  mood = constrain(mood + delta, 0, 20);
  if (mood < 18) celebrated = false;
  if (mood == 20 && !celebrated) {
    celebrated = true;
    startEffect(RAINBOW, 2000);
  }
  Serial.print("mood = "); Serial.println(mood);
}

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(RledPin, OUTPUT);
  pinMode(GledPin, OUTPUT);
  pinMode(BledPin, OUTPUT);
  touchedTimer = reducedTimer = millis();
}

void loop() {
  unsigned long now = millis();
  showLED();

  int state = digitalRead(buttonPin);

  if (state == HIGH && !buttonPressed) {
    buttonPressed = true;
    pressStart = now;
    hugged = false;
    touchedTimer = now;

    if (now - spamWindowStart > spamWindow) { spamWindowStart = now; spamCount = 0; }
    spamCount++;

    if (spamCount >= spamLimit) {
      addMood(-2);
      startEffect(ANNOYED, 1500);
      spamCount = 0;
    } else {
      addMood(1);
    }
  }

  if (state == HIGH && buttonPressed && !hugged && now - pressStart > 800) {
    hugged = true;
    addMood(3);
    touchedTimer = now;
    if (effect != RAINBOW) startEffect(HUG, 1500);
  }

  if (state == LOW && buttonPressed) buttonPressed = false;

  if (now - touchedTimer > unTouchInterval && now - reducedTimer > reducedInterval) {
    mood = max(mood - 1, 0);
    if (mood < 18) celebrated = false;
    reducedTimer = now;
    Serial.print("mood = "); Serial.println(mood);
  }
}