from Crypto.Cipher import AES  # Import the AES cipher module for encryption/decryption.
from Crypto.Util.Padding import unpad  # Import the unpad function to remove padding from decrypted data.
import socket  # Import the socket library for networking.
import secrets  # Import the secrets library for generating cryptographically secure random numbers.

# Define constants for Diffie-Hellman key exchange.
PRIME = 2**521 - 1  # A large prime number used as the modulus in Diffie-Hellman.
GENERATOR = 5  # The base (or generator) used in the Diffie-Hellman algorithm.

# Function to generate a private key for Diffie-Hellman.
def diffie_hellman_private_key():
    return secrets.randbelow(PRIME)  # Generate a random number less than the prime.

# Function to calculate the public key using the private key.
def diffie_hellman_public_key(private_key):
    return pow(GENERATOR, private_key, PRIME)  # Compute (GENERATOR^private_key) % PRIME.

# Function to compute the shared secret using the other party's public key and your private key.
def diffie_hellman_shared_key(their_public_key, private_key):
    return pow(their_public_key, private_key, PRIME)  # Compute (their_public_key^private_key) % PRIME.

# Function to decrypt a message using AES.
def decrypt_message(key, ciphertext):
    iv = ciphertext[:16]  # Extract the initialization vector (IV) from the first 16 bytes.
    cipher = AES.new(key[:32], AES.MODE_CBC, iv)  # Create a new AES cipher in CBC mode with the IV.
    plaintext = unpad(cipher.decrypt(ciphertext[16:]), AES.block_size)  # Decrypt and remove padding.
    return plaintext.decode()  # Decode the plaintext to a string.

# Generate a private key for Diffie-Hellman.
private_key = diffie_hellman_private_key()
print(f"Generated private key: {private_key}")

# Calculate the corresponding public key.
public_key = diffie_hellman_public_key(private_key)
print(f"Generated public key: {public_key}")

# Setup a TCP server to communicate with a client.
server_address = ('localhost', 12345)  # Define server address and port.
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP/IP socket.
sock.bind(server_address)  # Bind the socket to the server address.
sock.listen(1)  # Start listening for incoming connections (up to 1 queued connection).

print("Waiting for a connection...")
connection, client_address = sock.accept()  # Accept an incoming connection.
print(f"Connection established with {client_address}!")

try:
    # Receive the public key from the client.
    their_public_key = int(connection.recv(4096).decode())  # Read and decode the received public key.
    print(f"Received public key from sender.py: {their_public_key}")

    # Send this server's public key to the client.
    print("Sending public key to sender.py...")
    connection.sendall(str(public_key).encode())  # Encode and send the public key.

    # Compute the shared Diffie-Hellman key.
    shared_key = diffie_hellman_shared_key(their_public_key, private_key)
    print(f"Computed shared key: {shared_key}")

    # Derive an AES key from the shared key by converting it to bytes.
    aes_key = shared_key.to_bytes((shared_key.bit_length() + 7) // 8, byteorder='big')
    print(f"Derived AES key (first 32 bytes): {aes_key[:32].hex()}")

    # Continuously receive encrypted messages from the client.
    while True:
        encrypted_message = connection.recv(4096)  # Receive encrypted data.
        if not encrypted_message:  # If no data is received, continue waiting.
            continue
        
        print(f"Received encrypted message (IV + Ciphertext): {encrypted_message.hex()}")
        decrypted_message = decrypt_message(aes_key, encrypted_message)  # Decrypt the message.
        print(f"Decrypted message: {decrypted_message}")  # Print the decrypted message.
except Exception as e:
    # Handle any errors that occur.
    print(f"An error occurred: {e}")
finally:
    # Clean up the server resources.
    print("Exiting reader.py...")
    connection.close()  # Close the connection to the client.
    sock.close()  # Close the server socket.
