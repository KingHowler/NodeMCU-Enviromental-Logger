#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN D4  // SDA pin
#define RST_PIN D3 // Reset pin
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

WiFiClient client;
const char* ssid = "myWifi";        // Replace with your Wi-Fi SSID
const char* password = "myPassword"; // Replace with your Wi-Fi password

const char* host = "my-server.com";   // Replace with your server address

#define RedLight D0
#define YellowLight D1
#define GreenLight D2

bool newData;

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Timeout after 20 attempts
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to Wi-Fi");
    return;
  }
  Serial.println("\nConnected to Wi-Fi");
  
  SPI.begin();           // Initialize SPI bus
  mfrc522.PCD_Init();    // Initialize MFRC522
  Serial.println("Place your card to the reader...");
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
}

void loop() {
  newData = false;
  String resp;
  while (client.available()) {
    newData = true;
    resp = client.readStringUntil('\r');
  }
  if (newData == true) {
    // Close the connection
    client.stop();
    Serial.println("data detected");
  }
  resp = resp.substring(resp.length() - 2);
  if (resp == "NF") {
    Serial.println("Card not found");
    digitalWrite(RedLight, HIGH);
    digitalWrite(YellowLight, LOW);
    digitalWrite(GreenLight, LOW);
  }
  if (resp == "AP") {
    Serial.println("Already marked as Present");
    digitalWrite(RedLight, LOW);
    digitalWrite(YellowLight, HIGH);
    digitalWrite(GreenLight, LOW);
  }
  
  if (resp == "MP") {
    Serial.println("Marked as Present");
    digitalWrite(RedLight, LOW);
    digitalWrite(YellowLight, LOW);
    digitalWrite(GreenLight, HIGH);
  }

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  // Store the UID in a string in the format FF FF FF FF
  String uidString = "";
  String UIDtag = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidString += "0";  // Add leading zero for values less than 0x10
      UIDtag += "0";
    }
    uidString += String(mfrc522.uid.uidByte[i], HEX);  // Convert byte to hex string
    UIDtag += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) {
      uidString += "%20";  // Add space between bytes, but not after the last byte
      UIDtag += " ";  // Add space between bytes, but not after the last byte
    }
  }

  // Convert to uppercase for standard UID format
  uidString.toUpperCase();
  UIDtag.toUpperCase();

  // Print the UID string to Serial
  Serial.print("UID tag: ");
  Serial.println(UIDtag);
  MarkPresent(uidString);

  // Halt PICC to stop reading
  mfrc522.PICC_HaltA();
}

void MarkPresent(String Card) {
  // Connect to the server
  Serial.print("Connecting to ");
  Serial.print(host);
  if (!client.connect(host, 80)) { // Connect to server on port 80
    Serial.println(" - Connection to host failed");
  }
  Serial.println(" - Connected to host");

  String url = "/write.php?mode=mark-attendance&card=" + Card;

  // Send GET request
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "Connection: close\r\n\r\n");
}
