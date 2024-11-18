import socket
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from Crypto.Protocol.KDF import PBKDF2
from Crypto.Hash import SHA256
from Crypto.Random.random import randint

# Diffie-Hellman parameters (same as server)
p = 17
g = 14

def generate_private_key():
return randint(2, p - 2)

def generate_public_key(private_key):
return pow(g, private_key, p)

def generate_shared_secret(server_pub_key, private_key):
return pow(server_pub_key, private_key, p)

def derive_key(shared_secret):
return PBKDF2(str(shared_secret).encode(), b'salt', dkLen=32, count=1000, hmac_hash_module=SHA256)

# Connect to server
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect(('192.168.1.50', 80))

# DH Key Exchange
server_public_key = int(client_socket.recv(1024).decode())

client_private_key = generate_private_key()
client_public_key = generate_public_key(client_private_key)
client_socket.send(str(client_public_key).encode())

shared_secret = generate_shared_secret(server_public_key, client_private_key)
aes_key = derive_key(shared_secret)

# AES-CBC Decryption
iv = client_socket.recv(16)
cipher = AES.new(aes_key, AES.MODE_CBC, iv)

ciphertext = client_socket.recv(1024)
plaintext = unpad(cipher.decrypt(ciphertext), AES.block_size)
print(f"Decrypted message: {plaintext.decode()}")

# Close connection
client_socket.close()
