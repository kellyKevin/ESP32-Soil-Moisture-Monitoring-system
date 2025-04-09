#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>  // Include TimeLib for time functions

#define MOISTURE_SENSOR_PIN 34  // Moisture sensor input pin
#define WEATHER_API_KEY "fb5a9e821b328eb5cd73f9e671a2dda1"  // Replace with OpenWeather API key
#define WIFI_SSID "Blue_sky"
#define WIFI_PASSWORD "kelly_bluesky"

WebServer server(80);
BluetoothSerial SerialBT;

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize LCD with I2C address, columns, and rows

String city = "Nairobi";  // Default city

unsigned long previousMillis = 0;
const long interval = 15000;  // Increased update interval by 5 seconds (from 10s to 15s)
const int maxDataPoints = 10; // Store last 10 readings for the trend graph

int moistureData[maxDataPoints];  // Array to store moisture data for graph
int currentIndex = 0;  // Index to keep track of the latest data point

// Function to initialize the moisture data array
void initMoistureData() {
  int initialMoisture = getSoilMoisture();
  for (int i = 0; i < maxDataPoints; i++) {
    moistureData[i] = initialMoisture;
  }
}

// Function to update moisture data array with new reading
void updateMoistureData(int newReading) {
  moistureData[currentIndex] = newReading;
  currentIndex = (currentIndex + 1) % maxDataPoints;  // Circular buffer
}

int getAveragedMoisture() {
  int total = 0;
  for (int i = 0; i < maxDataPoints; i++) {
    total += moistureData[i];
  }
  return total / maxDataPoints;
}

// Function to get soil moisture reading
int getSoilMoisture() {
  int moistureValue = analogRead(MOISTURE_SENSOR_PIN);
  // Calibrated based on actual readings:
  // ~2610 = dry air (0%)
  // ~1110 = fully moistened soil (100%)
  moistureValue = map(moistureValue, 2610, 1110, 0, 100);
  // Constrain to ensure values stay within 0-100% range
  moistureValue = constrain(moistureValue, 0, 100);
  return moistureValue;
}

// Function to fetch weather data from OpenWeather API
String getWeatherData() {
  String weatherInfo = "Unknown";
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + WEATHER_API_KEY + "&units=metric";
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      float temp = doc["main"]["temp"];
      String condition = doc["weather"][0]["main"];
      weatherInfo = String(temp) + "°C, " + condition;
    }
    http.end();
  }
  return weatherInfo;
}

// Function to determine irrigation advice
String getIrrigationAdvice(int moisture, String weather) {
  if (moisture > 70) return "Soil is wet. No watering needed!";
  if (weather.indexOf("Rain") >= 0) return "Rain expected. No watering needed!";
  if (moisture < 30) return "Soil is dry. Water the crops!";
  return "Monitor soil moisture and adjust watering accordingly.";
}

// Function to get the current time (make sure time is synchronized)
String getLastUpdateTime() {
  return String(hour()) + ":" + String(minute()) + ":" + String(second());
}

// Function to generate graph data string from stored moisture values
String getGraphData() {
  String graphData = "";
  for (int i = 0; i < maxDataPoints; i++) {
    graphData += String(moistureData[i]);
    if (i < maxDataPoints - 1) {
      graphData += ",";
    }
  }
  return graphData;
}

// Function to generate a more professional and creative web page
String generateWebPage(String moisture, String weather, String advice, String lastUpdate) {
  String html = "";
  
  // HTML HEAD section
  html += "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  
  // CSS Styles
  html += "<style>";
  html += ":root {";
  html += "  --primary: #2e7d32;";
  html += "  --primary-light: #60ad5e;";
  html += "  --primary-dark: #005005;";
  html += "  --text-on-primary: #ffffff;";
  html += "  --background: #f5f5f5;";
  html += "  --card: #ffffff;";
  html += "  --text: #333333;";
  html += "  --border-radius: 12px;";
  html += "  --shadow: 0 4px 6px rgba(0,0,0,0.1);";
  html += "  --transition: all 0.3s ease;";
  html += "}";
  
  html += "body {";
  html += "  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;";
  html += "  background: var(--background);";
  html += "  color: var(--text);";
  html += "  margin: 0;";
  html += "  padding: 0;";
  html += "  transition: var(--transition);";
  html += "}";
  
  html += ".container {";
  html += "  padding: 24px;";
  html += "  max-width: 800px;";
  html += "  margin: 20px auto;";
  html += "  background: var(--card);";
  html += "  border-radius: var(--border-radius);";
  html += "  box-shadow: var(--shadow);";
  html += "}";
  
  html += ".header {";
  html += "  display: flex;";
  html += "  align-items: center;";
  html += "  justify-content: space-between;";
  html += "  margin-bottom: 24px;";
  html += "  border-bottom: 2px solid var(--primary-light);";
  html += "  padding-bottom: 16px;";
  html += "}";
  
  html += ".header img {";
  html += "  width: 60px;";
  html += "  height: auto;";
  html += "}";
  
  html += ".dashboard {";
  html += "  display: grid;";
  html += "  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));";
  html += "  gap: 16px;";
  html += "  margin-bottom: 24px;";
  html += "}";
  
  html += ".card {";
  html += "  padding: 20px;";
  html += "  background: linear-gradient(145deg, var(--card), #f0f0f0);";
  html += "  border-left: 4px solid var(--primary);";
  html += "  border-radius: var(--border-radius);";
  html += "  box-shadow: var(--shadow);";
  html += "  transition: var(--transition);";
  html += "}";
  
  html += ".card:hover {";
  html += "  transform: translateY(-5px);";
  html += "  box-shadow: 0 10px 15px rgba(0,0,0,0.1);";
  html += "}";
  
  html += ".card h3 {";
  html += "  margin-top: 0;";
  html += "  color: var(--primary-dark);";
  html += "  font-size: 1.2rem;";
  html += "}";
  
  html += ".card p {";
  html += "  font-size: 1.5rem;";
  html += "  font-weight: bold;";
  html += "  margin-bottom: 0;";
  html += "}";
  
  html += ".card .subtitle {";
  html += "  font-size: 0.9rem;";
  html += "  color: #666;";
  html += "  margin-top: 5px;";
  html += "}";
  
  html += ".chart-container {";
  html += "  background: var(--card);";
  html += "  padding: 20px;";
  html += "  border-radius: var(--border-radius);";
  html += "  box-shadow: var(--shadow);";
  html += "  margin-bottom: 24px;";
  html += "}";
  
  html += ".form-container {";
  html += "  background: var(--card);";
  html += "  padding: 20px;";
  html += "  border-radius: var(--border-radius);";
  html += "  box-shadow: var(--shadow);";
  html += "  margin-bottom: 24px;";
  html += "}";
  
  html += "input[type='text'] {";
  html += "  width: calc(100% - 24px);";
  html += "  padding: 12px;";
  html += "  margin-bottom: 16px;";
  html += "  font-size: 16px;";
  html += "  border-radius: 8px;";
  html += "  border: 1px solid #ddd;";
  html += "  transition: var(--transition);";
  html += "}";
  
  html += "input[type='text']:focus {";
  html += "  border-color: var(--primary);";
  html += "  outline: none;";
  html += "  box-shadow: 0 0 0 2px rgba(46, 125, 50, 0.2);";
  html += "}";
  
  html += "button, input[type='submit'] {";
  html += "  background: var(--primary);";
  html += "  color: var(--text-on-primary);";
  html += "  border: none;";
  html += "  padding: 12px 24px;";
  html += "  font-size: 16px;";
  html += "  border-radius: 8px;";
  html += "  cursor: pointer;";
  html += "  transition: var(--transition);";
  html += "  width: 100%;";
  html += "}";
  
  html += "button:hover, input[type='submit']:hover {";
  html += "  background: var(--primary-dark);";
  html += "}";
  
  html += ".theme-toggle {";
  html += "  position: fixed;";
  html += "  bottom: 20px;";
  html += "  right: 20px;";
  html += "  background: var(--primary);";
  html += "  color: var(--text-on-primary);";
  html += "  border: none;";
  html += "  width: 50px;";
  html += "  height: 50px;";
  html += "  border-radius: 50%;";
  html += "  display: flex;";
  html += "  align-items: center;";
  html += "  justify-content: center;";
  html += "  cursor: pointer;";
  html += "  box-shadow: var(--shadow);";
  html += "  z-index: 100;";
  html += "}";
  
  html += ".status-indicator {";
  html += "  display: inline-block;";
  html += "  width: 12px;";
  html += "  height: 12px;";
  html += "  border-radius: 50%;";
  html += "  margin-right: 8px;";
  html += "}";
  
  html += ".status-good { background-color: #4caf50; }";
  html += ".status-warning { background-color: #ff9800; }";
  html += ".status-alert { background-color: #f44336; }";
  
  html += ".dark-mode {";
  html += "  --primary: #81c784;";
  html += "  --primary-light: #b2fab4;";
  html += "  --primary-dark: #519657;";
  html += "  --text-on-primary: #000000;";
  html += "  --background: #121212;";
  html += "  --card: #1e1e1e;";
  html += "  --text: #e0e0e0;";
  html += "}";
  
  html += ".footer {";
  html += "  text-align: center;";
  html += "  margin-top: 20px;";
  html += "  font-size: 0.9rem;";
  html += "  color: #666;";
  html += "}";
  
  html += "@media (max-width: 600px) {";
  html += "  .dashboard { grid-template-columns: 1fr; }";
  html += "  .header { flex-direction: column; text-align: center; }";
  html += "}";
  html += "</style>";
  html += "</head>";
  
  // HTML BODY section
  html += "<body>";
  html += "<div class='container'>";
  
  // Header
  html += "<div class='header'>";
  html += "<div>";
  html += "<h1>Smart Irrigation System</h1>";
  html += "<p>Real-time monitoring and intelligent watering recommendations</p>";
  html += "</div>";
  
  // Weather icon
  String weatherIcon = (weather.indexOf("Rain") >= 0) ? 
    "https://cdn-icons-png.flaticon.com/512/414/414927.png" : 
    "https://cdn-icons-png.flaticon.com/512/869/869869.png";
  html += "<img src='" + weatherIcon + "' alt='Weather Icon'>";
  html += "</div>";
  
  // Dashboard cards
  html += "<div class='dashboard'>";
  
  // Moisture card
  html += "<div class='card'>";
  html += "<h3>Soil Moisture</h3>";
  html += "<p>" + moisture + "%</p>";
  html += "<div class='subtitle'>";
  
  String moistureStatus = "";
  String moistureClass = "";
  if (moisture.toInt() < 30) {
    moistureStatus = "Dry";
    moistureClass = "status-alert";
  } else if (moisture.toInt() < 60) {
    moistureStatus = "Moderate";
    moistureClass = "status-warning";
  } else {
    moistureStatus = "Optimal";
    moistureClass = "status-good";
  }
  
  html += "<span class='status-indicator " + moistureClass + "'></span>";
  html += moistureStatus;
  html += "</div></div>";
  
  // Weather card
  html += "<div class='card'>";
  html += "<h3>Weather</h3>";
  html += "<p>" + weather + "</p>";
  html += "<div class='subtitle'>Current conditions in " + city + "</div>";
  html += "</div>";
  
  // Recommendation card
  html += "<div class='card'>";
  html += "<h3>Recommendation</h3>";
  html += "<p style='font-size: 1.2rem;'>" + advice + "</p>";
  html += "<div class='subtitle'>Based on soil and weather conditions</div>";
  html += "</div>";
  
  // Last update card
  html += "<div class='card'>";
  html += "<h3>Last Update</h3>";
  html += "<p>" + lastUpdate + "</p>";
  html += "<div class='subtitle'>Auto-refreshes every 15 seconds</div>";
  html += "</div>";
  html += "</div>";
  
  // Chart container
  html += "<div class='chart-container'>";
  html += "<h3>Soil Moisture Trend</h3>";
  html += "<canvas id='moistureChart'></canvas>";
  html += "</div>";
  
  // Form container
  html += "<div class='form-container'>";
  html += "<h3>Settings</h3>";
  html += "<form action='/setCity' method='POST'>";
  html += "<input type='text' name='city' placeholder='Enter City Name' value='" + city + "'>";
  html += "<input type='submit' value='Update Weather Location'>";
  html += "</form>";
  html += "</div>";
  
  // Footer
  html += "<div class='footer'>";
  html += "Smart Irrigation System v1.0 | ESP32 IoT Project";
  html += "</div>";
  html += "</div>";
  
  // Theme toggle button
  html += "<button class='theme-toggle' onclick='toggleTheme()' title='Toggle Dark/Light Mode'>";
  html += "<svg width='24' height='24' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'>";
  html += "<path d='M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z'></path>";
  html += "</svg>";
  html += "</button>";
  
  // JavaScript
  html += "<script>";
  
  // Chart.js initialization
  html += "const ctx = document.getElementById('moistureChart').getContext('2d');";
  html += "const chart = new Chart(ctx, {";
  html += "  type: 'line',";
  html += "  data: {";
  html += "    labels: ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10'],";
  html += "    datasets: [{";
  html += "      label: 'Soil Moisture (%)',";
  html += "      data: [" + getGraphData() + "],";
  html += "      fill: true,";
  html += "      backgroundColor: 'rgba(75, 192, 192, 0.2)',";
  html += "      borderColor: 'rgba(75, 192, 192, 1)',";
  html += "      borderWidth: 2,";
  html += "      tension: 0.4,";
  html += "      pointBackgroundColor: 'rgba(75, 192, 192, 1)',";
  html += "      pointRadius: 4";
  html += "    }]";
  html += "  },";
  html += "  options: {";
  html += "    responsive: true,";
  html += "    plugins: {";
  html += "      legend: { position: 'top' },";
  html += "      tooltip: { mode: 'index', intersect: false }";
  html += "    },";
  html += "    scales: {";
  html += "      y: {";
  html += "        beginAtZero: true,";
  html += "        max: 100,";
  html += "        ticks: { callback: function(value) { return value + '%'; } }";
  html += "      }";
  html += "    }";
  html += "  }";
  html += "});";
  
  // Theme toggle function
  html += "function toggleTheme() {";
  html += "  document.body.classList.toggle('dark-mode');";
  html += "  localStorage.setItem('theme', document.body.classList.contains('dark-mode') ? 'dark' : 'light');";
  html += "  const isDark = document.body.classList.contains('dark-mode');";
  html += "  chart.data.datasets[0].borderColor = isDark ? 'rgba(129, 199, 132, 1)' : 'rgba(75, 192, 192, 1)';";
  html += "  chart.data.datasets[0].backgroundColor = isDark ? 'rgba(129, 199, 132, 0.2)' : 'rgba(75, 192, 192, 0.2)';";
  html += "  chart.data.datasets[0].pointBackgroundColor = isDark ? 'rgba(129, 199, 132, 1)' : 'rgba(75, 192, 192, 1)';";
  html += "  chart.update();";
  html += "}";
  
  // Apply saved theme on page load
  html += "window.onload = function() {";
  html += "  if (localStorage.getItem('theme') === 'dark') {";
  html += "    document.body.classList.add('dark-mode');";
  html += "    chart.data.datasets[0].borderColor = 'rgba(129, 199, 132, 1)';";
  html += "    chart.data.datasets[0].backgroundColor = 'rgba(129, 199, 132, 0.2)';";
  html += "    chart.data.datasets[0].pointBackgroundColor = 'rgba(129, 199, 132, 1)';";
  html += "    chart.update();";
  html += "  }";
  html += "  setTimeout(() => location.reload(), 15000);";
  html += "};";
  html += "</script>";
  html += "</body></html>";
  
  return html;
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Irrigation");
  lcd.init();
  lcd.backlight();
  
  // Initialize the moisture data array
  initMoistureData();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Display connecting message on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    delay(2000);
  } else {
    Serial.println("WiFi connection failed");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");
    lcd.setCursor(0, 1);
    lcd.print("Check Settings");
  }

  server.on("/", HTTP_GET, []() {
    int currentMoisture = getSoilMoisture();
    updateMoistureData(currentMoisture);
    
    String moisture = String(getAveragedMoisture());
    String weather = getWeatherData();
    String advice = getIrrigationAdvice(moisture.toInt(), weather);
    String lastUpdate = getLastUpdateTime();
    String page = generateWebPage(moisture, weather, advice, lastUpdate);
    server.send(200, "text/html", page);
  });

  server.on("/setCity", HTTP_POST, []() {
    if (server.hasArg("city")) {
      city = server.arg("city");
    }
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // Periodically update LCD and Bluetooth
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Get current moisture reading and update the data array
    int currentMoisture = getSoilMoisture();
    updateMoistureData(currentMoisture);
    
    // Get updated data
    String moisture = String(getAveragedMoisture());
    String weather = getWeatherData();
    String advice = getIrrigationAdvice(moisture.toInt(), weather);
    String lastUpdate = getLastUpdateTime();

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Moisture: " + moisture + "%");
    lcd.setCursor(0, 1);
    lcd.print(advice.substring(0, 16)); // Show first 16 chars of advice
    
    // Send updates to Bluetooth
    SerialBT.println("Moisture: " + moisture + "%");
    SerialBT.println("Weather: " + weather);
    SerialBT.println("Advice: " + advice);
    SerialBT.println("Last Update: " + lastUpdate);
    
    // Also send to serial for debugging
    Serial.println("Raw Sensor Value: " + String(analogRead(MOISTURE_SENSOR_PIN)));
    Serial.println("Moisture: " + moisture + "%");
    Serial.println("Weather: " + weather);
  }
}