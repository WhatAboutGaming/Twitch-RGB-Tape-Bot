#include <EEPROM.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <IRCClient.h>

#define IRC_SERVER   "irc.chat.twitch.tv"
#define IRC_PORT     6667

//------- Replace the following! ------
char ssid[] = "SSID";       // your network SSID (name)
char password[] = "KEY";  // your network key

//The name of the channel that you want the bot to join
String twitchChannelName[] = {"channel0", "channel1", "channel2"}; // Array of channels to join
String mainTwitchChannel = "mainchannelname"; // Main Channel Name
String debugTwitchChannel = "debugchannelname"; // Debug Channel Name
String adminNameToPing = "AdminNameToPing"; // Bot Owner (Can be display name or username)
String roomIdToListenTo = "12345678"; // Room ID To Listen To (Only one room, no need to listen for more rooms)
String debugRoomId = "23456789"; // Debug Room ID (ID of the room where test commands are received)
String adminUserId = "34567890"; // Admin User ID (ID of the bot owner)
String customRewardIdToRead = "RewardIDStringAsSentByTwitch"; // Reward ID used to tell when a LED reward has been redeemed

//The name that you want the bot to have
#define TWITCH_BOT_NAME "justinfan123"

//OAuth Key for your twitch bot
// https://twitchapps.com/tmi/
#define TWITCH_OAUTH_TOKEN "oauth:!yourtwitchoauthtokenkeyhere!"


//------------------------------

#define counter1 D1
#define counter2 D3
#define counter3 D7

#define rgbRed D6
#define rgbGreen D5
#define rgbBlue D8

#define resetButton D2
uint32_t counter1StepsToIncrement = 0;
uint32_t counter2StepsToIncrement = 0;
bool counter1State = 0;
bool counter2State = 0;
uint32_t currentMillis = 0;
uint32_t resetButtonDebounceMillis = 500;
uint32_t currentSeconds = 0; // Use this to hopefully keep track of time in seconds since arduino has started, millis() should not affect this even if it overflows
uint32_t currentMinutes = 0;
uint32_t currentHours = 0;
uint32_t currentDays = 0;
uint32_t totalSeconds = 0;
uint32_t totalMinutes = 0;
uint32_t totalHours = 0;
uint32_t totalDays = 0;
uint32_t wifiDisconnectCounter = 0;
uint32_t ircDisconnectCounter = 0;
uint32_t bootCounter = 0;
uint32_t millisToSecondsCurrent = currentMillis / 1000;
uint32_t millisToSecondsPrevious = millisToSecondsCurrent;
uint32_t resetButtonDebounceMillisPrevious = 0;
uint8_t redLevel = 0;
uint8_t greenLevel = 0;
uint8_t blueLevel = 0;
bool resetButtonStateCurrent = 0;
bool resetButtonStatePrevious = 0;
bool waitingForPong = false;
uint32_t pingDelayMillis = 5000;
uint32_t pingDelayMillisPrevious = 0;
uint32_t pongTimeoutMillis = 30000;
uint32_t pongTimeoutMillisPrevious = 0;
uint32_t counterOnTimeMillis = 16;
uint32_t counter1OnTimeMillisPrevious = 0;
uint32_t counter2OnTimeMillisPrevious = 0;
uint32_t counterOffTimeMillis = counterOnTimeMillis * 2;
uint32_t counter1OffTimeMillisPrevious = 0;
uint32_t counter2OffTimeMillisPrevious = 0;
uint32_t pingSentAt = 0;
uint32_t pongReceivedAt = 0;
uint32_t pongResponseDelay = 0;
uint8_t ledTapeMode = 0; // 0 = Get state sent by Twitch Chat and use that in the LED tape, 1 = LED tape analog write 0, 2 = LED tape analog write 255
uint32_t totalMessagesSent = 0;
uint32_t totalLedTapeRewardsRedeemed = 0;
bool incrementIrcDisconnectCounter = false;
uint32_t uptimeRequestedCounter = 0;
uint8_t ledAnimationMode = 0;
uint32_t flashbangDelay = 2000;
uint32_t flashbangTimer = 0;
uint32_t flashbangDecrementDelay = 10;
uint32_t flashbangDecrementTimer = 0;
uint8_t flashbangValue = 255;
uint8_t rainbowValueRed = 254;
uint8_t rainbowValueGreen = 0;
uint8_t rainbowValueBlue = 0;
bool startFlashbangAnimation = true;
uint32_t rainbowStepDelay = 10;
uint32_t rainbowStepTimer = 0;
uint32_t rainbowStepDelaySolid = 1000;
uint32_t rainbowStepTimerSolid = 0;
uint32_t rainbowStepSolidCounter = 0;

WiFiClient wiFiClient;
IRCClient client(IRC_SERVER, IRC_PORT, wiFiClient); // Hostname, Port, network interface (On ESP8266/ESP-12E, the interface is Wifi)

// put your setup code here, to run once:
void setup() {
  pinMode(resetButton, INPUT_PULLUP);

  pinMode(counter1, OUTPUT);
  pinMode(counter2, OUTPUT);
  pinMode(counter3, OUTPUT);
  pinMode(rgbRed, OUTPUT);
  pinMode(rgbGreen, OUTPUT);
  pinMode(rgbBlue, OUTPUT);

  digitalWrite(counter1, LOW);
  digitalWrite(counter2, LOW);
  digitalWrite(counter3, LOW);
  analogWrite(rgbRed, 0);
  analogWrite(rgbGreen, 0);
  analogWrite(rgbBlue, 0);

  EEPROM.begin(512);
  redLevel = EEPROM.read(0);
  greenLevel = EEPROM.read(1);
  blueLevel =  EEPROM.read(2);
  counter1StepsToIncrement = uint32_t(EEPROM.read(7) << 24) | uint32_t(EEPROM.read(6) << 16) | uint32_t(EEPROM.read(5) << 8) | uint32_t(EEPROM.read(4));
  counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
  totalMessagesSent = uint32_t(EEPROM.read(15) << 24) | uint32_t(EEPROM.read(14) << 16) | uint32_t(EEPROM.read(13) << 8) | uint32_t(EEPROM.read(12));
  totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));

  currentSeconds = uint32_t(EEPROM.read(35) << 24) | uint32_t(EEPROM.read(34) << 16) | uint32_t(EEPROM.read(33) << 8) | uint32_t(EEPROM.read(32));
  currentMinutes = uint32_t(EEPROM.read(39) << 24) | uint32_t(EEPROM.read(38) << 16) | uint32_t(EEPROM.read(37) << 8) | uint32_t(EEPROM.read(36));
  currentHours = uint32_t(EEPROM.read(43) << 24) | uint32_t(EEPROM.read(42) << 16) | uint32_t(EEPROM.read(41) << 8) | uint32_t(EEPROM.read(40));
  currentDays = uint32_t(EEPROM.read(47) << 24) | uint32_t(EEPROM.read(46) << 16) | uint32_t(EEPROM.read(45) << 8) | uint32_t(EEPROM.read(44));

  totalSeconds = uint32_t(EEPROM.read(51) << 24) | uint32_t(EEPROM.read(50) << 16) | uint32_t(EEPROM.read(49) << 8) | uint32_t(EEPROM.read(48));
  totalMinutes = uint32_t(EEPROM.read(55) << 24) | uint32_t(EEPROM.read(54) << 16) | uint32_t(EEPROM.read(53) << 8) | uint32_t(EEPROM.read(52));
  totalHours = uint32_t(EEPROM.read(59) << 24) | uint32_t(EEPROM.read(58) << 16) | uint32_t(EEPROM.read(57) << 8) | uint32_t(EEPROM.read(56));
  totalDays = uint32_t(EEPROM.read(63) << 24) | uint32_t(EEPROM.read(62) << 16) | uint32_t(EEPROM.read(61) << 8) | uint32_t(EEPROM.read(60));

  uptimeRequestedCounter = uint32_t(EEPROM.read(67) << 24) | uint32_t(EEPROM.read(66) << 16) | uint32_t(EEPROM.read(65) << 8) | uint32_t(EEPROM.read(64));
  ledTapeMode = EEPROM.read(68);
  ledAnimationMode = EEPROM.read(69);
  // 70 and 71 are reserved
  // use 72 73 74 75 for a 32 bit number

  delay(100);
  bootCounter = uint32_t(EEPROM.read(23) << 24) | uint32_t(EEPROM.read(22) << 16) | uint32_t(EEPROM.read(21) << 8) | uint32_t(EEPROM.read(20));
  wifiDisconnectCounter = uint32_t(EEPROM.read(27) << 24) | uint32_t(EEPROM.read(26) << 16) | uint32_t(EEPROM.read(25) << 8) | uint32_t(EEPROM.read(24));
  ircDisconnectCounter = uint32_t(EEPROM.read(31) << 24) | uint32_t(EEPROM.read(30) << 16) | uint32_t(EEPROM.read(29) << 8) | uint32_t(EEPROM.read(28));
  bootCounter++;
  delay(100);
  EEPROM.write(20, uint8_t((bootCounter) & 0xFF));
  EEPROM.write(21, uint8_t((bootCounter >> 8) & 0xFF));
  EEPROM.write(22, uint8_t((bootCounter >> 16) & 0xFF));
  EEPROM.write(23, uint8_t((bootCounter >> 24) & 0xFF));
  delay(100);
  EEPROM.commit();
  delay(100);
  bootCounter = uint32_t(EEPROM.read(23) << 24) | uint32_t(EEPROM.read(22) << 16) | uint32_t(EEPROM.read(21) << 8) | uint32_t(EEPROM.read(20));
  delay(100);

  digitalWrite(counter1, LOW);
  digitalWrite(counter2, LOW);
  digitalWrite(counter3, LOW);
  if (ledTapeMode == 0) {
    if (ledAnimationMode == 0) {
      analogWrite(rgbRed, redLevel);
      analogWrite(rgbGreen, greenLevel);
      analogWrite(rgbBlue, blueLevel);
    }
    if (ledAnimationMode == 1) {
      analogWrite(rgbRed, 255);
      analogWrite(rgbGreen, 0);
      analogWrite(rgbBlue, 0);
    }
    if (ledAnimationMode == 2) {
      analogWrite(rgbRed, flashbangValue);
      analogWrite(rgbGreen, flashbangValue);
      analogWrite(rgbBlue, flashbangValue);
    }
    if (ledAnimationMode == 3) {
      analogWrite(rgbRed, 255);
      analogWrite(rgbGreen, 0);
      analogWrite(rgbBlue, 0);
    }
  }
  if (ledTapeMode == 1) {
    analogWrite(rgbRed, 0);
    analogWrite(rgbGreen, 0);
    analogWrite(rgbBlue, 0);
  }
  if (ledTapeMode == 2) {
    analogWrite(rgbRed, 255);
    analogWrite(rgbGreen, 255);
    analogWrite(rgbBlue, 255);
  }

  Serial.begin(500000);
  //Serial.println("");

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  //WiFi.mode(WIFI_STA);
  //WiFi.disconnect();
  delay(500);
  Serial.print("bootCounter = ");
  Serial.println(bootCounter);

  Serial.print("Read colors from EEPROM as ");
  Serial.print(redLevel);
  Serial.print(",");
  Serial.print(greenLevel);
  Serial.print(",");
  Serial.print(blueLevel);
  Serial.println("!");

  Serial.println(millis());
  Serial.print(" UPTIME ");
  Serial.print((millis() / 86400000)); // Days
  Serial.print("d ");
  Serial.print((millis() / 3600000) % 24); // Hours
  Serial.print("h ");
  Serial.print((millis() / 60000) % 60); // Minutes
  Serial.print("m ");
  Serial.print((millis() / 1000) % 60); // Seconds
  Serial.print("s ");
  Serial.print((millis() % 1000)); // Milliseconds
  Serial.println("ms ");

  Serial.print(" A ");
  Serial.print(currentDays);
  Serial.print("d ");
  Serial.print(currentHours);
  Serial.print("h ");
  Serial.print(currentMinutes);
  Serial.print("m ");
  Serial.print(currentSeconds);
  Serial.println("s ");

  Serial.print(" B ");
  Serial.print(totalDays);
  Serial.print("d ");
  Serial.print(totalHours);
  Serial.print("h ");
  Serial.print(totalMinutes);
  Serial.print("m ");
  Serial.print(totalSeconds);
  Serial.println("s ");

  Serial.print("ledTapeMode = ");
  Serial.println(ledTapeMode);
  Serial.print("ledAnimationMode = ");
  Serial.println(ledAnimationMode);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.enableInsecureWEP(true);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("ssid = ");
    Serial.print(ssid);
    Serial.print(" , WiFi.status() = ");
    Serial.print(WiFi.status());
    Serial.print(" , WL_CONNECTED = ");
    Serial.println(WL_CONNECTED);
    //Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  client.setCallback(callback);
  /*
    Serial.println("Converting HEX TO INT");
    String testString = "1a90ff";
    uint32_t testInt = strtoul(&testString[0], NULL, 16);
    Serial.println(testInt);
    testString = "#1a90ff";
    testInt = strtoul(&testString[0], NULL, 16); // Works only if starting index is 1 (to remove #), otherwise it returns 0
    Serial.println(testInt);
    testString = "1A90FF";
    testInt = strtoul(&testString[0], NULL, 16);
    Serial.println(testInt);
    testString = "#1A90FF";
    testInt = strtoul(&testString[0], NULL, 16); // Works only if starting index is 1 (to remove #), otherwise it returns 0
    Serial.println(testInt);
  */
}

void loop() {
  //Serial.println(millis());
  currentMillis = millis();
  millisToSecondsCurrent = currentMillis / 1000;
  if (millisToSecondsCurrent != millisToSecondsPrevious) {
    //Serial.println(millisToSecondsCurrent);
    //Serial.println(millisToSecondsPrevious);
    //Serial.print("currentSeconds BEFORE = ");
    //Serial.println(currentSeconds);
    /*
      Serial.println(millis());
      Serial.print(" UPTIME ");
      Serial.print((millis() / 86400000)); // Days
      Serial.print("d ");
      Serial.print((millis() / 3600000) % 24); // Hours
      Serial.print("h ");
      Serial.print((millis() / 60000) % 60); // Minutes
      Serial.print("m ");
      Serial.print((millis() / 1000) % 60); // Seconds
      Serial.print("s ");
      Serial.print((millis() % 1000)); // Milliseconds
      Serial.println("ms ");

      Serial.print(" A ");
      Serial.print(currentDays);
      Serial.print("d ");
      Serial.print(currentHours);
      Serial.print("h ");
      Serial.print(currentMinutes);
      Serial.print("m ");
      Serial.print(currentSeconds);
      Serial.println("s ");

      Serial.print(" B ");
      Serial.print(totalDays);
      Serial.print("d ");
      Serial.print(totalHours);
      Serial.print("h ");
      Serial.print(totalMinutes);
      Serial.print("m ");
      Serial.print(totalSeconds);
      Serial.println("s ");
    */
    if (client.connected()) {
      if (currentSeconds % 5 == 0) {
        sendTwitchPing();
      }
    }
    currentSeconds = uint32_t(EEPROM.read(35) << 24) | uint32_t(EEPROM.read(34) << 16) | uint32_t(EEPROM.read(33) << 8) | uint32_t(EEPROM.read(32));
    currentSeconds++;
    EEPROM.write(32, uint8_t((currentSeconds) & 0xFF));
    EEPROM.write(33, uint8_t((currentSeconds >> 8) & 0xFF));
    EEPROM.write(34, uint8_t((currentSeconds >> 16) & 0xFF));
    EEPROM.write(35, uint8_t((currentSeconds >> 24) & 0xFF));
    currentSeconds = uint32_t(EEPROM.read(35) << 24) | uint32_t(EEPROM.read(34) << 16) | uint32_t(EEPROM.read(33) << 8) | uint32_t(EEPROM.read(32));
    //
    totalSeconds = uint32_t(EEPROM.read(51) << 24) | uint32_t(EEPROM.read(50) << 16) | uint32_t(EEPROM.read(49) << 8) | uint32_t(EEPROM.read(48));
    totalSeconds++;
    EEPROM.write(48, uint8_t((totalSeconds) & 0xFF));
    EEPROM.write(49, uint8_t((totalSeconds >> 8) & 0xFF));
    EEPROM.write(50, uint8_t((totalSeconds >> 16) & 0xFF));
    EEPROM.write(51, uint8_t((totalSeconds >> 24) & 0xFF));
    totalSeconds = uint32_t(EEPROM.read(51) << 24) | uint32_t(EEPROM.read(50) << 16) | uint32_t(EEPROM.read(49) << 8) | uint32_t(EEPROM.read(48));
    if (currentSeconds % 60 == 0) {
      currentMinutes = uint32_t(EEPROM.read(39) << 24) | uint32_t(EEPROM.read(38) << 16) | uint32_t(EEPROM.read(37) << 8) | uint32_t(EEPROM.read(36));
      currentMinutes++;
      EEPROM.write(36, uint8_t((currentMinutes) & 0xFF));
      EEPROM.write(37, uint8_t((currentMinutes >> 8) & 0xFF));
      EEPROM.write(38, uint8_t((currentMinutes >> 16) & 0xFF));
      EEPROM.write(39, uint8_t((currentMinutes >> 24) & 0xFF));
      currentMinutes = uint32_t(EEPROM.read(39) << 24) | uint32_t(EEPROM.read(38) << 16) | uint32_t(EEPROM.read(37) << 8) | uint32_t(EEPROM.read(36));
      //
      totalMinutes = uint32_t(EEPROM.read(55) << 24) | uint32_t(EEPROM.read(54) << 16) | uint32_t(EEPROM.read(53) << 8) | uint32_t(EEPROM.read(52));
      totalMinutes++;
      EEPROM.write(52, uint8_t((totalMinutes) & 0xFF));
      EEPROM.write(53, uint8_t((totalMinutes >> 8) & 0xFF));
      EEPROM.write(54, uint8_t((totalMinutes >> 16) & 0xFF));
      EEPROM.write(55, uint8_t((totalMinutes >> 24) & 0xFF));
      totalMinutes = uint32_t(EEPROM.read(55) << 24) | uint32_t(EEPROM.read(54) << 16) | uint32_t(EEPROM.read(53) << 8) | uint32_t(EEPROM.read(52));
      if (currentMinutes % 60 == 0) {
        currentHours = uint32_t(EEPROM.read(43) << 24) | uint32_t(EEPROM.read(42) << 16) | uint32_t(EEPROM.read(41) << 8) | uint32_t(EEPROM.read(40));
        currentHours++;
        EEPROM.write(40, uint8_t((currentHours) & 0xFF));
        EEPROM.write(41, uint8_t((currentHours >> 8) & 0xFF));
        EEPROM.write(42, uint8_t((currentHours >> 16) & 0xFF));
        EEPROM.write(43, uint8_t((currentHours >> 24) & 0xFF));
        currentHours = uint32_t(EEPROM.read(43) << 24) | uint32_t(EEPROM.read(42) << 16) | uint32_t(EEPROM.read(41) << 8) | uint32_t(EEPROM.read(40));
        //
        totalHours = uint32_t(EEPROM.read(59) << 24) | uint32_t(EEPROM.read(58) << 16) | uint32_t(EEPROM.read(57) << 8) | uint32_t(EEPROM.read(56));
        totalHours++;
        EEPROM.write(56, uint8_t((totalHours) & 0xFF));
        EEPROM.write(57, uint8_t((totalHours >> 8) & 0xFF));
        EEPROM.write(58, uint8_t((totalHours >> 16) & 0xFF));
        EEPROM.write(59, uint8_t((totalHours >> 24) & 0xFF));
        totalHours = uint32_t(EEPROM.read(59) << 24) | uint32_t(EEPROM.read(58) << 16) | uint32_t(EEPROM.read(57) << 8) | uint32_t(EEPROM.read(56));
        if (currentHours % 24 == 0) {
          currentDays = uint32_t(EEPROM.read(47) << 24) | uint32_t(EEPROM.read(46) << 16) | uint32_t(EEPROM.read(45) << 8) | uint32_t(EEPROM.read(44));
          currentDays++;
          EEPROM.write(44, uint8_t((currentDays) & 0xFF));
          EEPROM.write(45, uint8_t((currentDays >> 8) & 0xFF));
          EEPROM.write(46, uint8_t((currentDays >> 16) & 0xFF));
          EEPROM.write(47, uint8_t((currentDays >> 24) & 0xFF));
          currentDays = uint32_t(EEPROM.read(47) << 24) | uint32_t(EEPROM.read(46) << 16) | uint32_t(EEPROM.read(45) << 8) | uint32_t(EEPROM.read(44));
          //
          totalDays = uint32_t(EEPROM.read(63) << 24) | uint32_t(EEPROM.read(62) << 16) | uint32_t(EEPROM.read(61) << 8) | uint32_t(EEPROM.read(60));
          totalDays++;
          EEPROM.write(60, uint8_t((totalDays) & 0xFF));
          EEPROM.write(61, uint8_t((totalDays >> 8) & 0xFF));
          EEPROM.write(62, uint8_t((totalDays >> 16) & 0xFF));
          EEPROM.write(63, uint8_t((totalDays >> 24) & 0xFF));
          totalDays = uint32_t(EEPROM.read(63) << 24) | uint32_t(EEPROM.read(62) << 16) | uint32_t(EEPROM.read(61) << 8) | uint32_t(EEPROM.read(60));
        }
      }
    }
    //EEPROM.commit();
    if (totalSeconds >= 60) {
      totalSeconds = uint32_t(EEPROM.read(51) << 24) | uint32_t(EEPROM.read(50) << 16) | uint32_t(EEPROM.read(49) << 8) | uint32_t(EEPROM.read(48));
      totalSeconds = 0;
      EEPROM.write(48, uint8_t((totalSeconds) & 0xFF));
      EEPROM.write(49, uint8_t((totalSeconds >> 8) & 0xFF));
      EEPROM.write(50, uint8_t((totalSeconds >> 16) & 0xFF));
      EEPROM.write(51, uint8_t((totalSeconds >> 24) & 0xFF));
      totalSeconds = uint32_t(EEPROM.read(51) << 24) | uint32_t(EEPROM.read(50) << 16) | uint32_t(EEPROM.read(49) << 8) | uint32_t(EEPROM.read(48));
    }
    if (totalMinutes >= 60) {
      totalMinutes = uint32_t(EEPROM.read(55) << 24) | uint32_t(EEPROM.read(54) << 16) | uint32_t(EEPROM.read(53) << 8) | uint32_t(EEPROM.read(52));
      totalMinutes = 0;
      EEPROM.write(52, uint8_t((totalMinutes) & 0xFF));
      EEPROM.write(53, uint8_t((totalMinutes >> 8) & 0xFF));
      EEPROM.write(54, uint8_t((totalMinutes >> 16) & 0xFF));
      EEPROM.write(55, uint8_t((totalMinutes >> 24) & 0xFF));
      totalMinutes = uint32_t(EEPROM.read(55) << 24) | uint32_t(EEPROM.read(54) << 16) | uint32_t(EEPROM.read(53) << 8) | uint32_t(EEPROM.read(52));
    }
    if (totalHours >= 24) {
      totalHours = uint32_t(EEPROM.read(59) << 24) | uint32_t(EEPROM.read(58) << 16) | uint32_t(EEPROM.read(57) << 8) | uint32_t(EEPROM.read(56));
      totalHours = 0;
      EEPROM.write(56, uint8_t((totalHours) & 0xFF));
      EEPROM.write(57, uint8_t((totalHours >> 8) & 0xFF));
      EEPROM.write(58, uint8_t((totalHours >> 16) & 0xFF));
      EEPROM.write(59, uint8_t((totalHours >> 24) & 0xFF));
      totalHours = uint32_t(EEPROM.read(59) << 24) | uint32_t(EEPROM.read(58) << 16) | uint32_t(EEPROM.read(57) << 8) | uint32_t(EEPROM.read(56));
    }
    EEPROM.commit();
    //Serial.print("currentSeconds AFTER = ");
    //Serial.println(currentSeconds);
  }
  millisToSecondsPrevious = millisToSecondsCurrent;
  // Try to connect to chat. If it loses connection try again
  //Serial.print("Twitch IRC Connection Status = ");
  //Serial.println(client.connected());
  /*
    Serial.print("WiFi.status() = ");
    Serial.print(WiFi.status());
    Serial.print(" , WL_CONNECTED = ");
    Serial.println(WL_CONNECTED);
  */
  counterStuff();
  getResetButtonState();
  playLedAnimations();
  if (waitingForPong == true) {
    if (pongTimeoutMillisPrevious <= millis()) {
      Serial.print("Timed out while waiting for PONG response from Twitch, disconnecting.");
      Serial.print(" pingSentAt = ");
      Serial.print(pingSentAt);
      Serial.print(" millis() = ");
      Serial.println(millis());
      waitingForPong = false;
      client.disconnect();
      Serial.println("Disconnected from Twitch.");
    }
  }
  if (client.connected()) {
    client.loop();
    return;
  }
  waitingForPong = false;
  Serial.println("Twitch disconnected, what happened?");
  Serial.print("ssid = ");
  Serial.print(ssid);
  Serial.print(" , WiFi.status() = ");
  Serial.print(WiFi.status());
  Serial.print(" , WL_CONNECTED = ");
  Serial.println(WL_CONNECTED);
  if (WiFi.status() != WL_CONNECTED) {
    // Do something to reconnect the WIFI
    Serial.println("Attempting to disconnect");
    WiFi.disconnect();
    Serial.print("ssid = ");
    Serial.print(ssid);
    Serial.print(" , WiFi.status() = ");
    Serial.print(WiFi.status());
    Serial.print(" , WL_CONNECTED = ");
    Serial.println(WL_CONNECTED);
    Serial.println("Attempting to begin");
    WiFi.begin(ssid, password);
    Serial.print("ssid = ");
    Serial.print(ssid);
    Serial.print(" , WiFi.status() = ");
    Serial.print(WiFi.status());
    Serial.print(" , WL_CONNECTED = ");
    Serial.println(WL_CONNECTED);
    Serial.println("Hopefully it worked? 1");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("WAITING TO RECONNECT ssid = ");
      Serial.print(ssid);
      Serial.print(" , WiFi.status() = ");
      Serial.print(WiFi.status());
      Serial.print(" , WL_CONNECTED = ");
      Serial.println(WL_CONNECTED);
      //Serial.print(".");
      delay(500);
    }
    Serial.println("Hopefully it worked? 2");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("A wifiDisconnectCounter = ");
      Serial.println(wifiDisconnectCounter);
      wifiDisconnectCounter = uint32_t(EEPROM.read(27) << 24) | uint32_t(EEPROM.read(26) << 16) | uint32_t(EEPROM.read(25) << 8) | uint32_t(EEPROM.read(24));
      Serial.print("B wifiDisconnectCounter = ");
      Serial.println(wifiDisconnectCounter);
      wifiDisconnectCounter++;
      Serial.print("C wifiDisconnectCounter = ");
      Serial.println(wifiDisconnectCounter);
      EEPROM.write(24, uint8_t((wifiDisconnectCounter) & 0xFF));
      EEPROM.write(25, uint8_t((wifiDisconnectCounter >> 8) & 0xFF));
      EEPROM.write(26, uint8_t((wifiDisconnectCounter >> 16) & 0xFF));
      EEPROM.write(27, uint8_t((wifiDisconnectCounter >> 24) & 0xFF));
      EEPROM.commit();
      wifiDisconnectCounter = uint32_t(EEPROM.read(27) << 24) | uint32_t(EEPROM.read(26) << 16) | uint32_t(EEPROM.read(25) << 8) | uint32_t(EEPROM.read(24));
      Serial.print("D wifiDisconnectCounter = ");
      Serial.println(wifiDisconnectCounter);
    }
  }
  if (!client.connected()) {
    Serial.println("AND AGAIN");
    Serial.print("ssid = ");
    Serial.print(ssid);
    Serial.print(" , WiFi.status() = ");
    Serial.print(WiFi.status());
    Serial.print(" , WL_CONNECTED = ");
    Serial.println(WL_CONNECTED);
    Serial.println("Attempting to connect to twitch");
    // Attempt to connect
    // Second param is not needed by Twtich
    // Name, Name, Pass
    if (client.connect(TWITCH_BOT_NAME, TWITCH_BOT_NAME, TWITCH_OAUTH_TOKEN)) {
      Serial.println("Connecting(?) to twitch");
      if (incrementIrcDisconnectCounter == true) {
        Serial.print("A ircDisconnectCounter = ");
        Serial.println(ircDisconnectCounter);
        ircDisconnectCounter = uint32_t(EEPROM.read(31) << 24) | uint32_t(EEPROM.read(30) << 16) | uint32_t(EEPROM.read(29) << 8) | uint32_t(EEPROM.read(28));
        Serial.print("B ircDisconnectCounter = ");
        Serial.println(ircDisconnectCounter);
        ircDisconnectCounter++;
        Serial.print("C ircDisconnectCounter = ");
        Serial.println(ircDisconnectCounter);
        EEPROM.write(28, uint8_t((ircDisconnectCounter) & 0xFF));
        EEPROM.write(29, uint8_t((ircDisconnectCounter >> 8) & 0xFF));
        EEPROM.write(30, uint8_t((ircDisconnectCounter >> 16) & 0xFF));
        EEPROM.write(31, uint8_t((ircDisconnectCounter >> 24) & 0xFF));
        EEPROM.commit();
        ircDisconnectCounter = uint32_t(EEPROM.read(31) << 24) | uint32_t(EEPROM.read(30) << 16) | uint32_t(EEPROM.read(29) << 8) | uint32_t(EEPROM.read(28));
        Serial.print("D ircDisconnectCounter = ");
        Serial.println(ircDisconnectCounter);
      }
      if (incrementIrcDisconnectCounter == false) {
        Serial.println("DO NOT INCREMENT IRC DISCONNECT COUNTER");
        incrementIrcDisconnectCounter = true;
      }
      //client.sendCapReq("twitch.tv/commands");
      //client.sendCapReq("twitch.tv/tags");
      //client.sendCapReq("twitch.tv/membership");
      for (unsigned int twitchChannelIndex = 0; twitchChannelIndex < sizeof(twitchChannelName) / sizeof(twitchChannelName[0]); twitchChannelIndex++) {
        Serial.print(twitchChannelIndex);
        Serial.print(" = ");
        Serial.println(twitchChannelName[twitchChannelIndex]);
        client.joinChannel(twitchChannelName[twitchChannelIndex]);
        //client.sendRaw("JOIN " + twitchChannelName[twitchChannelIndex]);
      }
      redLevel = EEPROM.read(0);
      greenLevel = EEPROM.read(1);
      blueLevel =  EEPROM.read(2);

      counter1StepsToIncrement = uint32_t(EEPROM.read(7) << 24) | uint32_t(EEPROM.read(6) << 16) | uint32_t(EEPROM.read(5) << 8) | uint32_t(EEPROM.read(4));
      counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
      totalMessagesSent = uint32_t(EEPROM.read(15) << 24) | uint32_t(EEPROM.read(14) << 16) | uint32_t(EEPROM.read(13) << 8) | uint32_t(EEPROM.read(12));
      totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));

      currentSeconds = uint32_t(EEPROM.read(35) << 24) | uint32_t(EEPROM.read(34) << 16) | uint32_t(EEPROM.read(33) << 8) | uint32_t(EEPROM.read(32));
      currentMinutes = uint32_t(EEPROM.read(39) << 24) | uint32_t(EEPROM.read(38) << 16) | uint32_t(EEPROM.read(37) << 8) | uint32_t(EEPROM.read(36));
      currentHours = uint32_t(EEPROM.read(43) << 24) | uint32_t(EEPROM.read(42) << 16) | uint32_t(EEPROM.read(41) << 8) | uint32_t(EEPROM.read(40));
      currentDays = uint32_t(EEPROM.read(47) << 24) | uint32_t(EEPROM.read(46) << 16) | uint32_t(EEPROM.read(45) << 8) | uint32_t(EEPROM.read(44));

      totalSeconds = uint32_t(EEPROM.read(51) << 24) | uint32_t(EEPROM.read(50) << 16) | uint32_t(EEPROM.read(49) << 8) | uint32_t(EEPROM.read(48));
      totalMinutes = uint32_t(EEPROM.read(55) << 24) | uint32_t(EEPROM.read(54) << 16) | uint32_t(EEPROM.read(53) << 8) | uint32_t(EEPROM.read(52));
      totalHours = uint32_t(EEPROM.read(59) << 24) | uint32_t(EEPROM.read(58) << 16) | uint32_t(EEPROM.read(57) << 8) | uint32_t(EEPROM.read(56));
      totalDays = uint32_t(EEPROM.read(63) << 24) | uint32_t(EEPROM.read(62) << 16) | uint32_t(EEPROM.read(61) << 8) | uint32_t(EEPROM.read(60));
      uptimeRequestedCounter = uint32_t(EEPROM.read(67) << 24) | uint32_t(EEPROM.read(66) << 16) | uint32_t(EEPROM.read(65) << 8) | uint32_t(EEPROM.read(64));

      bootCounter = uint32_t(EEPROM.read(23) << 24) | uint32_t(EEPROM.read(22) << 16) | uint32_t(EEPROM.read(21) << 8) | uint32_t(EEPROM.read(20));
      wifiDisconnectCounter = uint32_t(EEPROM.read(27) << 24) | uint32_t(EEPROM.read(26) << 16) | uint32_t(EEPROM.read(25) << 8) | uint32_t(EEPROM.read(24));
      ircDisconnectCounter = uint32_t(EEPROM.read(31) << 24) | uint32_t(EEPROM.read(30) << 16) | uint32_t(EEPROM.read(29) << 8) | uint32_t(EEPROM.read(28));

      Serial.println("Connected! PogChamp");
      uint32_t uptimeMillisTotal = millis();
      uint32_t uptimeDays = (uptimeMillisTotal / 86400000);
      uint32_t uptimeHours = (uptimeMillisTotal / 3600000) % 24;
      uint32_t uptimeMinutes = (uptimeMillisTotal / 60000) % 60;
      uint32_t uptimeSeconds = (uptimeMillisTotal / 1000) % 60;
      uint32_t uptimeMillis = (uptimeMillisTotal % 1000);
      String uptimeStringA = String(uptimeDays) + "d " + String(uptimeHours) + "h " + String(uptimeMinutes) + "m " + String(uptimeSeconds) + "s " + String(uptimeMillis) + "ms , " + String(uptimeMillisTotal) + " total ms";
      String uptimeStringB = String(currentDays) + "d " + String(currentHours) + "h " + String(currentMinutes) + "m " + String(currentSeconds) + "s";
      String uptimeStringC = String(totalDays) + "d " + String(totalHours) + "h " + String(totalMinutes) + "m " + String(totalSeconds) + "s";
      client.sendAction(debugTwitchChannel, "Connected! PogChamp bootCounter = " + String(bootCounter) + " , wifiDisconnectCounter = " + String(wifiDisconnectCounter) + " , ircDisconnectCounter = " + String(ircDisconnectCounter) + " , uptimeRequestedCounter = " + String(uptimeRequestedCounter) + " , uptimeA = " + uptimeStringA + " , uptimeB = " + uptimeStringB + " , uptimeC = " + uptimeStringC + " , totalMessagesSent = " + String(totalMessagesSent) + " , totalLedTapeRewardsRedeemed = " + String(totalLedTapeRewardsRedeemed));
      //client.sendWhisper("twitchtriestoplay", "YOOOOOOOOOOOOOO LFG");
    } else {
      Serial.println("failed... try again in 1 seconds");
      // Wait 1 second before retrying
      delay(1000);
    }
    return;
  }
  client.loop();
}

/*
  void sendAction(String channelName, String message) {
  client.sendMessage(channelName, "ACTION " + message + "");
  }
*/

void sendTwitchPing() {
  if (waitingForPong == true) {
    return;
  }
  if (waitingForPong == false) {
    //Serial.println("[IRC] Sending Ping");
    client.sendRaw("PING");
    pingSentAt = millis();
    pingDelayMillisPrevious = millis() + pingDelayMillis;
    pongTimeoutMillisPrevious = millis() + pongTimeoutMillis;
    waitingForPong = true;
    /*
      if (pingDelayMillisPrevious > millis()) {
      Serial.println("Dont send Ping");
      }
    */
    /*
      if (pingDelayMillisPrevious <= millis()) {
      Serial.println("[IRC] Sending Ping");
      client.sendRaw("PING");
      pingDelayMillisPrevious = millis() + pingDelayMillis;
      pongTimeoutMillisPrevious = millis() + pongTimeoutMillis;
      waitingForPong = true;
      }
    */
  }
}

void getResetButtonState() {
  //Serial.println(millis());
  if (ledTapeMode > 2) {
    Serial.println("A Resetting LED TAPE MODE to 0");
    Serial.print("A ledTapeMode = ");
    Serial.println(ledTapeMode);

    ledTapeMode = EEPROM.read(68);
    ledAnimationMode = EEPROM.read(69);

    Serial.print("B ledTapeMode = ");
    Serial.println(ledTapeMode);

    ledTapeMode = 0;

    Serial.print("C ledTapeMode = ");
    Serial.println(ledTapeMode);

    EEPROM.write(68, ledTapeMode);
    EEPROM.commit();

    Serial.print("D ledTapeMode = ");
    Serial.println(ledTapeMode);

    ledTapeMode = EEPROM.read(68);
    ledAnimationMode = EEPROM.read(69);

    Serial.print("E ledTapeMode = ");
    Serial.println(ledTapeMode);
  }
  resetButtonStateCurrent = !digitalRead(resetButton);
  if (resetButtonStateCurrent != resetButtonStatePrevious) {
    //Serial.println(resetButtonStateCurrent);
    //Serial.println(resetButtonStatePrevious);
    if (resetButtonStateCurrent == 1) {
      /*
        if (resetButtonDebounceMillisPrevious > millis()) {
        Serial.println("Ignoring button press");
        }
      */
      if (resetButtonDebounceMillisPrevious <= millis()) {
        Serial.print("A ledTapeMode = ");
        Serial.println(ledTapeMode);
        ledTapeMode = EEPROM.read(68);
        ledAnimationMode = EEPROM.read(69);
        Serial.print("B ledTapeMode = ");
        Serial.println(ledTapeMode);
        Serial.println("Accepting button press");
        resetButtonDebounceMillisPrevious = millis() + resetButtonDebounceMillis;
        //Serial.println(ledTapeMode);
        if (ledTapeMode > 2) {
          Serial.println("B Resetting LED TAPE MODE to 0");
          Serial.print("A ledTapeMode = ");
          Serial.println(ledTapeMode);

          ledTapeMode = EEPROM.read(68);
          ledAnimationMode = EEPROM.read(69);

          Serial.print("B ledTapeMode = ");
          Serial.println(ledTapeMode);

          ledTapeMode = 0;

          Serial.print("C ledTapeMode = ");
          Serial.println(ledTapeMode);

          EEPROM.write(68, ledTapeMode);
          EEPROM.commit();

          Serial.print("D ledTapeMode = ");
          Serial.println(ledTapeMode);

          ledTapeMode = EEPROM.read(68);
          ledAnimationMode = EEPROM.read(69);

          Serial.print("E ledTapeMode = ");
          Serial.println(ledTapeMode);
        }
        //Serial.println(ledTapeMode);
        ledTapeMode++;
        //Serial.println(ledTapeMode);
        if (ledTapeMode > 2) {
          Serial.println("C Resetting LED TAPE MODE to 0");
          Serial.print("A ledTapeMode = ");
          Serial.println(ledTapeMode);

          ledTapeMode = EEPROM.read(68);
          ledAnimationMode = EEPROM.read(69);

          Serial.print("B ledTapeMode = ");
          Serial.println(ledTapeMode);

          ledTapeMode = 0;

          Serial.print("C ledTapeMode = ");
          Serial.println(ledTapeMode);

          EEPROM.write(68, ledTapeMode);
          EEPROM.commit();

          Serial.print("D ledTapeMode = ");
          Serial.println(ledTapeMode);

          ledTapeMode = EEPROM.read(68);
          ledAnimationMode = EEPROM.read(69);

          Serial.print("E ledTapeMode = ");
          Serial.println(ledTapeMode);
        }
        if (ledTapeMode == 0) {
          // Do something
          if (client.connected()) {
            client.sendAction(debugTwitchChannel, "ledTapeMode = " + String(ledTapeMode) + " (Twitch LED Tape)");
          }
          Serial.println("Twitch LED tape");
          redLevel = EEPROM.read(0);
          greenLevel = EEPROM.read(1);
          blueLevel =  EEPROM.read(2);
          analogWrite(rgbRed, redLevel);
          analogWrite(rgbGreen, greenLevel);
          analogWrite(rgbBlue, blueLevel);
        }
        if (ledTapeMode == 1) {
          if (client.connected()) {
            client.sendAction(debugTwitchChannel, "ledTapeMode = " + String(ledTapeMode) + " (LED Tape Off)");
          }
          Serial.println("Turn LED tape off");
          analogWrite(rgbRed, 0);
          analogWrite(rgbGreen, 0);
          analogWrite(rgbBlue, 0);
        }
        if (ledTapeMode == 2) {
          if (client.connected()) {
            client.sendAction(debugTwitchChannel, "ledTapeMode = " + String(ledTapeMode) + " (LED Tape On)");
          }
          Serial.println("Turn LED tape on to full brightness");
          analogWrite(rgbRed, 255);
          analogWrite(rgbGreen, 255);
          analogWrite(rgbBlue, 255);
        }
        Serial.println(ledTapeMode);
        //Serial.println("BUTTON PRESSED");
        Serial.print("C ledTapeMode = ");
        Serial.println(ledTapeMode);
        EEPROM.write(68, ledTapeMode);
        EEPROM.commit();
        Serial.print("D ledTapeMode = ");
        Serial.println(ledTapeMode);
        ledTapeMode = EEPROM.read(68);
        ledAnimationMode = EEPROM.read(69);
        Serial.print("E ledTapeMode = ");
        Serial.println(ledTapeMode);
      }
    }
    if (resetButtonStateCurrent == 0) {
      //Serial.println("BUTTON RELEASED");
    }
  }
  resetButtonStatePrevious = resetButtonStateCurrent;
}
void counterStuff() {
  counter1StepsToIncrement = uint32_t(EEPROM.read(7) << 24) | uint32_t(EEPROM.read(6) << 16) | uint32_t(EEPROM.read(5) << 8) | uint32_t(EEPROM.read(4));
  if (counter1StepsToIncrement > 0) {
    if (millis() - counter1OnTimeMillisPrevious >= counterOnTimeMillis) {
      counter1OnTimeMillisPrevious = millis();
      if (counter1State == LOW) {
        // Turn on counter
        counter1State = HIGH;
        digitalWrite(counter1, counter1State);
      } else {
        // Turn off counter
        counter1State = LOW;
        digitalWrite(counter1, counter1State);
        //Serial.println("READING NUMBER FROM EEPROM");
        counter1StepsToIncrement = uint32_t(EEPROM.read(7) << 24) | uint32_t(EEPROM.read(6) << 16) | uint32_t(EEPROM.read(5) << 8) | uint32_t(EEPROM.read(4));
        totalMessagesSent = uint32_t(EEPROM.read(15) << 24) | uint32_t(EEPROM.read(14) << 16) | uint32_t(EEPROM.read(13) << 8) | uint32_t(EEPROM.read(12));
        //Serial.print("totalMessagesSent = ");
        //Serial.println(totalMessagesSent);
        //Serial.println(counter1StepsToIncrement, DEC);
        //Serial.println(counter1StepsToIncrement, HEX);
        counter1StepsToIncrement--;
        EEPROM.write(4, uint8_t((counter1StepsToIncrement) & 0xFF));
        EEPROM.write(5, uint8_t((counter1StepsToIncrement >> 8) & 0xFF));
        EEPROM.write(6, uint8_t((counter1StepsToIncrement >> 16) & 0xFF));
        EEPROM.write(7, uint8_t((counter1StepsToIncrement >> 24) & 0xFF));
        EEPROM.commit();
        //
        counter1StepsToIncrement = uint32_t(EEPROM.read(7) << 24) | uint32_t(EEPROM.read(6) << 16) | uint32_t(EEPROM.read(5) << 8) | uint32_t(EEPROM.read(4));
        totalMessagesSent = uint32_t(EEPROM.read(15) << 24) | uint32_t(EEPROM.read(14) << 16) | uint32_t(EEPROM.read(13) << 8) | uint32_t(EEPROM.read(12));
        //Serial.print("totalMessagesSent = ");
        //Serial.println(totalMessagesSent);
        //Serial.println(counter1StepsToIncrement, DEC);
        //Serial.println(counter1StepsToIncrement, HEX);
        //Serial.println("NUMBER READ FROM EEPROM");
      }
    }
  }
  counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
  if (counter2StepsToIncrement > 0) {
    if (millis() - counter2OnTimeMillisPrevious >= counterOnTimeMillis) {
      counter2OnTimeMillisPrevious = millis();
      if (counter2State == LOW) {
        // Turn on counter
        counter2State = HIGH;
        digitalWrite(counter2, counter2State);
      } else {
        // Turn off counter
        counter2State = LOW;
        digitalWrite(counter2, counter2State);
        //Serial.println("READING NUMBER FROM EEPROM");
        counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
        totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
        //Serial.print("totalLedTapeRewardsRedeemed = ");
        //Serial.println(totalLedTapeRewardsRedeemed);
        //Serial.println(counter2StepsToIncrement, DEC);
        //Serial.println(counter2StepsToIncrement, HEX);
        counter2StepsToIncrement--;
        EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
        EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
        EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
        EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));
        EEPROM.commit();
        //
        counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
        totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
        //Serial.print("totalLedTapeRewardsRedeemed = ");
        //Serial.println(totalLedTapeRewardsRedeemed);
        //Serial.println(counter2StepsToIncrement, DEC);
        //Serial.println(counter2StepsToIncrement, HEX);
        //Serial.println("NUMBER READ FROM EEPROM");
      }
    }
  }
}

void playLedAnimations() {
  if (ledTapeMode == 0) {
    if (ledAnimationMode == 0) {
      // Solid color only
      // Wait one second then go back to the last solid color?
    }
    if (ledAnimationMode == 1) {
      // rainbow1
      // Play the Rainbow effect
      if (millis() >= rainbowStepTimer) {
        rainbowStepTimer = millis() + rainbowStepDelay;
        if (rainbowValueRed >= 255 && rainbowValueGreen < 255 && rainbowValueBlue == 0) {
          // Switch from red to yellow
          rainbowValueGreen++;
          /*
            Serial.println("Step 0");
            Serial.print("rainbowValueGreen = ");
            Serial.println(rainbowValueGreen);
          */
        }
        if (rainbowValueRed > 0 && rainbowValueGreen >= 255 && rainbowValueBlue == 0) {
          // Switch from yellow to green
          rainbowValueRed--;
          /*
            Serial.println("Step 1");
            Serial.print("rainbowValueRed = ");
            Serial.println(rainbowValueRed);
          */
        }
        if (rainbowValueRed == 0 && rainbowValueGreen > 0 && rainbowValueBlue < 255) {
          // Switch from green to cyan
          rainbowValueBlue++;
          /*
            Serial.println("Step 2");
            Serial.print("rainbowValueBlue = ");
            Serial.println(rainbowValueBlue);
          */
        }
        if (rainbowValueRed == 0 && rainbowValueGreen > 0 && rainbowValueBlue >= 255) {
          // Switch from cyan to blue
          rainbowValueGreen--;
          /*
            Serial.println("Step 3");
            Serial.print("rainbowValueGreen = ");
            Serial.println(rainbowValueGreen);
          */
        }
        if (rainbowValueRed < 255 && rainbowValueGreen == 0 && rainbowValueBlue >= 255) {
          // Switch from blue to magenta
          rainbowValueRed++;
          /*
            Serial.println("Step 4");
            Serial.print("rainbowValueRed = ");
            Serial.println(rainbowValueRed);
          */
        }
        if (rainbowValueRed >= 255 && rainbowValueGreen == 0 && rainbowValueBlue > 0) {
          // Switch from magenta to red
          rainbowValueBlue--;
          /*
            Serial.println("Step 5");
            Serial.print("rainbowValueBlue = ");
            Serial.println(rainbowValueBlue);
          */
        }
        /*
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
        */
        analogWrite(rgbRed, rainbowValueRed);
        analogWrite(rgbGreen, rainbowValueGreen);
        analogWrite(rgbBlue, rainbowValueBlue);
      }
    }
    if (ledAnimationMode == 2) {
      // Flashbang
      // Play the Flashbang effect
      if (startFlashbangAnimation == true) {
        //Serial.println("Starting flashbang animation");
        flashbangValue = 255;
        analogWrite(rgbRed, flashbangValue);
        analogWrite(rgbGreen, flashbangValue);
        analogWrite(rgbBlue, flashbangValue);
        flashbangTimer = millis() + flashbangDelay;
        //Serial.print("flashbangTimer = ");
        //Serial.println(flashbangTimer);
        startFlashbangAnimation = false;
      }
      if (millis() >= flashbangTimer) {
        //Serial.println("test");
        //flashbangDecrementDelay = 33;
        //flashbangDecrementTimer = millis() + flashbangDecrementDelay;
        if (flashbangValue <= 0) {
          //Serial.println("Animation ended?");
          ledAnimationMode = EEPROM.read(69);
          ledAnimationMode = 0;
          EEPROM.write(69, ledAnimationMode);
          EEPROM.commit();
          ledAnimationMode = EEPROM.read(69);
          flashbangValue = 0;
          analogWrite(rgbRed, flashbangValue);
          analogWrite(rgbGreen, flashbangValue);
          analogWrite(rgbBlue, flashbangValue);
        }
        if (millis() >= flashbangDecrementTimer) {
          flashbangDecrementTimer = millis() + flashbangDecrementDelay;
          flashbangValue--;
          //Serial.print("flashbangValue = ");
          //Serial.println(flashbangValue);
          analogWrite(rgbRed, flashbangValue);
          analogWrite(rgbGreen, flashbangValue);
          analogWrite(rgbBlue, flashbangValue);
          //flashbangValue--;
        }
        if (flashbangValue <= 0) {
          //Serial.println("Animation ended?");
          ledAnimationMode = EEPROM.read(69);
          ledAnimationMode = 0;
          EEPROM.write(69, ledAnimationMode);
          EEPROM.commit();
          ledAnimationMode = EEPROM.read(69);
          flashbangValue = 0;
          analogWrite(rgbRed, flashbangValue);
          analogWrite(rgbGreen, flashbangValue);
          analogWrite(rgbBlue, flashbangValue);
        }
      }
    }
    if (ledAnimationMode == 3) {
      // rainbow2
      if (millis() >= rainbowStepTimer) {
        rainbowStepTimer = millis() + rainbowStepDelay;
        if (rainbowValueRed >= 255 && rainbowValueGreen < 255 && rainbowValueBlue == 0) {
          // Switch from red to yellow
          rainbowValueGreen++;
        }
        if (rainbowValueRed >= 255 && rainbowValueGreen >= 255 && rainbowValueBlue < 255) {
          // Switch from yellow to white
          rainbowValueBlue++;
        }
        if (rainbowValueRed > 0 && rainbowValueGreen >= 255 && rainbowValueBlue >= 255) {
          // Switch from white to cyan
          rainbowValueRed--;
        }
        if (rainbowValueRed == 0 && rainbowValueGreen > 0 && rainbowValueBlue >= 255) {
          // Switch from cyan to blue
          rainbowValueGreen--;
        }
        if (rainbowValueRed == 0 && rainbowValueGreen == 0 && rainbowValueBlue > 0) {
          // Switch from blue to black
          rainbowValueBlue--;
        }
        if (rainbowValueRed < 255 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
          // Switch from black to red
          rainbowValueRed++;
        }
        /*
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
        */
        analogWrite(rgbRed, rainbowValueRed);
        analogWrite(rgbGreen, rainbowValueGreen);
        analogWrite(rgbBlue, rainbowValueBlue);
      }
    }
    if (ledAnimationMode == 4) {
      // rainbow3
      if (millis() >= rainbowStepTimer) {
        rainbowStepTimer = millis() + rainbowStepDelay;
        if (rainbowValueRed >= 1 && rainbowValueRed < 254 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
          // Switch from black to red (Assuming initial value is larger than 0, it unsigned so its either larger than 0 or equals 0)
          //Serial.println("Step 1");
          rainbowValueRed++;
          rainbowValueGreen = 0;
          rainbowValueBlue = 0;
        }
        if (rainbowValueRed == 254 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
          // Start switching from red to black
          //Serial.println("Step 2");
          rainbowValueRed++;
          rainbowValueGreen = 1;
          rainbowValueBlue = 0;
        }
        if (rainbowValueRed <= 255 && rainbowValueGreen == 1 && rainbowValueBlue == 0) {
          // Switch from red to black
          //Serial.println("Step 3");
          rainbowValueRed--;
          rainbowValueGreen = 1;
          rainbowValueBlue = 0;
        }
        if (rainbowValueRed == 0 && rainbowValueGreen >= 1 && rainbowValueGreen < 254 && rainbowValueBlue == 0) {
          // Switch from black to green
          //Serial.println("Step 4");
          rainbowValueRed = 0;
          rainbowValueGreen++;
          rainbowValueBlue = 0;
        }
        if (rainbowValueRed == 0 && rainbowValueGreen == 254 && rainbowValueBlue == 0) {
          // Start switching from green to black
          //Serial.println("Step 5");
          rainbowValueRed = 0;
          rainbowValueGreen++;
          rainbowValueBlue = 1;
        }
        if (rainbowValueRed == 0 && rainbowValueGreen <= 255 && rainbowValueBlue == 1) {
          // Switch from green to black
          //Serial.println("Step 6");
          rainbowValueRed = 0;
          rainbowValueGreen--;
          rainbowValueBlue = 1;
        }
        if (rainbowValueRed == 0 && rainbowValueGreen == 0 && rainbowValueBlue >= 1 && rainbowValueBlue < 254) {
          // Switch from black to blue
          //Serial.println("Step 7");
          rainbowValueRed = 0;
          rainbowValueGreen = 0;
          rainbowValueBlue++;
        }
        if (rainbowValueRed == 0 && rainbowValueGreen == 0 && rainbowValueBlue == 254) {
          // Start switching from blue to black
          //Serial.println("Step 8");
          rainbowValueRed = 1;
          rainbowValueGreen = 0;
          rainbowValueBlue++;
        }
        if (rainbowValueRed == 1 && rainbowValueGreen == 0 && rainbowValueBlue <= 255) {
          // Switch from blue to black
          //Serial.println("Step 9");
          rainbowValueRed = 1;
          rainbowValueGreen = 0;
          rainbowValueBlue--;
        }
        /*
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
        */
        analogWrite(rgbRed, rainbowValueRed);
        analogWrite(rgbGreen, rainbowValueGreen);
        analogWrite(rgbBlue, rainbowValueBlue);
      }
    }
    if (ledAnimationMode == 5) {
      // rainbow4
      if (millis() >= rainbowStepTimerSolid) {
        //Serial.println("START");
        rainbowStepTimerSolid = millis() + rainbowStepDelaySolid;
        if (rainbowStepSolidCounter > 5) {
          rainbowStepSolidCounter = 0;
        }
        if (rainbowStepSolidCounter == 0) {
          if (rainbowValueRed >= 1 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
            // Switch from red to red + green
            //Serial.println("Step 0");
            rainbowValueRed = 255;
            rainbowValueGreen = 255;
            rainbowValueBlue = 0;
          }
        }
        if (rainbowStepSolidCounter == 1) {
          if (rainbowValueRed >= 1 && rainbowValueGreen >= 1 && rainbowValueBlue == 0) {
            // Switch from red + green to green
            //Serial.println("Step 1");
            rainbowValueRed = 0;
            rainbowValueGreen = 255;
            rainbowValueBlue = 0;
          }
        }
        if (rainbowStepSolidCounter == 2) {
          if (rainbowValueRed == 0 && rainbowValueGreen >= 1 && rainbowValueBlue == 0) {
            // Switch from green to green + blue
            //Serial.println("Step 2");
            rainbowValueRed = 0;
            rainbowValueGreen = 255;
            rainbowValueBlue = 255;
          }
        }
        if (rainbowStepSolidCounter == 3) {
          if (rainbowValueRed == 0 && rainbowValueGreen >= 1 && rainbowValueBlue >= 1) {
            // Switch from green + blue to blue
            //Serial.println("Step 3");
            rainbowValueRed = 0;
            rainbowValueGreen = 0;
            rainbowValueBlue = 255;
          }
        }
        if (rainbowStepSolidCounter == 4) {
          if (rainbowValueRed == 0 && rainbowValueGreen == 0 && rainbowValueBlue >= 1) {
            // Switch from blue to blue + red
            //Serial.println("Step 4");
            rainbowValueRed = 255;
            rainbowValueGreen = 0;
            rainbowValueBlue = 255;
          }
        }
        if (rainbowStepSolidCounter == 5) {
          if (rainbowValueRed >= 1 && rainbowValueGreen == 0 && rainbowValueBlue >= 1) {
            // Switch from blue + red to red
            //Serial.println("Step 5");
            rainbowValueRed = 255;
            rainbowValueGreen = 0;
            rainbowValueBlue = 0;
          }
        }
        rainbowStepSolidCounter++;
        if (rainbowStepSolidCounter > 5) {
          rainbowStepSolidCounter = 0;
        }
        //Serial.println("END");
        /*
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
        */
        analogWrite(rgbRed, rainbowValueRed);
        analogWrite(rgbGreen, rainbowValueGreen);
        analogWrite(rgbBlue, rainbowValueBlue);
      }
    }
    if (ledAnimationMode == 6) {
      // rainbow5
      if (millis() >= rainbowStepTimerSolid) {
        //Serial.println("START");
        rainbowStepTimerSolid = millis() + rainbowStepDelaySolid;
        if (rainbowStepSolidCounter > 2) {
          rainbowStepSolidCounter = 0;
        }
        if (rainbowStepSolidCounter == 0) {
          if (rainbowValueRed >= 1 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
            // Switch from red to green
            //Serial.println("Step 0");
            rainbowValueRed = 0;
            rainbowValueGreen = 255;
            rainbowValueBlue = 0;
          }
        }
        if (rainbowStepSolidCounter == 1) {
          if (rainbowValueRed == 0 && rainbowValueGreen >= 1 && rainbowValueBlue == 0) {
            // Switch from green to blue
            //Serial.println("Step 1");
            rainbowValueRed = 0;
            rainbowValueGreen = 0;
            rainbowValueBlue = 255;
          }
        }
        if (rainbowStepSolidCounter == 2) {
          if (rainbowValueRed == 0 && rainbowValueGreen == 0 && rainbowValueBlue >= 1) {
            // Switch from blue to red
            //Serial.println("Step 2");
            rainbowValueRed = 255;
            rainbowValueGreen = 0;
            rainbowValueBlue = 0;
          }
        }
        rainbowStepSolidCounter++;
        if (rainbowStepSolidCounter > 2) {
          rainbowStepSolidCounter = 0;
        }
        //Serial.println("END");
        /*
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
        */
        analogWrite(rgbRed, rainbowValueRed);
        analogWrite(rgbGreen, rainbowValueGreen);
        analogWrite(rgbBlue, rainbowValueBlue);
      }
    }
    if (ledAnimationMode == 7) {
      // rainbow6
      if (millis() >= rainbowStepTimer) {
        //Serial.println("START");
        rainbowStepTimer = millis() + rainbowStepDelay;
        if (rainbowValueRed >= 1 && rainbowValueRed < 255 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
          // Switch from black to red (Assuming initial value is larger than 0, it unsigned so its either larger than 0 or equals 0)
          /*
            Serial.println("Step 0");
            Serial.print(rainbowValueRed);
            Serial.print(",");
            Serial.print(rainbowValueGreen);
            Serial.print(",");
            Serial.println(rainbowValueBlue);
          */
          rainbowValueRed++;
          rainbowValueGreen = 0;
          rainbowValueBlue = 0;
          /*
            Serial.print(rainbowValueRed);
            Serial.print(",");
            Serial.print(rainbowValueGreen);
            Serial.print(",");
            Serial.println(rainbowValueBlue);
          */
        }
        if (rainbowValueRed == 255 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
          // Switch from black to red (Assuming initial value is larger than 0, it unsigned so its either larger than 0 or equals 0)
          /*
            Serial.println("Step 1");
            Serial.print(rainbowValueRed);
            Serial.print(",");
            Serial.print(rainbowValueGreen);
            Serial.print(",");
            Serial.println(rainbowValueBlue);
          */
          rainbowValueRed--;
          rainbowValueGreen = 1;
          rainbowValueBlue = 0;
          /*
            Serial.print(rainbowValueRed);
            Serial.print(",");
            Serial.print(rainbowValueGreen);
            Serial.print(",");
            Serial.println(rainbowValueBlue);
          */
        }
        if (rainbowValueRed <= 254 && rainbowValueGreen == 1 && rainbowValueBlue == 0) {
          // Switch from black to red (Assuming initial value is larger than 0, it unsigned so its either larger than 0 or equals 0)
          /*
            Serial.println("Step 2");
            Serial.print(rainbowValueRed);
            Serial.print(",");
            Serial.print(rainbowValueGreen);
            Serial.print(",");
            Serial.println(rainbowValueBlue);
          */
          rainbowValueRed--;
          rainbowValueGreen = 1;
          rainbowValueBlue = 0;
          /*
            Serial.print(rainbowValueRed);
            Serial.print(",");
            Serial.print(rainbowValueGreen);
            Serial.print(",");
            Serial.println(rainbowValueBlue);
          */
        }
        if (rainbowValueRed >= 0 && rainbowValueGreen >= 1 && rainbowValueBlue == 0) {
          // Switch from black to red (Assuming initial value is larger than 0, it unsigned so its either larger than 0 or equals 0)
          /*
            Serial.println("Step 3");
            Serial.print(rainbowValueRed);
            Serial.print(",");
            Serial.print(rainbowValueGreen);
            Serial.print(",");
            Serial.println(rainbowValueBlue);
          */
          rainbowValueRed++;
          rainbowValueGreen++;
          rainbowValueBlue = 0;
          /*
            Serial.print(rainbowValueRed);
            Serial.print(",");
            Serial.print(rainbowValueGreen);
            Serial.print(",");
            Serial.println(rainbowValueBlue);
          */
        }
        if (rainbowValueRed == 0 || rainbowValueGreen == 0) {
          Serial.println("Overflow?");
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
        }
        /*
          if (rainbowValueRed <= 255 && rainbowValueGreen <= 255 && rainbowValueBlue == 0) {
          // Switch from black to red (Assuming initial value is larger than 0, it unsigned so its either larger than 0 or equals 0)
          Serial.println("Step 4");
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          rainbowValueRed--;
          rainbowValueGreen--;
          rainbowValueBlue = 0;
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          }
        */
        /*
          if (rainbowValueRed >= 1 && rainbowValueRed < 254 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
          // Switch from black to red (Assuming initial value is larger than 0, it unsigned so its either larger than 0 or equals 0)
          Serial.println("Step 0");
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          rainbowValueRed++;
          rainbowValueGreen = 0;
          rainbowValueBlue = 0;
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          }
          if (rainbowValueRed == 254 && rainbowValueGreen == 0 && rainbowValueBlue == 0) {
          // Start switching from red to black
          Serial.println("Step 1");
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          rainbowValueRed++;
          rainbowValueGreen = 1;
          rainbowValueBlue = 0;
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          }
          if (rainbowValueRed >= 1 && rainbowValueRed <= 255 && rainbowValueGreen == 1 && rainbowValueBlue == 0) {
          // Switch from red to black
          Serial.println("Step 2");
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          rainbowValueRed--;
          rainbowValueGreen = 1;
          rainbowValueBlue = 0;
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          }
          if (rainbowValueRed >= 1 && rainbowValueRed <= 255 && rainbowValueGreen >= 1 && rainbowValueGreen < 254 && rainbowValueBlue == 0) {
          // Switch from black to green
          Serial.println("Step 3");
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          rainbowValueRed++;
          rainbowValueGreen++;
          rainbowValueBlue = 0;
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
          }
        */
        /*
          if (rainbowValueRed == 254 && rainbowValueGreen == 254 && rainbowValueBlue == 0) {
          // Start switching from green to black
          //Serial.println("Step 5");
          rainbowValueRed = 0;
          rainbowValueGreen++;
          rainbowValueBlue = 1;
          }
          if (rainbowValueRed == 0 && rainbowValueGreen <= 255 && rainbowValueBlue == 1) {
          // Switch from green to black
          //Serial.println("Step 6");
          rainbowValueRed = 0;
          rainbowValueGreen--;
          rainbowValueBlue = 1;
          }
          if (rainbowValueRed == 0 && rainbowValueGreen == 0 && rainbowValueBlue >= 1 && rainbowValueBlue < 254) {
          // Switch from black to blue
          //Serial.println("Step 7");
          rainbowValueRed = 0;
          rainbowValueGreen = 0;
          rainbowValueBlue++;
          }
          if (rainbowValueRed == 0 && rainbowValueGreen == 0 && rainbowValueBlue == 254) {
          // Start switching from blue to black
          //Serial.println("Step 8");
          rainbowValueRed = 1;
          rainbowValueGreen = 0;
          rainbowValueBlue++;
          }
          if (rainbowValueRed == 1 && rainbowValueGreen == 0 && rainbowValueBlue <= 255) {
          // Switch from blue to black
          //Serial.println("Step 9");
          rainbowValueRed = 1;
          rainbowValueGreen = 0;
          rainbowValueBlue--;
          }
        */
        /*
          Serial.print(rainbowValueRed);
          Serial.print(",");
          Serial.print(rainbowValueGreen);
          Serial.print(",");
          Serial.println(rainbowValueBlue);
        */
        analogWrite(rgbRed, rainbowValueRed);
        analogWrite(rgbGreen, rainbowValueGreen);
        analogWrite(rgbBlue, rainbowValueBlue);
        //Serial.println("END");
      }
    }
    if (ledAnimationMode == 255) {
      // Play the debug effect
    }
  }
}


void callback(IRCMessage ircMessage) {
  String displayName = "";
  String customRewardId = "";
  String rawIrcData = ircMessage.original + " ";
  String ircParameters[6] = {"", "", "", "", "", ""};
  //Serial.println("In CallBack");
  //Serial.print("[IRC] ");
  //Serial.println(rawIrcData);
  String testString = "";
  String testString2 = "";
  int lastSpaceFoundAtParameterIndex = -1;
  unsigned int parameterIndexTotal = 6;
  testString = rawIrcData;
  for (unsigned int parameterIndex = 0; parameterIndex < parameterIndexTotal; parameterIndex++) {
    int32_t spaceIndex = testString.indexOf(" ");
    if (spaceIndex >= 0) {
      lastSpaceFoundAtParameterIndex = parameterIndex;
      testString2 = testString.substring(spaceIndex + 1);
      testString = testString.substring(0, spaceIndex);
      if (parameterIndex == parameterIndexTotal - 1) {
        testString = testString + " " + testString2;
        ircParameters[parameterIndex] = testString;
      }
      if (parameterIndex != parameterIndexTotal - 1) {
        ircParameters[parameterIndex] = testString;
        testString = testString2;
      }
    }
  }
  if (lastSpaceFoundAtParameterIndex >= 0) {
    if (ircParameters[0] == "PONG" || ircParameters[1] == "PONG" || ircParameters[2] == "PONG") {
      //Serial.println("RECEIVED PONG RESPONSE FROM OUR PING PogChamp");
      pongReceivedAt = millis();
      pongResponseDelay = pongReceivedAt - pingSentAt;
      //Serial.print("pongResponseDelay = ");
      //Serial.println(pongResponseDelay);
      waitingForPong = false;
    }
    if (ircParameters[0] == "PING" || ircParameters[1] == "PING" || ircParameters[2] == "PING") {
      //Serial.print("RECEIVED PING, RESPONDING WITH ");
      client.sendRaw("PONG " + ircParameters[1]);
      //Serial.println("PONG " + ircParameters[1]);
    }
    if (ircParameters[2] == "PRIVMSG") {

      String displayName = "";
      String messageId = "";
      String roomId = "";
      String userId = "";
      String customRewardId = "";
      String usernameToPing = "";

      String channelName = ircParameters[3].substring(1);
      String chatMessage = ircParameters[4] + " " + ircParameters[5];
      chatMessage = chatMessage.substring(1, chatMessage.length() - 1);
      String actionToFind = "ACTION ";
      int32_t actionStartIndex = chatMessage.indexOf(actionToFind);
      if (actionStartIndex == 0) {
        Serial.println("This is a slash me message, do something about it!");
        int32_t actionToFindLength = actionToFind.length();
        if (actionToFindLength > 0) {
          chatMessage = chatMessage.substring(actionToFindLength);
          Serial.print("No longer an ACTION? ");
          Serial.println(chatMessage);
          actionToFind = "";
          int32_t actionEndIndex = chatMessage.indexOf(actionToFind);
          if (actionEndIndex == int(chatMessage.length()) - 1) {
            actionToFindLength = actionToFind.length();
            if (actionToFindLength > 0) {
              chatMessage = chatMessage.substring(0, chatMessage.length() - 1);
              Serial.print("No longer an ACTION 2? ");
              Serial.println(chatMessage);
            }
          }
        }
      }
      actionToFind = "/me ";
      actionStartIndex = chatMessage.indexOf(actionToFind);
      if (actionStartIndex == 0) {
        Serial.println("This is a fake /me message, remove the /me from the message");
        int32_t actionToFindLength = actionToFind.length();
        if (actionToFindLength > 0) {
          chatMessage = chatMessage.substring(actionToFindLength);
          Serial.print("No longer a fake /me message?");
          Serial.println(chatMessage);
        }
      }
      actionToFind = "\\me ";
      actionStartIndex = chatMessage.indexOf(actionToFind);
      if (actionStartIndex == 0) {
        Serial.println("This is a fake /me message, remove the /me from the message");
        int32_t actionToFindLength = actionToFind.length();
        if (actionToFindLength > 0) {
          chatMessage = chatMessage.substring(actionToFindLength);
          Serial.print("No longer a fake /me message?");
          Serial.println(chatMessage);
        }
      }
      actionToFind = ".me ";
      actionStartIndex = chatMessage.indexOf(actionToFind);
      if (actionStartIndex == 0) {
        Serial.println("This is a fake /me message, remove the /me from the message");
        int32_t actionToFindLength = actionToFind.length();
        if (actionToFindLength > 0) {
          chatMessage = chatMessage.substring(actionToFindLength);
          Serial.print("No longer a fake /me message?");
          Serial.println(chatMessage);
        }
      }
      int32_t userNameIndex = ircParameters[1].indexOf("!");
      String userName = ircParameters[1].substring(1, userNameIndex);
      userName.replace("\\s", "");
      //userName.replace("\s", "");
      userName.replace(" ", "");

      // Get an user's display name
      String tagToFind = "display-name";
      int32_t tagToFindLength = tagToFind.length();
      int32_t tagToFindIndex = ircParameters[0].indexOf(tagToFind);
      //
      if (tagToFindIndex < 0) {
        displayName = "";
      }
      if (tagToFindIndex >= 0) {
        displayName = ircParameters[0].substring(tagToFindIndex + tagToFindLength + 1);
        int32_t displayNameIndex = displayName.indexOf(";");
        displayName = displayName.substring(0, displayNameIndex);
      }
      displayName.replace("\\s", "");
      //displayName.replace("\s", "");
      displayName.replace(" ", "");

      // Get the message ID
      tagToFind = ";id";
      tagToFindLength = tagToFind.length();
      tagToFindIndex = ircParameters[0].indexOf(tagToFind);
      //
      if (tagToFindIndex < 0) {
        messageId = "";
      }
      if (tagToFindIndex >= 0) {
        messageId = ircParameters[0].substring(tagToFindIndex + tagToFindLength + 1);
        int32_t messageIdIndex = messageId.indexOf(";");
        messageId = messageId.substring(0, messageIdIndex);
      }
      messageId.replace("\\s", "");
      //messageId.replace("\s", "");
      messageId.replace(" ", "");

      // Get the room ID
      tagToFind = "room-id";
      tagToFindLength = tagToFind.length();
      tagToFindIndex = ircParameters[0].indexOf(tagToFind);
      //
      if (tagToFindIndex < 0) {
        roomId = "";
      }
      if (tagToFindIndex >= 0) {
        roomId = ircParameters[0].substring(tagToFindIndex + tagToFindLength + 1);
        int32_t roomIdIndex = roomId.indexOf(";");
        roomId = roomId.substring(0, roomIdIndex);
      }

      // Get an user's ID
      tagToFind = "user-id";
      tagToFindLength = tagToFind.length();
      tagToFindIndex = ircParameters[0].indexOf(tagToFind);
      //
      if (tagToFindIndex < 0) {
        userId = "";
      }
      if (tagToFindIndex >= 0) {
        userId = ircParameters[0].substring(tagToFindIndex + tagToFindLength + 1);
        int32_t userIdIndex = userId.indexOf(";");
        userId = userId.substring(0, userIdIndex);
      }

      // Get a custom reward ID
      tagToFind = "custom-reward-id";
      tagToFindLength = tagToFind.length();
      tagToFindIndex = ircParameters[0].indexOf(tagToFind);
      //
      if (tagToFindIndex < 0) {
        customRewardId = "";
      }
      if (tagToFindIndex >= 0) {
        customRewardId = ircParameters[0].substring(tagToFindIndex + tagToFindLength + 1);
        int32_t customRewardIdIndex = customRewardId.indexOf(";");
        customRewardId = customRewardId.substring(0, customRewardIdIndex);
      }
      String lowerCaseDisplayName = displayName;
      lowerCaseDisplayName.toLowerCase();
      if (userName == lowerCaseDisplayName) {
        //Serial.println("We got a fancy display name we can use with capital letters and all that shit");
        usernameToPing = displayName;
      }
      if (userName != lowerCaseDisplayName) {
        //Serial.println("We got a display name we can't use, it either is empty or it contains foreign characters, like Japanese characters");
        usernameToPing = userName;
      }
      //Serial.print("usernameToPing = ");
      //Serial.println(usernameToPing);
      String messageWord0 = "";
      int32_t firstSpaceIndex = chatMessage.indexOf(" ");
      if (firstSpaceIndex < 0) {
        //Serial.println("This message does not have any spaces");
        messageWord0 = chatMessage;
      }
      if (firstSpaceIndex >= 0) {
        //Serial.println("This message has at least one space");
        messageWord0 = chatMessage.substring(0, firstSpaceIndex);
      }
      String messageWord0Copy = messageWord0;
      messageWord0Copy.toLowerCase();
      if (roomId == debugRoomId) {
        if (userId == adminUserId) {
          if (messageWord0Copy == "!uptime" || messageWord0Copy == "!stats" || messageWord0Copy == "!status" || messageWord0Copy == "!info" || messageWord0Copy == "!debug" || messageWord0Copy == "!information") {
            redLevel = EEPROM.read(0);
            greenLevel = EEPROM.read(1);
            blueLevel =  EEPROM.read(2);

            counter1StepsToIncrement = uint32_t(EEPROM.read(7) << 24) | uint32_t(EEPROM.read(6) << 16) | uint32_t(EEPROM.read(5) << 8) | uint32_t(EEPROM.read(4));
            counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
            totalMessagesSent = uint32_t(EEPROM.read(15) << 24) | uint32_t(EEPROM.read(14) << 16) | uint32_t(EEPROM.read(13) << 8) | uint32_t(EEPROM.read(12));
            totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));

            currentSeconds = uint32_t(EEPROM.read(35) << 24) | uint32_t(EEPROM.read(34) << 16) | uint32_t(EEPROM.read(33) << 8) | uint32_t(EEPROM.read(32));
            currentMinutes = uint32_t(EEPROM.read(39) << 24) | uint32_t(EEPROM.read(38) << 16) | uint32_t(EEPROM.read(37) << 8) | uint32_t(EEPROM.read(36));
            currentHours = uint32_t(EEPROM.read(43) << 24) | uint32_t(EEPROM.read(42) << 16) | uint32_t(EEPROM.read(41) << 8) | uint32_t(EEPROM.read(40));
            currentDays = uint32_t(EEPROM.read(47) << 24) | uint32_t(EEPROM.read(46) << 16) | uint32_t(EEPROM.read(45) << 8) | uint32_t(EEPROM.read(44));

            totalSeconds = uint32_t(EEPROM.read(51) << 24) | uint32_t(EEPROM.read(50) << 16) | uint32_t(EEPROM.read(49) << 8) | uint32_t(EEPROM.read(48));
            totalMinutes = uint32_t(EEPROM.read(55) << 24) | uint32_t(EEPROM.read(54) << 16) | uint32_t(EEPROM.read(53) << 8) | uint32_t(EEPROM.read(52));
            totalHours = uint32_t(EEPROM.read(59) << 24) | uint32_t(EEPROM.read(58) << 16) | uint32_t(EEPROM.read(57) << 8) | uint32_t(EEPROM.read(56));
            totalDays = uint32_t(EEPROM.read(63) << 24) | uint32_t(EEPROM.read(62) << 16) | uint32_t(EEPROM.read(61) << 8) | uint32_t(EEPROM.read(60));
            uptimeRequestedCounter = uint32_t(EEPROM.read(67) << 24) | uint32_t(EEPROM.read(66) << 16) | uint32_t(EEPROM.read(65) << 8) | uint32_t(EEPROM.read(64));

            bootCounter = uint32_t(EEPROM.read(23) << 24) | uint32_t(EEPROM.read(22) << 16) | uint32_t(EEPROM.read(21) << 8) | uint32_t(EEPROM.read(20));
            wifiDisconnectCounter = uint32_t(EEPROM.read(27) << 24) | uint32_t(EEPROM.read(26) << 16) | uint32_t(EEPROM.read(25) << 8) | uint32_t(EEPROM.read(24));
            ircDisconnectCounter = uint32_t(EEPROM.read(31) << 24) | uint32_t(EEPROM.read(30) << 16) | uint32_t(EEPROM.read(29) << 8) | uint32_t(EEPROM.read(28));

            uptimeRequestedCounter++;
            EEPROM.write(64, uint8_t((uptimeRequestedCounter) & 0xFF));
            EEPROM.write(65, uint8_t((uptimeRequestedCounter >> 8) & 0xFF));
            EEPROM.write(66, uint8_t((uptimeRequestedCounter >> 16) & 0xFF));
            EEPROM.write(67, uint8_t((uptimeRequestedCounter >> 24) & 0xFF));
            EEPROM.commit();
            uptimeRequestedCounter = uint32_t(EEPROM.read(67) << 24) | uint32_t(EEPROM.read(66) << 16) | uint32_t(EEPROM.read(65) << 8) | uint32_t(EEPROM.read(64));

            uint32_t uptimeMillisTotal = millis();
            uint32_t uptimeDays = (uptimeMillisTotal / 86400000);
            uint32_t uptimeHours = (uptimeMillisTotal / 3600000) % 24;
            uint32_t uptimeMinutes = (uptimeMillisTotal / 60000) % 60;
            uint32_t uptimeSeconds = (uptimeMillisTotal / 1000) % 60;
            uint32_t uptimeMillis = (uptimeMillisTotal % 1000);
            String uptimeStringA = String(uptimeDays) + "d " + String(uptimeHours) + "h " + String(uptimeMinutes) + "m " + String(uptimeSeconds) + "s " + String(uptimeMillis) + "ms , " + String(uptimeMillisTotal) + " total ms";
            String uptimeStringB = String(currentDays) + "d " + String(currentHours) + "h " + String(currentMinutes) + "m " + String(currentSeconds) + "s";
            String uptimeStringC = String(totalDays) + "d " + String(totalHours) + "h " + String(totalMinutes) + "m " + String(totalSeconds) + "s";
            //client.sendAction(debugTwitchChannel, "Connected! PogChamp bootCounter = " + String(bootCounter) + " , wifiDisconnectCounter = " + String(wifiDisconnectCounter) + " , ircDisconnectCounter = " + String(ircDisconnectCounter) + " , uptimeRequestedCounter = " + String(uptimeRequestedCounter) + " , uptimeA = " + uptimeStringA + " , uptimeB = " + uptimeStringB + " , uptimeC = " + uptimeStringC);
            //client.sendNormalReply(channelName, "@" + usernameToPing + " Test", messageId);
            client.sendNormalReply(channelName, "@" + usernameToPing + " bootCounter = " + String(bootCounter) + " , wifiDisconnectCounter = " + String(wifiDisconnectCounter) + " , ircDisconnectCounter = " + String(ircDisconnectCounter) + " , uptimeRequestedCounter = " + String(uptimeRequestedCounter) + " , uptimeA = " + uptimeStringA + " , uptimeB = " + uptimeStringB + " , uptimeC = " + uptimeStringC + " , totalMessagesSent = " + String(totalMessagesSent) + " , totalLedTapeRewardsRedeemed = " + String(totalLedTapeRewardsRedeemed), messageId);
          }
        }
      }
      messageWord0Copy.replace("!", "");
      if (roomId == roomIdToListenTo) {
        //Serial.println("WRITING THE NUMBER TO EEPROM");
        counter1StepsToIncrement = uint32_t(EEPROM.read(7) << 24) | uint32_t(EEPROM.read(6) << 16) | uint32_t(EEPROM.read(5) << 8) | uint32_t(EEPROM.read(4));
        totalMessagesSent = uint32_t(EEPROM.read(15) << 24) | uint32_t(EEPROM.read(14) << 16) | uint32_t(EEPROM.read(13) << 8) | uint32_t(EEPROM.read(12));
        //Serial.print("totalMessagesSent = ");
        //Serial.println(totalMessagesSent);
        //Serial.println(counter1StepsToIncrement, DEC);
        //Serial.println(counter1StepsToIncrement, HEX);
        counter1StepsToIncrement++;
        totalMessagesSent++;
        EEPROM.write(4, uint8_t((counter1StepsToIncrement) & 0xFF));
        EEPROM.write(5, uint8_t((counter1StepsToIncrement >> 8) & 0xFF));
        EEPROM.write(6, uint8_t((counter1StepsToIncrement >> 16) & 0xFF));
        EEPROM.write(7, uint8_t((counter1StepsToIncrement >> 24) & 0xFF));

        EEPROM.write(12, uint8_t((totalMessagesSent) & 0xFF));
        EEPROM.write(13, uint8_t((totalMessagesSent >> 8) & 0xFF));
        EEPROM.write(14, uint8_t((totalMessagesSent >> 16) & 0xFF));
        EEPROM.write(15, uint8_t((totalMessagesSent >> 24) & 0xFF));
        EEPROM.commit();
        /*
          Serial.print(EEPROM.read(7), HEX);
          Serial.print(EEPROM.read(6), HEX);
          Serial.print(EEPROM.read(5), HEX);
          Serial.println(EEPROM.read(4), HEX);
          Serial.println("INVERTED 1");
          Serial.print(EEPROM.read(4), HEX);
          Serial.print(EEPROM.read(5), HEX);
          Serial.print(EEPROM.read(6), HEX);
          Serial.println(EEPROM.read(7), HEX);
        */
        counter1StepsToIncrement = uint32_t(EEPROM.read(7) << 24) | uint32_t(EEPROM.read(6) << 16) | uint32_t(EEPROM.read(5) << 8) | uint32_t(EEPROM.read(4));
        totalMessagesSent = uint32_t(EEPROM.read(15) << 24) | uint32_t(EEPROM.read(14) << 16) | uint32_t(EEPROM.read(13) << 8) | uint32_t(EEPROM.read(12));
        /*
          Serial.print("totalMessagesSent = ");
          Serial.println(totalMessagesSent);
          Serial.println(counter1StepsToIncrement, DEC);
          Serial.println(counter1StepsToIncrement, HEX);
          Serial.println("NUMBER WRITTEN TO EEPROM");
          Serial.println(customRewardId);
          Serial.println(userId);
          Serial.println(roomId);
          Serial.println(messageId);
          Serial.println(displayName);
          Serial.println(usernameToPing);
          Serial.print(channelName);
          Serial.print(" ");a
          Serial.print(userName);
          Serial.print(": ");
          Serial.println(chatMessage);
        */
        //messageWord0Copy.replace("!", "");
        if (customRewardId == customRewardIdToRead) {
          //String messageWord0Copy = messageWord0;
          //messageWord0Copy.toLowerCase();
          if (messageWord0Copy.startsWith("rainbow1") == true || messageWord0Copy.startsWith("flashbang") == true || messageWord0Copy.startsWith("rainbow2") == true || messageWord0Copy.startsWith("rainbow3") == true || messageWord0Copy.startsWith("rainbow4") == true || messageWord0Copy.startsWith("rainbow5") == true || messageWord0Copy.startsWith("debug") == true) {
            Serial.println("This is not a solid color");
            if (messageWord0Copy.startsWith("rainbow1") == true) {
              // rainbow1 animation
              Serial.println("RAINBOW1 ANIMATION");
              ledAnimationMode = EEPROM.read(69);
              ledAnimationMode = 1;
              EEPROM.write(69, ledAnimationMode);
              EEPROM.commit();
              ledAnimationMode = EEPROM.read(69);
              rainbowValueRed = 255;
              rainbowValueGreen = 0;
              rainbowValueBlue = 0;
              if (ledTapeMode == 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow1 appear!", messageId);
              }
              if (ledTapeMode != 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow1 appear, but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
              }
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
              //Serial.print("totalLedTapeRewardsRedeemed = ");
              //Serial.println(totalLedTapeRewardsRedeemed);
              //Serial.println(counter2StepsToIncrement, DEC);
              //Serial.println(counter2StepsToIncrement, HEX);
              counter2StepsToIncrement++;
              totalLedTapeRewardsRedeemed++;
              EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
              EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
              EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
              EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));

              EEPROM.write(16, uint8_t((totalLedTapeRewardsRedeemed) & 0xFF));
              EEPROM.write(17, uint8_t((totalLedTapeRewardsRedeemed >> 8) & 0xFF));
              EEPROM.write(18, uint8_t((totalLedTapeRewardsRedeemed >> 16) & 0xFF));
              EEPROM.write(19, uint8_t((totalLedTapeRewardsRedeemed >> 24) & 0xFF));
              EEPROM.commit();
              /*
                Serial.print(EEPROM.read(11), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.println(EEPROM.read(8), HEX);
                Serial.println("INVERTED 1");
                Serial.print(EEPROM.read(8), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.println(EEPROM.read(11), HEX);
              */
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
            }
            if (messageWord0Copy.startsWith("flashbang") == true) {
              // Flashbang animation
              Serial.println("FLASHBANG ANIMATION");
              startFlashbangAnimation = true;
              flashbangValue = 255;
              ledAnimationMode = EEPROM.read(69);
              ledAnimationMode = 2;
              EEPROM.write(69, ledAnimationMode);
              EEPROM.commit();
              ledAnimationMode = EEPROM.read(69);
              if (ledTapeMode == 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has thrown a flashbang, guard your eyes!", messageId);
              }
              if (ledTapeMode != 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has thrown a flashbang, guard your eyes, but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
              }
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
              //Serial.print("totalLedTapeRewardsRedeemed = ");
              //Serial.println(totalLedTapeRewardsRedeemed);
              //Serial.println(counter2StepsToIncrement, DEC);
              //Serial.println(counter2StepsToIncrement, HEX);
              counter2StepsToIncrement++;
              totalLedTapeRewardsRedeemed++;
              EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
              EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
              EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
              EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));

              EEPROM.write(16, uint8_t((totalLedTapeRewardsRedeemed) & 0xFF));
              EEPROM.write(17, uint8_t((totalLedTapeRewardsRedeemed >> 8) & 0xFF));
              EEPROM.write(18, uint8_t((totalLedTapeRewardsRedeemed >> 16) & 0xFF));
              EEPROM.write(19, uint8_t((totalLedTapeRewardsRedeemed >> 24) & 0xFF));
              EEPROM.commit();
              /*
                Serial.print(EEPROM.read(11), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.println(EEPROM.read(8), HEX);
                Serial.println("INVERTED 1");
                Serial.print(EEPROM.read(8), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.println(EEPROM.read(11), HEX);
              */
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
            }
            if (messageWord0Copy.startsWith("rainbow2") == true) {
              // rainbow2 animation
              // (r)ed (y)ellow (w)hite (c)yan (b)lue blac(k)
              //Serial.println("TEST B");
              Serial.println("RAINBOW2 ANIMATION");
              ledAnimationMode = EEPROM.read(69);
              ledAnimationMode = 3;
              EEPROM.write(69, ledAnimationMode);
              EEPROM.commit();
              ledAnimationMode = EEPROM.read(69);
              rainbowValueRed = 255;
              rainbowValueGreen = 0;
              rainbowValueBlue = 0;
              if (ledTapeMode == 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow2 appear!", messageId);
              }
              if (ledTapeMode != 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow2 appear, but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
              }
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
              //Serial.print("totalLedTapeRewardsRedeemed = ");
              //Serial.println(totalLedTapeRewardsRedeemed);
              //Serial.println(counter2StepsToIncrement, DEC);
              //Serial.println(counter2StepsToIncrement, HEX);
              counter2StepsToIncrement++;
              totalLedTapeRewardsRedeemed++;
              EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
              EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
              EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
              EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));

              EEPROM.write(16, uint8_t((totalLedTapeRewardsRedeemed) & 0xFF));
              EEPROM.write(17, uint8_t((totalLedTapeRewardsRedeemed >> 8) & 0xFF));
              EEPROM.write(18, uint8_t((totalLedTapeRewardsRedeemed >> 16) & 0xFF));
              EEPROM.write(19, uint8_t((totalLedTapeRewardsRedeemed >> 24) & 0xFF));
              EEPROM.commit();
              /*
                Serial.print(EEPROM.read(11), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.println(EEPROM.read(8), HEX);
                Serial.println("INVERTED 1");
                Serial.print(EEPROM.read(8), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.println(EEPROM.read(11), HEX);
              */
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
            }
            if (messageWord0Copy.startsWith("rainbow3") == true) {
              // rainbow3 animation
              //Serial.println("TEST B");
              Serial.println("RAINBOW3 ANIMATION");
              ledAnimationMode = EEPROM.read(69);
              ledAnimationMode = 4;
              EEPROM.write(69, ledAnimationMode);
              EEPROM.commit();
              ledAnimationMode = EEPROM.read(69);
              rainbowValueRed = 1;
              rainbowValueGreen = 0;
              rainbowValueBlue = 0;
              if (ledTapeMode == 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow3 appear!", messageId);
              }
              if (ledTapeMode != 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow3 appear, but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
              }
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
              //Serial.print("totalLedTapeRewardsRedeemed = ");
              //Serial.println(totalLedTapeRewardsRedeemed);
              //Serial.println(counter2StepsToIncrement, DEC);
              //Serial.println(counter2StepsToIncrement, HEX);
              counter2StepsToIncrement++;
              totalLedTapeRewardsRedeemed++;
              EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
              EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
              EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
              EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));

              EEPROM.write(16, uint8_t((totalLedTapeRewardsRedeemed) & 0xFF));
              EEPROM.write(17, uint8_t((totalLedTapeRewardsRedeemed >> 8) & 0xFF));
              EEPROM.write(18, uint8_t((totalLedTapeRewardsRedeemed >> 16) & 0xFF));
              EEPROM.write(19, uint8_t((totalLedTapeRewardsRedeemed >> 24) & 0xFF));
              EEPROM.commit();
              /*
                Serial.print(EEPROM.read(11), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.println(EEPROM.read(8), HEX);
                Serial.println("INVERTED 1");
                Serial.print(EEPROM.read(8), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.println(EEPROM.read(11), HEX);
              */
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
            }
            if (messageWord0Copy.startsWith("rainbow4") == true) {
              // rainbow4 animation
              //Serial.println("TEST B");
              Serial.println("RAINBOW4 ANIMATION");
              ledAnimationMode = EEPROM.read(69);
              ledAnimationMode = 5;
              EEPROM.write(69, ledAnimationMode);
              EEPROM.commit();
              ledAnimationMode = EEPROM.read(69);
              rainbowValueRed = 255;
              rainbowValueGreen = 0;
              rainbowValueBlue = 0;
              if (ledTapeMode == 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow4 appear!", messageId);
              }
              if (ledTapeMode != 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow4 appear, but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
              }
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
              //Serial.print("totalLedTapeRewardsRedeemed = ");
              //Serial.println(totalLedTapeRewardsRedeemed);
              //Serial.println(counter2StepsToIncrement, DEC);
              //Serial.println(counter2StepsToIncrement, HEX);
              counter2StepsToIncrement++;
              totalLedTapeRewardsRedeemed++;
              EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
              EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
              EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
              EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));

              EEPROM.write(16, uint8_t((totalLedTapeRewardsRedeemed) & 0xFF));
              EEPROM.write(17, uint8_t((totalLedTapeRewardsRedeemed >> 8) & 0xFF));
              EEPROM.write(18, uint8_t((totalLedTapeRewardsRedeemed >> 16) & 0xFF));
              EEPROM.write(19, uint8_t((totalLedTapeRewardsRedeemed >> 24) & 0xFF));
              EEPROM.commit();
              /*
                Serial.print(EEPROM.read(11), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.println(EEPROM.read(8), HEX);
                Serial.println("INVERTED 1");
                Serial.print(EEPROM.read(8), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.println(EEPROM.read(11), HEX);
              */
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
            }
            if (messageWord0Copy.startsWith("rainbow5") == true) {
              // rainbow4 animation
              //Serial.println("TEST B");
              Serial.println("RAINBOW5 ANIMATION");
              ledAnimationMode = EEPROM.read(69);
              ledAnimationMode = 6;
              EEPROM.write(69, ledAnimationMode);
              EEPROM.commit();
              ledAnimationMode = EEPROM.read(69);
              rainbowValueRed = 255;
              rainbowValueGreen = 0;
              rainbowValueBlue = 0;
              if (ledTapeMode == 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow5 appear!", messageId);
              }
              if (ledTapeMode != 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " has made a rainbow5 appear, but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
              }
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
              //Serial.print("totalLedTapeRewardsRedeemed = ");
              //Serial.println(totalLedTapeRewardsRedeemed);
              //Serial.println(counter2StepsToIncrement, DEC);
              //Serial.println(counter2StepsToIncrement, HEX);
              counter2StepsToIncrement++;
              totalLedTapeRewardsRedeemed++;
              EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
              EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
              EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
              EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));

              EEPROM.write(16, uint8_t((totalLedTapeRewardsRedeemed) & 0xFF));
              EEPROM.write(17, uint8_t((totalLedTapeRewardsRedeemed >> 8) & 0xFF));
              EEPROM.write(18, uint8_t((totalLedTapeRewardsRedeemed >> 16) & 0xFF));
              EEPROM.write(19, uint8_t((totalLedTapeRewardsRedeemed >> 24) & 0xFF));
              EEPROM.commit();
              /*
                Serial.print(EEPROM.read(11), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.println(EEPROM.read(8), HEX);
                Serial.println("INVERTED 1");
                Serial.print(EEPROM.read(8), HEX);
                Serial.print(EEPROM.read(9), HEX);
                Serial.print(EEPROM.read(10), HEX);
                Serial.println(EEPROM.read(11), HEX);
              */
              counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
              totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
            }
            if (messageWord0Copy.startsWith("debug") == true) {
              // Used for testing only!!!!!!!!!
              rainbowValueRed = 1;
              rainbowValueGreen = 0;
              rainbowValueBlue = 0;
              rainbowStepSolidCounter = 0;
              Serial.println("DEBUG ANIMATION");
              ledAnimationMode = EEPROM.read(69);
              ledAnimationMode = 255;
              EEPROM.write(69, ledAnimationMode);
              EEPROM.commit();
              ledAnimationMode = EEPROM.read(69);
              if (ledTapeMode == 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " DEBUG", messageId);
              }
              if (ledTapeMode != 0) {
                client.sendNormalReply(channelName, "@" + usernameToPing + " DEBUG, but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
              }
            }
          }
          if (messageWord0Copy.startsWith("rainbow1") == false && messageWord0Copy.startsWith("flashbang") == false && messageWord0Copy.startsWith("rainbow2") == false && messageWord0Copy.startsWith("rainbow3") == false && messageWord0Copy.startsWith("rainbow4") == false && messageWord0Copy.startsWith("rainbow5") == false && messageWord0Copy.startsWith("debug") == false) {
            Serial.println("This IS a solid color");
            /*
              ledAnimationMode = EEPROM.read(69);
              ledAnimationMode = 0;
              EEPROM.write(69, ledAnimationMode);
              EEPROM.commit();
              ledAnimationMode = EEPROM.read(69);
            */
            if (messageWord0Copy.startsWith("0x") == true || messageWord0Copy.startsWith("0X") == true || messageWord0Copy.startsWith("#") == true) {
              Serial.println("This might be a hex number");
              String hexString = messageWord0Copy;
              hexString.replace("\\s", "");
              //hexString.replace("\s", "");
              hexString.replace(" ", "");
              hexString.replace("0x", "");
              hexString.replace("0X", "");
              hexString.replace("0l", "");
              hexString.replace("0L", "");
              hexString.replace("#", "");
              hexString.replace(",", "");
              //Serial.print("hexString.length() = ");
              //Serial.println(hexString.length());
              if (hexString.length() >= 6) {
                Serial.println("Valid hex integer length?");
                String hexStrings[3] = {"", "", ""};
                int32_t hexStringsToInt[3] = { -1, -1, -1};
                String hexIntsBackToHex[3] = {"", "", ""};
                bool areThereInvalidHexColors = false;
                bool areThereInvalidHexColors2 = false;
                /*
                  String hexStrings[3] = {hexString.substring(0, 2), hexString.substring(2, 4), hexString.substring(4, 6)};
                  int32_t hexStringsToInt[3] = {strtol(&hexStrings[0][0], NULL, 16), strtol(&hexStrings[1][0], NULL, 16), strtol(&hexStrings[2][0], NULL, 16)};
                  String hexIntsBackToHex[3] = {String(hexStringsToInt[0], HEX), String(hexStringsToInt[1], HEX), String(hexStringsToInt[2], HEX)};
                */
                uint32_t hexColorsIndexTotal = 3;
                for (unsigned int hexColorsIndex = 0; hexColorsIndex < hexColorsIndexTotal; hexColorsIndex++) {
                  uint32_t startingIndex = hexColorsIndex * 2;
                  uint32_t endingIndex = startingIndex + 2;
                  hexStrings[hexColorsIndex] = hexString.substring(startingIndex, endingIndex);
                  /*
                    Serial.print("hexColorsIndex = ");
                    Serial.println(hexColorsIndex);
                    Serial.print("startingIndex = ");
                    Serial.println(startingIndex);
                    Serial.print("endingIndex = ");
                    Serial.println(endingIndex);
                    Serial.print("hexStrings[hexColorsIndex] = ");
                    Serial.println(hexStrings[hexColorsIndex]);
                  */
                  hexStringsToInt[hexColorsIndex] = strtol(&hexStrings[hexColorsIndex][0], NULL, 16);
                  /*
                    Serial.print("hexStringsToInt[hexColorsIndex] = ");
                    Serial.println(hexStringsToInt[hexColorsIndex]);
                  */
                  hexIntsBackToHex[hexColorsIndex] = String(hexStringsToInt[hexColorsIndex], HEX);
                  if (hexIntsBackToHex[hexColorsIndex].length() < 2) {
                    hexIntsBackToHex[hexColorsIndex] = "0" + hexIntsBackToHex[hexColorsIndex];
                  }
                  /*
                    Serial.print("hexIntsBackToHex[hexColorsIndex] = ");
                    Serial.println(hexIntsBackToHex[hexColorsIndex]);
                  */
                  if (hexStrings[hexColorsIndex] != hexIntsBackToHex[hexColorsIndex]) {
                    // Invalid color
                    Serial.println("Invalid hex color detected, ignoring");
                    areThereInvalidHexColors = true;
                  }
                  if (hexStrings[hexColorsIndex] == hexIntsBackToHex[hexColorsIndex]) {
                    // Valid color
                    Serial.println("This is a valid hex color");
                  }
                }
                if (areThereInvalidHexColors == true) {
                  Serial.println("INVALID HEX COLORS 1");
                  client.sendNormalReply(channelName, "@" + usernameToPing + " Invalid colors!", messageId);
                  // Invalid colors found, warn user about it
                }
                if (areThereInvalidHexColors == false) {
                  Serial.println("The hex colors found so far are still valid");
                  for (unsigned int hexColorsIndex = 0; hexColorsIndex < hexColorsIndexTotal; hexColorsIndex++) {
                    if (hexStringsToInt[hexColorsIndex] < 0 || hexStringsToInt[hexColorsIndex] > 255) {
                      // Invalid
                      Serial.println("Hex color is out of valid range");
                      areThereInvalidHexColors2 = true;
                    }
                    if (hexStringsToInt[hexColorsIndex] >= 0 || hexStringsToInt[hexColorsIndex] <= 255) {
                      // Valid
                      Serial.println("Hex color is within valid range");
                    }
                  }
                  if (areThereInvalidHexColors2 == true) {
                    Serial.println("INVALID HEX COLORS 2");
                    client.sendNormalReply(channelName, "@" + usernameToPing + " Invalid colors!", messageId);
                    // Still found invalid colors, warn user about it
                  }
                  if (areThereInvalidHexColors2 == false) {
                    Serial.println("The hex colors found still valid, nice");
                    // Valid colors found, parse this
                    // Parse colors
                    ledAnimationMode = EEPROM.read(69);
                    ledAnimationMode = 0;
                    EEPROM.write(69, ledAnimationMode);
                    EEPROM.commit();
                    ledAnimationMode = EEPROM.read(69);
                    Serial.println("LETS FUCKING GOOOOOOOOOOOOOOOOO");
                    if (ledTapeMode == 0) {
                      redLevel = uint8_t(hexStringsToInt[0]);
                      greenLevel = uint8_t(hexStringsToInt[1]);
                      blueLevel = uint8_t(hexStringsToInt[2]);
                      EEPROM.write(0, redLevel);
                      EEPROM.write(1, greenLevel);
                      EEPROM.write(2, blueLevel);
                      EEPROM.commit();
                      redLevel = EEPROM.read(0);
                      greenLevel = EEPROM.read(1);
                      blueLevel =  EEPROM.read(2);
                      analogWrite(rgbRed, redLevel);
                      analogWrite(rgbGreen, greenLevel);
                      analogWrite(rgbBlue, blueLevel);
                      if (client.connected()) {
                        //client.sendAction(channelName, "@" + usernameToPing + " changed LED tape colors to " + String(redLevel) + "," + String(greenLevel) + "," + String(blueLevel) + "!");
                        client.sendNormalReply(channelName, "@" + usernameToPing + " changed LED tape colors to " + String(redLevel) + "," + String(greenLevel) + "," + String(blueLevel) + "!", messageId);
                      }
                    }
                    if (ledTapeMode != 0) {
                      redLevel = uint8_t(hexStringsToInt[0]);
                      greenLevel = uint8_t(hexStringsToInt[1]);
                      blueLevel = uint8_t(hexStringsToInt[2]);
                      EEPROM.write(0, redLevel);
                      EEPROM.write(1, greenLevel);
                      EEPROM.write(2, blueLevel);
                      EEPROM.commit();
                      redLevel = EEPROM.read(0);
                      greenLevel = EEPROM.read(1);
                      blueLevel =  EEPROM.read(2);
                      if (client.connected()) {
                        //client.sendAction(channelName, "@" + usernameToPing + " changed LED tape colors to " + String(redLevel) + "," + String(greenLevel) + "," + String(blueLevel) + ", but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!");
                        client.sendNormalReply(channelName, "@" + usernameToPing + " changed LED tape colors to " + String(redLevel) + "," + String(greenLevel) + "," + String(blueLevel) + ", but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
                      }
                    }
                    //Serial.println("WRITING THE NUMBER TO EEPROM");
                    counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
                    totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
                    //Serial.print("totalLedTapeRewardsRedeemed = ");
                    //Serial.println(totalLedTapeRewardsRedeemed);
                    //Serial.println(counter2StepsToIncrement, DEC);
                    //Serial.println(counter2StepsToIncrement, HEX);
                    counter2StepsToIncrement++;
                    totalLedTapeRewardsRedeemed++;
                    EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
                    EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
                    EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
                    EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));

                    EEPROM.write(16, uint8_t((totalLedTapeRewardsRedeemed) & 0xFF));
                    EEPROM.write(17, uint8_t((totalLedTapeRewardsRedeemed >> 8) & 0xFF));
                    EEPROM.write(18, uint8_t((totalLedTapeRewardsRedeemed >> 16) & 0xFF));
                    EEPROM.write(19, uint8_t((totalLedTapeRewardsRedeemed >> 24) & 0xFF));
                    EEPROM.commit();
                    /*
                      Serial.print(EEPROM.read(11), HEX);
                      Serial.print(EEPROM.read(10), HEX);
                      Serial.print(EEPROM.read(9), HEX);
                      Serial.println(EEPROM.read(8), HEX);
                      Serial.println("INVERTED 1");
                      Serial.print(EEPROM.read(8), HEX);
                      Serial.print(EEPROM.read(9), HEX);
                      Serial.print(EEPROM.read(10), HEX);
                      Serial.println(EEPROM.read(11), HEX);
                    */
                    counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
                    totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
                    /*
                      Serial.print("totalLedTapeRewardsRedeemed = ");
                      Serial.println(totalLedTapeRewardsRedeemed);
                      Serial.println(counter2StepsToIncrement, DEC);
                      Serial.println(counter2StepsToIncrement, HEX);
                      Serial.println("NUMBER WRITTEN TO EEPROM");
                    */
                  }
                }
                //Serial.println(hexIntsBackToHex[0]);
                //Serial.println(hexIntsBackToHex[1]);
                //Serial.println(hexIntsBackToHex[2]);
                //Serial.println("Valid hex integer length?");
                //uint32_t hexInt = strtoul(&hexString[0], NULL, 16);
                //Serial.println(hexString);
                //Serial.println(hexInt);
              }
              if (hexString.length() < 6) {
                Serial.println("HEX STRING TOO SHORT!!!!!!!!!");
                client.sendNormalReply(channelName, "@" + usernameToPing + " Invalid colors!", messageId);
              }
            }
            if (messageWord0Copy.startsWith("0x") == false && messageWord0Copy.startsWith("0X") == false && messageWord0Copy.startsWith("#") == false) {
              Serial.println("NOT A HEX NUMBER!!!!!!!!!!!!!!!!!!!!!!!!!!!");
              String ledTapeColorsToSet[3] = {"", "", ""};
              int32_t ledTapeColorsToInt[3] = { -1, -1, -1};
              String ledTapeColorsBackToString[3] = {"", "", ""};
              String colorsTestString = messageWord0Copy;
              String colorsTestString2 = "";
              bool areThereInvalidColors = false;
              bool areThereInvalidColors2 = false;
              uint32_t colorsIndexTotal = 3;
              for (uint32_t colorsIndex = 0; colorsIndex < colorsIndexTotal; colorsIndex++) {
                int32_t commaIndex = colorsTestString.indexOf(",");
                if (colorsIndex < colorsIndexTotal - 1) {
                  if (commaIndex < 0) {
                    ledTapeColorsToSet[colorsIndex] = "INVALID COLORS";
                  }
                  if (commaIndex >= 0) {
                    colorsTestString2 = colorsTestString.substring(0, commaIndex);
                    colorsTestString = colorsTestString.substring(commaIndex + 1);
                    ledTapeColorsToSet[colorsIndex] = colorsTestString2;
                  }
                }
                if (colorsIndex >= colorsIndexTotal - 1) {
                  if (commaIndex < 0) {
                    ledTapeColorsToSet[colorsIndex] = colorsTestString;
                  }
                  if (commaIndex >= 0) {
                    colorsTestString2 = colorsTestString.substring(0, commaIndex);
                    colorsTestString = colorsTestString.substring(commaIndex + 1);
                    ledTapeColorsToSet[colorsIndex] = colorsTestString2;
                  }
                }
                if (ledTapeColorsToSet[colorsIndex] == "INVALID COLORS" || ledTapeColorsToSet[colorsIndex] == "") {
                  areThereInvalidColors = true;
                }
              }
              if (areThereInvalidColors == true) {
                Serial.println("INVALID COLORS FOUND, IGNORING");
                // Try to parse hex colors here?
                if (client.connected()) {
                  //client.sendAction(channelName, "@" + usernameToPing + " Invalid colors!");
                  //client.sendActionReply(channelName, "Invalid colors!", messageId);
                  //client.sendActionReply(channelName, "@" + usernameToPing + " Invalid colors!", messageId);
                  client.sendNormalReply(channelName, "@" + usernameToPing + " Invalid colors!", messageId);
                }
              }
              if (areThereInvalidColors == false) {
                Serial.println("THE COLORS ARE VALID UP UNTIL THIS POINT, NEEDS FURTHER TESTING");
                for (unsigned int colorsIndex = 0; colorsIndex < colorsIndexTotal; colorsIndex++) {
                  ledTapeColorsToInt[colorsIndex] = ledTapeColorsToSet[colorsIndex].toInt();
                  ledTapeColorsBackToString[colorsIndex] = String(ledTapeColorsToInt[colorsIndex]);
                  Serial.println("BEFORE");
                  Serial.println(ledTapeColorsToSet[colorsIndex]);
                  Serial.println(ledTapeColorsToInt[colorsIndex]);
                  Serial.println(ledTapeColorsBackToString[colorsIndex]);
                  if (ledTapeColorsToSet[colorsIndex].startsWith("0")) {
                    bool checkForLeadingZeroes = true;
                    uint32_t leadingZeroesToRemove = 0;
                    for (unsigned int stringIndex = 0; stringIndex < ledTapeColorsToSet[colorsIndex].length(); stringIndex++) {
                      if (checkForLeadingZeroes == true) {
                        if (ledTapeColorsToSet[colorsIndex][stringIndex] != '0') {
                          //Serial.print("Stopping at ");
                          //Serial.println(stringIndex);
                          leadingZeroesToRemove = stringIndex;
                          checkForLeadingZeroes = false;
                        }
                      }
                    }
                    if (checkForLeadingZeroes == false) {
                      if (leadingZeroesToRemove > 0) {
                        ledTapeColorsToSet[colorsIndex].remove(0, leadingZeroesToRemove);
                      }
                    }
                  }
                  ledTapeColorsToInt[colorsIndex] = ledTapeColorsToSet[colorsIndex].toInt();
                  ledTapeColorsBackToString[colorsIndex] = String(ledTapeColorsToInt[colorsIndex]);
                  /*
                    Serial.println("AFTER");
                    Serial.println(ledTapeColorsToSet[colorsIndex]);
                    Serial.println(ledTapeColorsToInt[colorsIndex]);
                    Serial.println(ledTapeColorsBackToString[colorsIndex]);
                  */
                  if (ledTapeColorsToSet[colorsIndex] != ledTapeColorsBackToString[colorsIndex]) {
                    Serial.println("INValid color?");
                    areThereInvalidColors2 = true;
                  }
                  if (ledTapeColorsToSet[colorsIndex] == ledTapeColorsBackToString[colorsIndex]) {
                    Serial.println("Valid color?");
                    if (ledTapeColorsToInt[colorsIndex] < 0 || ledTapeColorsToInt[colorsIndex] > 255) {
                      /*
                        Serial.println("Color value out of valid range");
                        Serial.println(colorsIndex);
                        //Serial.println("AFTER");
                        Serial.println(ledTapeColorsToSet[colorsIndex]);
                        Serial.println(ledTapeColorsToInt[colorsIndex]);
                        Serial.println(ledTapeColorsBackToString[colorsIndex]);
                      */
                      areThereInvalidColors2 = true;
                    }
                    /*
                      if (ledTapeColorsToInt[colorsIndex] >= 0 && ledTapeColorsToInt[colorsIndex] <= 255) {
                      Serial.println("Color value is valid");
                      Serial.println(colorsIndex);
                      //Serial.println("AFTER");
                      Serial.println(ledTapeColorsToSet[colorsIndex]);
                      Serial.println(ledTapeColorsToInt[colorsIndex]);
                      Serial.println(ledTapeColorsBackToString[colorsIndex]);
                      }
                    */
                  }
                }
                if (areThereInvalidColors2 == true) {
                  // Dont parse colors, tell user the colors are invalid
                  Serial.println("INVALID COLORS YOU DUMBASS");
                  if (client.connected()) {
                    //client.sendAction(channelName, "@" + usernameToPing + " Invalid colors!");
                    client.sendNormalReply(channelName, "@" + usernameToPing + " Invalid colors!", messageId);
                  }
                }
                if (areThereInvalidColors2 == false) {
                  // Parse colors
                  Serial.println("LETS FUCKING GOOOOOOOOOOOOOOOOO");
                  ledAnimationMode = EEPROM.read(69);
                  ledAnimationMode = 0;
                  EEPROM.write(69, ledAnimationMode);
                  EEPROM.commit();
                  ledAnimationMode = EEPROM.read(69);
                  if (ledTapeMode == 0) {
                    redLevel = ledTapeColorsToInt[0];
                    greenLevel = ledTapeColorsToInt[1];
                    blueLevel = ledTapeColorsToInt[2];
                    EEPROM.write(0, redLevel);
                    EEPROM.write(1, greenLevel);
                    EEPROM.write(2, blueLevel);
                    EEPROM.commit();
                    redLevel = EEPROM.read(0);
                    greenLevel = EEPROM.read(1);
                    blueLevel =  EEPROM.read(2);
                    analogWrite(rgbRed, redLevel);
                    analogWrite(rgbGreen, greenLevel);
                    analogWrite(rgbBlue, blueLevel);
                    if (client.connected()) {
                      //client.sendAction(channelName, "@" + usernameToPing + " changed LED tape colors to " + String(redLevel) + "," + String(greenLevel) + "," + String(blueLevel) + "!");
                      client.sendNormalReply(channelName, "@" + usernameToPing + " changed LED tape colors to " + String(redLevel) + "," + String(greenLevel) + "," + String(blueLevel) + "!", messageId);
                    }
                  }
                  if (ledTapeMode != 0) {
                    redLevel = ledTapeColorsToInt[0];
                    greenLevel = ledTapeColorsToInt[1];
                    blueLevel = ledTapeColorsToInt[2];
                    EEPROM.write(0, redLevel);
                    EEPROM.write(1, greenLevel);
                    EEPROM.write(2, blueLevel);
                    EEPROM.commit();
                    redLevel = EEPROM.read(0);
                    greenLevel = EEPROM.read(1);
                    blueLevel =  EEPROM.read(2);
                    if (client.connected()) {
                      //client.sendAction(channelName, "@" + usernameToPing + " changed LED tape colors to " + String(redLevel) + "," + String(greenLevel) + "," + String(blueLevel) + ", but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!");
                      client.sendNormalReply(channelName, "@" + usernameToPing + " changed LED tape colors to " + String(redLevel) + "," + String(greenLevel) + "," + String(blueLevel) + ", but @" + adminNameToPing + " LED Tape Mode (ledTapeMode = " + String(ledTapeMode) + ") is set incorrectly!", messageId);
                    }
                  }
                  //Serial.println("WRITING THE NUMBER TO EEPROM");
                  counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
                  totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
                  //Serial.print("totalLedTapeRewardsRedeemed = ");
                  //Serial.println(totalLedTapeRewardsRedeemed);
                  //Serial.println(counter2StepsToIncrement, DEC);
                  //Serial.println(counter2StepsToIncrement, HEX);
                  counter2StepsToIncrement++;
                  totalLedTapeRewardsRedeemed++;
                  EEPROM.write(8, uint8_t((counter2StepsToIncrement) & 0xFF));
                  EEPROM.write(9, uint8_t((counter2StepsToIncrement >> 8) & 0xFF));
                  EEPROM.write(10, uint8_t((counter2StepsToIncrement >> 16) & 0xFF));
                  EEPROM.write(11, uint8_t((counter2StepsToIncrement >> 24) & 0xFF));

                  EEPROM.write(16, uint8_t((totalLedTapeRewardsRedeemed) & 0xFF));
                  EEPROM.write(17, uint8_t((totalLedTapeRewardsRedeemed >> 8) & 0xFF));
                  EEPROM.write(18, uint8_t((totalLedTapeRewardsRedeemed >> 16) & 0xFF));
                  EEPROM.write(19, uint8_t((totalLedTapeRewardsRedeemed >> 24) & 0xFF));
                  EEPROM.commit();
                  /*
                    Serial.print(EEPROM.read(11), HEX);
                    Serial.print(EEPROM.read(10), HEX);
                    Serial.print(EEPROM.read(9), HEX);
                    Serial.println(EEPROM.read(8), HEX);
                    Serial.println("INVERTED 1");
                    Serial.print(EEPROM.read(8), HEX);
                    Serial.print(EEPROM.read(9), HEX);
                    Serial.print(EEPROM.read(10), HEX);
                    Serial.println(EEPROM.read(11), HEX);
                  */
                  counter2StepsToIncrement = uint32_t(EEPROM.read(11) << 24) | uint32_t(EEPROM.read(10) << 16) | uint32_t(EEPROM.read(9) << 8) | uint32_t(EEPROM.read(8));
                  totalLedTapeRewardsRedeemed = uint32_t(EEPROM.read(19) << 24) | uint32_t(EEPROM.read(18) << 16) | uint32_t(EEPROM.read(17) << 8) | uint32_t(EEPROM.read(16));
                  /*
                    Serial.print("totalLedTapeRewardsRedeemed = ");
                    Serial.println(totalLedTapeRewardsRedeemed);
                    Serial.println(counter2StepsToIncrement, DEC);
                    Serial.println(counter2StepsToIncrement, HEX);
                    Serial.println("NUMBER WRITTEN TO EEPROM");
                  */
                }
              }
            }
          }
        }
      }
    }
  }
}
