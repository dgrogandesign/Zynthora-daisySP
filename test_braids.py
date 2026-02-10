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
        print("Connected! Testing Braids Extended Commands...")
        
        # 1. Switch to Braids
        s.send(build_frame("wave:braids")) 
        time.sleep(0.5)
        
        # 2. Test Model CSAW
        s.send(build_frame("braids_model:0"))
        time.sleep(0.2)
        
        # 3. Test Timbre/Color
        s.send(build_frame("braids_timbre:0.5"))
        s.send(build_frame("braids_color:0.5"))
        time.sleep(0.2)
        
        # 4. Test Fine Tune
        print("Testing Fine Tune...")
        s.send(build_frame("braids_fine:0.5")) # +0.5 semitone
        s.send(build_frame("gate:1"))
        s.send(build_frame("note:60")) 
        time.sleep(1.0)
        s.send(build_frame("gate:0"))
        time.sleep(0.5)
        
        # 5. Test Coarse Tune
        print("Testing Coarse Tune...")
        s.send(build_frame("braids_fine:0.0"))
        s.send(build_frame("braids_coarse:12")) # +1 octave
        s.send(build_frame("gate:1"))
        s.send(build_frame("note:60")) 
        time.sleep(1.0)
        s.send(build_frame("gate:0"))
        time.sleep(0.5)

        # 6. Test FM (Pitch Mod LFO)
        print("Testing FM (Pitch LFO)...")
        s.send(build_frame("braids_coarse:0"))
        s.send(build_frame("braids_fm:0.5")) 
        s.send(build_frame("gate:1"))
        s.send(build_frame("note:50")) 
        time.sleep(2.0) # Listen for wobble
        s.send(build_frame("gate:0"))
        s.send(build_frame("braids_fm:0.0"))
        time.sleep(0.5)

        # 7. Test Modulation (Timbre Mod LFO)
        print("Testing Modulation (Timbre LFO)...")
        s.send(build_frame("braids_modulation:0.8")) 
        s.send(build_frame("gate:1"))
        s.send(build_frame("note:40")) 
        time.sleep(2.0)
        s.send(build_frame("gate:0"))
        s.send(build_frame("braids_modulation:0.0"))
        
        print("Extended Test complete.")
        
    s.close()
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
