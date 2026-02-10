import socket
import time
import base64
import random
import sys

def create_ws_key():
    key = random.randbytes(16)
    return base64.b64encode(key).decode('utf-8')

def build_frame(message):
    data = message.encode('utf-8')
    length = len(data)
    frame = bytearray([0x81, 0x80 | length])
    mask = [random.randint(0, 255) for _ in range(4)]
    frame.extend(mask)
    for i, byte in enumerate(data):
        frame.append(byte ^ mask[i % 4])
    return frame

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    s.connect(('127.0.0.1', 8000))
    
    key = create_ws_key()
    req = (
        "GET /websocket HTTP/1.1\r\n"
        "Host: localhost:8000\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: {}\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n"
    ).format(key)
    
    s.send(req.encode())
    resp = s.recv(4096)
    if "101 Switching Protocols" in resp.decode():
        print("Connected! Testing Logue UI Commands...")
        
        # 1. Switch to Logue via String Command
        print("Sending 'wave:logue'...")
        s.send(build_frame("wave:logue")) 
        time.sleep(0.5)
        
        # 2. Test Detune
        print("Sending 'logue_detune:0.5' (50%)...")
        s.send(build_frame("logue_detune:0.5"))
        time.sleep(0.2)

        # 3. Test Octave
        print("Sending 'logue_oct:-1' (Sub Octave)...")
        s.send(build_frame("logue_oct:-1"))
        time.sleep(0.2)
        
        # 4. Play a note to hear it
        print("Playing Note (60)...")
        s.send(build_frame("gate:1"))
        s.send(build_frame("note:60"))
        time.sleep(1.0)
        s.send(build_frame("gate:0"))
        
        print("Test complete.")
        
    s.close()
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
