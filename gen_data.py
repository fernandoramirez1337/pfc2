import random

# Define file size in bytes
file_size = 2 * 1024 * 1024  # 2MB

# Open the file in binary write mode
with open("data.bin", "wb") as f:
  # Generate random bytes and write them to the file
  for _ in range(file_size):
    f.write(bytes([random.randint(0, 255)]))

print("Created data.bin with 2MB of random data.")