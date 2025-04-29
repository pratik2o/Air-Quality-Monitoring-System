#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Pin Definitions
#define MQ5_D_PIN 49
#define MQ5_A_PIN A0
#define MQ6_D_PIN 51
#define MQ6_A_PIN A1
#define MQ4_D_PIN 53
#define MQ4_A_PIN A2
#define MQ135_D_PIN 47
#define MQ135_A_PIN A3
#define MQ7_D_PIN 45
#define MQ7_A_PIN A4
#define DHT11_PIN 43
#define GP2Y_D_PIN 41
#define GP2Y_A_PIN A15
#define BUZZER_PIN 52

// Sensor Thresholds
#define GAS_THRESHOLD 300 // Change this based on calibration
#define DUST_THRESHOLD 80  // Adjust based on calibration

// LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4);

// DHT sensor setup
DHT dht(DHT11_PIN, DHT11);

// Variables for sensor readings
int mq5_analog, mq6_analog, mq4_analog, mq135_analog, mq7_analog, dust_level;
float humidity, temperature;
int mq5_digital, mq6_digital, mq4_digital, mq135_digital, mq7_digital, dust_digital;

// Calibration parameters for dust sensor
#define DUST_ZERO_POINT 10  // Clean air analog reading for dust sensor
#define DUST_A 2.22         // Replace with your slope from calibration
#define DUST_B 0            // Intercept

// Calibration parameters for MQ4 sensor
#define MQ4_ZERO_POINT 133  // Clean air analog reading for MQ4 sensor
#define MQ4_A 0.374         // Replace with your slope from calibration
#define MQ4_B 0             // Intercept

// Calibration parameters for MQ5 sensor
#define MQ5_ZERO_POINT 52   // Clean air analog reading for MQ5 sensor
#define MQ5_A 0.505         // Replace with your slope from calibration
#define MQ5_B 0             // Intercept

// Calibration parameters for MQ7 sensor
#define MQ7_ZERO_POINT 41   // Clean air analog reading for MQ7 sensor
#define MQ7_A 1.69          // Replace with your slope from calibration
#define MQ7_B 0             // Intercept

// AQI breakpoints for CO (in µg/m³)
const int CO_AQI_BREAKPOINTS[6] = {0, 500, 1000, 1500, 2000, 2500};
const int CO_AQI_VALUES[6] = {0, 50, 100, 150, 200, 300};

// AQI breakpoints for PM (in µg/m³)
const int PM_AQI_BREAKPOINTS[6] = {0, 50, 100, 150, 200, 300};
const int PM_AQI_VALUES[6] = {0, 50, 100, 150, 200, 300};

void setup() {
  // LCD initialization
  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight

  // Sensor pins setup
  pinMode(MQ5_D_PIN, INPUT);
  pinMode(MQ6_D_PIN, INPUT);
  pinMode(MQ4_D_PIN, INPUT);
  pinMode(MQ135_D_PIN, INPUT);
  pinMode(MQ7_D_PIN, INPUT);
  pinMode(GP2Y_D_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dht.begin();

  lcd.setCursor(0, 0);
  lcd.print("Air Quality Sys");
  delay(2000);
  lcd.clear();
}

float getDustConcentration(int rawValue) {
    float concentration = (DUST_A * (rawValue - DUST_ZERO_POINT) + DUST_B); // Calculate dust concentration
    return (concentration < 0) ? 0 : concentration; // Clamp to zero if negative
}

float getMQ4Concentration(int rawValue) {
    float concentration = (MQ4_A * (rawValue - MQ4_ZERO_POINT) + MQ4_B); // Calculate MQ4 concentration
    return (concentration < 0) ? 0 : concentration; // Clamp to zero if negative
}

float getMQ5Concentration(int rawValue) {
    float concentration = (MQ5_A * (rawValue - MQ5_ZERO_POINT) + MQ5_B); // Calculate MQ5 concentration
    return (concentration < 0) ? 0 : concentration; // Clamp to zero if negative
}

float getMQ7Concentration(int rawValue) {
    float concentration = (MQ7_A * (rawValue - MQ7_ZERO_POINT) + MQ7_B); // Calculate MQ7 concentration
    return (concentration < 0) ? 0 : concentration; // Clamp to zero if negative
}

int calculateAQI(float concentration, const int *breakpoints, const int *aqiValues, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (concentration >= breakpoints[i] && concentration < breakpoints[i + 1]) {
            // Linear interpolation
            return map(concentration, breakpoints[i], breakpoints[i + 1], aqiValues[i], aqiValues[i + 1]);
        }
    }
    return 0; // If out of range
}

void loop() {
    // Read analog values from gas sensors
    mq5_analog = analogRead(MQ5_A_PIN);
    mq6_analog = analogRead(MQ6_A_PIN);
    mq4_analog = analogRead(MQ4_A_PIN);
    mq135_analog = analogRead(MQ135_A_PIN);
    mq7_analog = analogRead(MQ7_A_PIN);
    dust_level = analogRead(GP2Y_A_PIN);

    // Read digital values from gas sensors
    mq5_digital = digitalRead(MQ5_D_PIN);
    mq6_digital = digitalRead(MQ6_D_PIN);
    mq4_digital = digitalRead(MQ4_D_PIN);
    mq135_digital = digitalRead(MQ135_D_PIN);
    mq7_digital = digitalRead(MQ7_D_PIN);
    dust_digital = digitalRead(GP2Y_D_PIN);

    // Read DHT11 for temperature and humidity
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    // Error handling for DHT11 readings
    if (isnan(humidity) || isnan(temperature)) {
        lcd.setCursor(0, 0);
        lcd.print("DHT11 Error     ");
    } else {
        lcd.setCursor(0, 0);
        lcd.print("Temp:");
        lcd.print(temperature);
        lcd.print("C Hum:");
        lcd.print(humidity);
        lcd.print("%");
    }

    // Convert sensor levels to concentration
    float dustConcentration = getDustConcentration(dust_level);
    float mq4Concentration = getMQ4Concentration(mq4_analog);
    float mq5Concentration = getMQ5Concentration(mq5_analog);
    float mq7Concentration = getMQ7Concentration(mq7_analog);

    // Calculate AQI for CO and Dust
    int aqi_CO = calculateAQI(mq7Concentration, CO_AQI_BREAKPOINTS, CO_AQI_VALUES, 6);
    int aqi_PM = calculateAQI(dustConcentration, PM_AQI_BREAKPOINTS, PM_AQI_VALUES, 6);
    
    // Final AQI based on the highest value
    int overallAQI = max(aqi_CO, aqi_PM);

    // Display data on the LCD
    lcd.setCursor(0, 1);
    lcd.print("CO2:");
    lcd.print(mq135_analog);
    lcd.print(" CO:");
    lcd.print(mq7Concentration); // Display MQ7 concentration

    lcd.setCursor(0, 2);
    lcd.print("LPG:");
    lcd.print(mq5Concentration); // Display MQ5 concentration
    lcd.print(" Met:");
    lcd.print(mq4Concentration); // Display MQ4 concentration

    lcd.setCursor(0, 3);
    lcd.print("AQI:");
    lcd.print(overallAQI); // Display AQI

    // Gas alert and buzzer control
    if (mq5_analog > GAS_THRESHOLD || mq6_analog > GAS_THRESHOLD || mq4_analog > GAS_THRESHOLD ||
        mq135_analog > GAS_THRESHOLD || mq7_analog > GAS_THRESHOLD || dustConcentration > DUST_THRESHOLD) {
        digitalWrite(BUZZER_PIN, HIGH); // Activate buzzer
        lcd.setCursor(0, 3);
        lcd.print("!!ALERT!!       ");
    } else {
        digitalWrite(BUZZER_PIN, LOW); // Deactivate buzzer
    }

    delay(1000); // Wait for 1 second before next reading
}
