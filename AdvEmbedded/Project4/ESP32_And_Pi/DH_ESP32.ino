//#include <Arduino.h>
#include <WiFi.h>

// Setup WiFi
const char* ssid      = "SCinterCo";  // Modify your SSID
const char* password  = "bondbeng";   // Modify your password

// Set static IP address and gateway
IPAddress local_IP(192, 168, 1, 50); // Static IP
WiFiServer server(80);   // Web server on port 80

// Diffie-Hellman parameters
const long p = 23;  // Prime number
const long g = 5;   // Generator

// Global variables for time
struct tm timeinfo;

// Function to perform modular exponentiation: (base^exp) % mod
long modExp(long base, long exp, long mod) {
    long result = 1;
    base = base % mod;  // Ensure base is within mod

    while (exp > 0) {
        if (exp % 2 == 1) {  // If exp is odd, multiply base with result
            result = (result * base) % mod;
        }
        exp = exp >> 1;  // Divide exp by 2
        base = (base * base) % mod;  // Square the base
    }

    return result;
}

long generatePrivateKey() {
    return random(2, p - 1);  // Random private key
}

long generatePublicKey(long privateKey) {
    return modExp(g, privateKey, p);  // Public key calculation
}

long generateSharedSecret(long receivedPublicKey, long privateKey) {
    return modExp(receivedPublicKey, privateKey, p);  // Shared secret calculation
}

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print(F("Connecting to "));
    Serial.println(ssid);
    // Configure the Wi-Fi connection
    if (!WiFi.config(local_IP)) {
        Serial.println(F("STA Failed to configure"));
    }
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(F("."));
    }
    Serial.println(F(""));
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    
    // Setup Wi-Fi
    setup_wifi();
    
    // Wait for Wi-Fi connection
    delay(500);
    
    // Diffie-Hellman key exchange
    long privateKey = generatePrivateKey();
    long publicKey = generatePublicKey(privateKey);
    
    // Output the public key to the Serial Monitor
    Serial.print("Public Key: ");
    Serial.println(publicKey);
    
    // Simulate receiving a client's public key
    long clientPublicKey = 10;  // Replace with actual received value from client
    long sharedSecret = generateSharedSecret(clientPublicKey, privateKey);
    
    // Output the shared secret to the Serial Monitor
    Serial.print("Shared Secret: ");
    Serial.println(sharedSecret);
    
    // Start the server
    server.begin();
    Serial.println("Server started and waiting for client...");
}

void loop() {
    WiFiClient client = server.available();  // Listen for incoming clients
    if (client) {
        Serial.println("Client connected!");
        
        // Send public key to client
        long publicKey = generatePublicKey(generatePrivateKey());
        client.print(publicKey);
        
        // Wait for client's response
        while (client.connected()) {
            if (client.available()) {
                String msg = client.readString();
                Serial.println("Received message from client: " + msg);
            }
        }
        
        // Disconnect client after communication
        client.stop();
        Serial.println("Client disconnected.");
    }
}
