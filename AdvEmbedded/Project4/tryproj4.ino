#include <AES.h>

// Diffie-Hellman parameters
#define PRIME 23    // Example prime number
#define BASE 5      // Example base

// Global variables
uint32_t privateKey, publicKey, sharedSecret;
byte aesKey[16]; // Derived AES key
byte iv[16] = {0}; // Initialization vector for AES

AES aes;

void setup() {
    Serial.begin(9600);
    randomSeed(analogRead(0)); // Seed random number generator

    // Generate private and public keys
    privateKey = random(1, PRIME);
    publicKey = computePublicKey(BASE, privateKey, PRIME);

    // Share public key
    Serial.println(publicKey);

    // Wait for the other Arduino's public key
    while (!Serial.available());
    uint32_t otherPublicKey = Serial.parseInt();

    // Compute shared secret and derive AES key
    sharedSecret = computeSharedSecret(otherPublicKey, privateKey, PRIME);
    memcpy(aesKey, &sharedSecret, sizeof(sharedSecret));

    Serial.println("Key Exchange Complete!");
}

void loop() {
    // Example: Sending an encrypted message
    char plaintext[] = "Hello Arduino!";
    byte ciphertext[16];
    encrypt((byte*)plaintext, ciphertext, sizeof(plaintext));
    Serial.write(ciphertext, sizeof(ciphertext));

    // Wait for and decrypt an incoming message
    while (!Serial.available());
    byte receivedCiphertext[16];
    Serial.readBytes(receivedCiphertext, 16);
    char decryptedText[16];
    decrypt(receivedCiphertext, (byte*)decryptedText, sizeof(receivedCiphertext));

    // Print the decrypted message
    Serial.print("Decrypted Message: ");
    Serial.println(decryptedText);

    delay(2000); // Pause between iterations
}

// Compute public key (g^privateKey mod p)
uint32_t computePublicKey(uint32_t base, uint32_t privateKey, uint32_t prime) {
    return (uint32_t)pow(base, privateKey) % prime;
}

// Compute shared secret (OtherPublicKey^privateKey mod p)
uint32_t computeSharedSecret(uint32_t otherPublicKey, uint32_t privateKey, uint32_t prime) {
    return (uint32_t)pow(otherPublicKey, privateKey) % prime;
}

// Encrypt data using AES-CBC
void encrypt(byte* plaintext, byte* ciphertext, size_t length) {
    aes.do_aes_encrypt(plaintext, length, ciphertext, aesKey, 128, iv);
}

// Decrypt data using AES-CBC
void decrypt(byte* ciphertext, byte* plaintext, size_t length) {
    aes.do_aes_decrypt(ciphertext, length, plaintext, aesKey, 128, iv);
}
