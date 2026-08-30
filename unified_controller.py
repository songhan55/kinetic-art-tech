"""
Unified ESP32 PCA9685 & Arduino Motor Controller (Windows CP949/UTF-8 Compatible)
"""

import sys
import time
import math
import serial
import serial.tools.list_ports
import threading

# Windows 콘솔 인코딩 UTF-8 강제 지정
try:
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stdin.reconfigure(encoding='utf-8')
except Exception:
    pass

stop_threads = False
active_ports = {}

def find_ports():
    ports = serial.tools.list_ports.comports()
    found = []
    for p in ports:
        # Bluetooth 포트 제외
        if "Bluetooth" not in p.description and "BTH" not in p.hwid:
            found.append(p.device)
    return found

def reader_thread(port_name, ser):
    while not stop_threads and ser and ser.is_open:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"[{port_name}] {line}")
        except Exception:
            break

def send_all(cmd_str):
    success = 0
    for name, ser in active_ports.items():
        if ser and ser.is_open:
            try:
                ser.write((cmd_str + "\n").encode('utf-8'))
                ser.flush()
                success += 1
            except Exception as e:
                print(f"[{name}] 전송 에러: {e}")
    return success

def run_wave_demo(duration_sec=15):
    print(f"\n[파도 데모 시작] ({duration_sec}초 동안 3채널 연속 파도타기...)")
    send_all("wave")
    start = time.time()
    while time.time() - start < duration_sec:
        time.sleep(0.5)
    print("\n[파도 데모 완료] 150도 홈 포지션 복귀")
    send_all("home")

def main():
    global stop_threads
    print("=" * 60)
    print("  [통합 모터 제어 콘솔] ESP32 PCA9685 / Arduino")
    print("=" * 60)

    # 연결 가능한 시리얼 포트 검색
    detected = find_ports()
    if not detected:
        print("경고: 연결된 USB COM 포트를 찾을 수 없습니다. 케이블을 확인해주세요.")
    else:
        print(f"발견된 USB 포트: {', '.join(detected)}")

    for p in detected:
        # ESP32 및 Arduino Uno 모두 115200 baud 사용
        baud = 115200
        try:
            ser = serial.Serial(p, baud, timeout=0.1)
            active_ports[p] = ser
            t = threading.Thread(target=reader_thread, args=(p, ser), daemon=True)
            t.start()
            print(f"  -> {p} 연결 성공 ({baud} baud)")
        except Exception as e:
            print(f"  -> {p} 연결 실패: {e}")

    time.sleep(0.5)
    # 초기 150도 중립 정렬
    send_all("150")

    print("\n" + "-" * 60)
    print("[명령어 가이드]")
    print("  • 각도 숫자 입력 (예: 150, 130, 170) -> 3개 모터 전체 그 각도로 회전")
    print("  • 개별 제어: '0 130' (0번만), '1 150' (1번만), '2 170' (2번만)")
    print("  • 'test'  -> 0번, 1번, 2번 모터를 하나씩 순서대로 징~ 움직여 채널 진단")
    print("  • 'wave'  -> 3개 모터가 시차를 두고 출렁이는 부드러운 파도타기")
    print("  • 'sweep' -> 130도 ~ 170도 사이를 2회 왕복 스위프")
    print("  • 'full'  -> 150도 기준 ±30도(120도~180도) 3회 왕복 풀스윙")
    print("  • 'home'  -> 150도 중립 홈 포지션 정렬")
    print("  • 'scan'  -> PCA9685 I2C 통신 상태 스캔")
    print("  • 'q'     -> 콘솔 종료")
    print("-" * 60 + "\n")

    try:
        while True:
            cmd = input(">> [명령어 입력] ").strip().lower()
            if not cmd:
                continue

            if cmd in ["q", "quit", "exit"]:
                break
            elif cmd == "wave":
                run_wave_demo(15)
            elif cmd in ["home", "h", "stop"]:
                send_all("home")
                print(">> 모든 모터 150도 홈 포지션 정렬 완료")
            elif cmd in ["test", "t", "diag"]:
                send_all("test")
                print(">> 채널별 1개씩 순차 자가진단 실행 중...")
            elif cmd in ["sweep", "s"]:
                send_all("sweep")
                print(">> 130도 ~ 170도 왕복 스위프 실행 중...")
            elif cmd in ["full", "f"]:
                send_all("full")
                print(">> ±30도(120도~180도) 3회 왕복 풀스윙 실행 중...")
            elif cmd == "scan":
                send_all("scan")
                print(">> I2C 버스 스캔 명령 전송...")
            else:
                send_all(cmd)
                print(f">> 명령 전송 완료: {cmd}")

    except KeyboardInterrupt:
        pass
    finally:
        stop_threads = True
        for ser in active_ports.values():
            if ser and ser.is_open:
                ser.close()
        print("\n통합 제어 콘솔을 종료합니다.")

if __name__ == "__main__":
    main()
