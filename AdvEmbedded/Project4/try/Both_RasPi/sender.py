from Crypto.Cipher import AES  # Import AES cipher module for encryption.
from Crypto.Util.Padding import pad  # Import the pad function to add padding to plaintext.
from Crypto.Random import get_random_bytes  # Import function to generate random bytes for the IV.
import socket  # Import the socket library for networking.
import secrets  # Import the secrets library to generate secure random numbers.
import time  # Import the time library for adding delays.

# Define constants for Diffie-Hellman key exchange.
PRIME = 2**521 - 1  # A large prime number used as the modulus in Diffie-Hellman.
GENERATOR = 5  # The base (generator) used in the Diffie-Hellman algorithm.

# Function to generate a private key for Diffie-Hellman.
def diffie_hellman_private_key():
    return secrets.randbelow(PRIME)  # Generate a random private key less than the prime.

# Function to calculate the public key using the private key.
def diffie_hellman_public_key(private_key):
    return pow(GENERATOR, private_key, PRIME)  # Compute (GENERATOR^private_key) % PRIME.

# Function to compute the shared secret using the other party's public key and the private key.
def diffie_hellman_shared_key(their_public_key, private_key):
    return pow(their_public_key, private_key, PRIME)  # Compute (their_public_key^private_key) % PRIME.

# Function to encrypt a message using AES in CBC mode.
def encrypt_message(key, plaintext):
    iv = get_random_bytes(16)  # Generate a random 16-byte initialization vector (IV).
    cipher = AES.new(key[:32], AES.MODE_CBC, iv)  # Create an AES cipher object using the first 32 bytes of the key.
    ciphertext = cipher.encrypt(pad(plaintext.encode(), AES.block_size))  # Encrypt the padded plaintext.
    return iv + ciphertext  # Return the IV concatenated with the ciphertext.

# Generate a private key for Diffie-Hellman.
private_key = diffie_hellman_private_key()
print(f"Generated private key: {private_key}")

# Calculate the corresponding public key.
public_key = diffie_hellman_public_key(private_key)
print(f"Generated public key: {public_key}")

# Set up a TCP client to communicate with a server.
server_address = ('localhost', 12345)  # Define the server address and port.
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP socket.
print("Connecting to server...")
sock.connect(server_address)  # Connect to the server.
print("Connected to server!")

try:
    # Send the public key to the server.
    print("Sending public key to reader.py...")
    sock.sendall(str(public_key).encode())  # Convert the public key to a string and send it.

    # Receive the server's public key.
    their_public_key = int(sock.recv(4096).decode())  # Receive and decode the server's public key.
    print(f"Received public key from reader.py: {their_public_key}")

    # Compute the shared Diffie-Hellman key.
    shared_key = diffie_hellman_shared_key(their_public_key, private_key)
    print(f"Computed shared key: {shared_key}")

    # Derive the AES encryption key from the shared key.
    aes_key = shared_key.to_bytes((shared_key.bit_length() + 7) // 8, byteorder='big')  # Convert the key to bytes.
    print(f"Derived AES key (first 32 bytes): {aes_key[:32].hex()}")  # Print the first 32 bytes of the key.

    # Loop to send numbers 1 through 25 repeatedly.
    count = 1  # Start with the number 1.
    while True:
        message = str(count)  # Convert the current count to a string.
        encrypted_message = encrypt_message(aes_key, message)  # Encrypt the message.
        print(f"Encrypting message: {message}")
        print(f"Encrypted message (IV + Ciphertext): {encrypted_message.hex()}")  # Print the encrypted message.

        sock.sendall(encrypted_message)  # Send the encrypted message to the server.
        print(f"Sent encrypted message: {message}")

        count += 1  # Increment the counter.
        if count > 25:  # Reset the counter to 1 after 25.
            count = 1

        time.sleep(1)  # Wait for 1 second before sending the next message.
except Exception as e:
    # Handle any exceptions that occur during communication.
    print(f"An error occurred: {e}")
finally:
    # Ensure the program cleans up properly.
    print("Exiting sender.py...")
    sock.close()  # Close the socket connection.
