import serial, time

try:
    ser = serial.Serial('COM15', 115200, timeout=1)
    time.sleep(0.5)
    print("Reading COM15 for 3 seconds...")
    start = time.time()
    while time.time() - start < 3:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore')
            print(line, end='')
    ser.close()
except Exception as e:
    print("Error:", e)
