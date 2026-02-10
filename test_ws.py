import socket
import time
import base64
import random

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
        print("Connected! Testing HD Morph (0-63)...")
        
        # Switch to WT
        s.send(build_frame("wave:wavetable"))
        time.sleep(0.1)
        s.send(build_frame("note:40")) # Deep bass
        s.send(build_frame("gate:1"))
        
        # Sweep Full Range 0-63
        # Validating smooth morph across 64 frames
        for i in range(0, 640 , 20):  
            val = i / 10.0 
            if val > 63.0: val = 63.0
            
            cmd = f"wt_index:{val}"
            print(f"Sending: {cmd}")
            s.send(build_frame(cmd))
            time.sleep(0.05) # Fast sweep
            
        s.send(build_frame("gate:0"))
        print("HD Morph sweep complete.")
        
    s.close()
except Exception as e:
    print(f"Error: {e}")
