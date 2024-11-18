#include <WiFi.h>
#include <WebServer.h>
#include <CryptoAES_CBC.h>
#include <AES.h>
#include <string.h>
#include <BigNumber.h>

// Wi-Fi credentials - Access Point mode
const char* ssid = "ultra_wifi";
const char* password = "12345678";

// Diffie-Hellman parameters (use large prime numbers in production)
const char* prime = "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                   "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                   "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                   "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                   "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381"
                   "FFFFFFFFFFFFFFFF";
const char* generator = "2";

// Encryption variables
AES128 aes128;
byte ciphertext[128];
int ciphertextLength = 0;
byte iv[16];
byte derivedKey[16];

// DH variables
BigNumber privateKey;
BigNumber publicKey;
BigNumber sharedSecret;

// Web server
WebServer server(80);

// Helper function to print bytes
void printBytes(const char* label, byte* data, int length) {
    Serial.print(label);
    Serial.print(": ");
    for (int i = 0; i < length; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

// Convert byte array to hex string
String byteArrayToHexString(byte* data, int length) {
    String result = "";
    for (int i = 0; i < length; i++) {
        if (data[i] < 0x10) {
            result += "0";
        }
        result += String(data[i], HEX);
        result += " ";
    }
    return result;
}

// Generate random bytes for IV
void generateIV() {
    for (int i = 0; i < 16; i++) {
        iv[i] = random(256);
    }
}

// Initialize Diffie-Hellman parameters
void initDH() {
    BigNumber::begin();
    
    // Convert parameters to BigNumber
    BigNumber p(prime, 16);
    BigNumber g(generator);
    
    // Generate random private key (should be truly random in production)
    privateKey = BigNumber(random(1000000));
    
    // Calculate public key: g^private mod p
    publicKey = g.powMod(privateKey, p);
}

// Calculate shared secret from peer's public key
void calculateSharedSecret(String peerPublicKeyStr) {
    BigNumber p(prime, 16);
    BigNumber peerPublicKey(peerPublicKeyStr);
    
    // Calculate shared secret: peer_public^private mod p
    sharedSecret = peerPublicKey.powMod(privateKey, p);
    
    // Derive AES key from shared secret
    String secretStr = sharedSecret.toString();
    for (int i = 0; i < 16; i++) {
        derivedKey[i] = secretStr[i % secretStr.length()];
    }
    
    // Initialize AES with derived key
    aes128.setKey(derivedKey, 16);
}

void encryptMessage(const char* message) {
    int messageLen = strlen(message);
    byte plaintext[128];
    memcpy(plaintext, message, messageLen);
    
    // Add PKCS7 padding
    int paddingLength = 16 - (messageLen % 16);
    for (int i = 0; i < paddingLength; i++) {
        plaintext[messageLen + i] = paddingLength;
    }
    
    int totalLength = messageLen + paddingLength;
    
    // Generate new IV for each message
    generateIV();
    
    // Perform CBC encryption
    byte prev_block[16];
    memcpy(prev_block, iv, 16);
    
    for (int i = 0; i < totalLength; i += 16) {
        byte block[16];
        
        for (int j = 0; j < 16; j++) {
            block[j] = plaintext[i + j] ^ prev_block[j];
        }
        
        aes128.encryptBlock(&ciphertext[i], block);
        memcpy(prev_block, &ciphertext[i], 16);
    }
    
    ciphertextLength = totalLength;
}

void handleRoot() {
    String response = byteArrayToHexString(iv, 16) + "|" + 
                     byteArrayToHexString(ciphertext, ciphertextLength);
    server.send(200, "text/plain", response);
}

void handleKeyExchange() {
    String peerPublicKey = server.arg("key");
    if (peerPublicKey.length() > 0) {
        calculateSharedSecret(peerPublicKey);
        server.send(200, "text/plain", publicKey.toString());
    } else {
        server.send(400, "text/plain", "Missing public key");
    }
}

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));
    
    // Initialize Diffie-Hellman
    initDH();
    
    // Create Access Point
    WiFi.softAP(ssid, password);
    Serial.println("Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    // Setup web server
    server.on("/", handleRoot);
    server.on("/exchange", handleKeyExchange);
    server.begin();
    Serial.println("HTTP server started");
    
    // Generate initial IV
    generateIV();
}

void loop() {
    server.handleClient();
    
    if (Serial.available()) {
        String message = Serial.readStringUntil('\n');
        encryptMessage(message.c_str());
    }
    
    delay(10);
}
