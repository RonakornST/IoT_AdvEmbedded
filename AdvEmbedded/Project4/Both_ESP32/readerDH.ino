#include <WiFi.h>
#include <HTTPClient.h>
#include <CryptoAES_CBC.h>
#include <AES.h>
#include <BigNumber.h>

// Wi-Fi credentials - must match sender's AP
const char* ssid = "ultra_wifi";
const char* password = "12345678";

// Diffie-Hellman parameters (same as sender)
const char* prime = "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                   "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                   "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                   "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                   "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381"
                   "FFFFFFFFFFFFFFFF";
const char* generator = "2";

// Encryption variables
AES128 aes128;
byte decryptedtext[128];
byte derivedKey[16];

// DH variables
BigNumber privateKey;
BigNumber publicKey;
BigNumber sharedSecret;

// Server URL
String serverURL;

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

// Convert hex string to byte array
int hexStringToByteArray(String hexString, byte* data) {
    String cleanHex = "";
    for (int i = 0; i < hexString.length(); i++) {
        if (isxdigit(hexString.charAt(i))) {
            cleanHex += hexString.charAt(i);
        }
    }
    
    int length = cleanHex.length() / 2;
    for (int i = 0; i < length; i++) {
        String byteString = cleanHex.substring(i * 2, i * 2 + 2);
        data[i] = (byte)strtol(byteString.c_str(), NULL, 16);
    }
    
    return length;
}

// Initialize Diffie-Hellman parameters
void initDH() {
    BigNumber::begin();
    
    BigNumber p(prime, 16);
    BigNumber g(generator);
    
    privateKey = BigNumber(random(1000000));
    publicKey = g.powMod(privateKey, p);
}

// Calculate shared secret from peer's public key
void calculateSharedSecret(String peerPublicKeyStr) {
    BigNumber p(prime, 16);
    BigNumber peerPublicKey(peerPublicKeyStr);
    
    sharedSecret = peerPublicKey.powMod(privateKey, p);
    
    // Derive AES key from shared secret
    String secretStr = sharedSecret.toString();
    for (int i = 0; i < 16; i++) {
        derivedKey[i] = secretStr[i % secretStr.length()];
    }
    
    // Initialize AES with derived key
    aes128.setKey(derivedKey, 16);
}

// Perform key exchange with server
bool performKeyExchange() {
    HTTPClient http;
    String exchangeURL = serverURL + "exchange?key=" + publicKey.toString();
    
    http.begin(exchangeURL);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
        String peerPublicKey = http.getString();
        calculateSharedSecret(peerPublicKey);
        http.end();
        return true;
    }
    
    http.end();
    return false;
}

void decryptMessage(byte* iv, byte* ciphertext, int length) {
    Serial.println("\nStarting decryption process");
    
    // Perform CBC decryption
    byte prev_block[16];
    memcpy(prev_block, iv, 16);
    
    for (int i = 0; i < length; i += 16) {
        byte temp[16];
        byte current_block[16];
        memcpy(current_block, &ciphertext[i], 16);
        
        aes128.decryptBlock(temp, current_block);
        
        for (int j = 0; j < 16; j++) {
            decryptedtext[i + j] = temp[j] ^ prev_block[j];
        }
        
        memcpy(prev_block, current_block, 16);
    }
    
    // Remove PKCS7 padding
    int padding = decryptedtext[length - 1];
    if (padding > 0 && padding <= 16) {
        bool valid_padding = true;
        for (int i = length - padding; i < length; i++) {
            if (decryptedtext[i] != padding) {
                valid_padding = false;
                break;
            }
        }
        if (valid_padding) {
            length -= padding;
        }
    }
    
    Serial.print("Decrypted message: ");
    for (int i = 0; i < length; i++) {
        Serial.write(decryptedtext[i]);
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));
    
    // Initialize Diffie-Hellman
    initDH();
    
    // Connect to sender's AP
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    serverURL = "http://" + WiFi.gatewayIP().toString() + "/";
    Serial.print("Server URL: ");
    Serial.println(serverURL);
    
    // Perform initial key exchange
    if (performKeyExchange()) {
        Serial.println("Key exchange successful");
    } else {
        Serial.println("Key exchange failed");
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverURL);
        int httpResponseCode = http.GET();
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            response.trim();
            
            // Split response into IV and ciphertext
            int delimiterPos = response.indexOf('|');
            if (delimiterPos > 0) {
                String ivHex = response.substring(0, delimiterPos);
                String ciphertextHex = response.substring(delimiterPos + 1);
                
                byte iv[16];
                byte ciphertext[128];
                
                int ivLength = hexStringToByteArray(ivHex, iv);
                int ciphertextLength = hexStringToByteArray(ciphertextHex, ciphertext);
                
                if (ivLength == 16 && ciphertextLength > 0) {
                    decryptMessage(iv, ciphertext, ciphertextLength);
                }
            }
        }
        
        http.end();
    } else {
        Serial.println("WiFi disconnected, reconnecting...");
        WiFi.begin(ssid, password);
    }
    
    delay(5000);
}
