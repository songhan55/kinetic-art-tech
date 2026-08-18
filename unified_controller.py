"""
Unified 3-Board Interactive Motor Controller
Controls Master ESP32 (COM15), Slave ESP32 (COM12), and Arduino UNO (COM8) from a single terminal!
"""

import sys
import time
import math
import serial
import threading

PORTS = {
    "Master ESP32": {"port": "COM15", "baud": 115200, "ser": None},
    "Slave ESP32":  {"port": "COM12", "baud": 115200, "ser": None},
    "Arduino UNO":  {"port": "COM8",  "baud": 9600,   "ser": None}
}

stop_threads = False

def reader_thread(name, ser):
    while not stop_threads and ser and ser.is_open:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"[{name}] {line}")
        except Exception:
            break

def send_to_port(name, cmd_str):
    info = PORTS.get(name)
    if info and info["ser"] and info["ser"].is_open:
        try:
            info["ser"].write((cmd_str + "\n").encode('utf-8'))
            info["ser"].flush()
            return True
        except Exception as e:
            print(f"❌ [{name}] 전송 실패: {e}")
            return False
    return False

def send_to_all(cmd_str):
    success_count = 0
    for name in PORTS:
        if send_to_port(name, cmd_str):
            success_count += 1
    return success_count

def run_wave_demo(duration_sec=15):
    print(f"\n🌊 [3-모터 동기화 파도 데모 시작] ({duration_sec}초간 실행...)")
    start = time.time()
    while time.time() - start < duration_sec:
        t = time.time() - start
        # 3개 모터에 위상차(Phase offset)를 주어 파도타기 연출
        # 모터 1 (마스터): sin(t * 3.2)
        # 모터 2 (슬레이브): sin(t * 3.2 - 0.8)
        # 모터 3 (우노): sin(t * 3.2 - 1.6)
        ang1 = int(round(150 + 18 * math.sin(t * 3.2)))
        ang2 = int(round(150 + 18 * math.sin(t * 3.2 - 0.8)))
        ang3 = int(round(150 + 18 * math.sin(t * 3.2 - 1.6)))

        send_to_port("Master ESP32", str(ang1))
        send_to_port("Slave ESP32", str(ang2))
        send_to_port("Arduino UNO", str(ang3))
        time.sleep(0.04) # 25Hz 파도

    print("✅ 파도 데모 완료! 150도 중립으로 복귀합니다.")
    send_to_all("150")

def main():
    global stop_threads
    print("=" * 60)
    print("  🚀 [통합 모터 제어 콘솔] 3개 보드 단일 시리얼 제어기")
    print("=" * 60)

    # 1. 3개 포트 연결
    for name, info in PORTS.items():
        try:
            ser = serial.Serial(info["port"], info["baud"], timeout=0.1)
            info["ser"] = ser
            t = threading.Thread(target=reader_thread, args=(name, ser), daemon=True)
            t.start()
            print(f"  ✅ {name} 연결 성공 ({info['port']} @ {info['baud']} baud)")
        except Exception as e:
            print(f"  ⚠️ {name} 연결 실패 ({info['port']}): {e}")

    time.sleep(1.0)
    # 초기 150도 정렬
    send_to_all("150")

    print("\n" + "-" * 60)
    print("👉 [명령어 가이드]")
    print("   • 각도 숫자 입력 (예: 150, 130, 170) -> 3개 모터 전부 그 각도로 동시 회전!")
    print("   • 개별 제어: '1 130' (마스터만), '2 170' (슬레이브만), '3 150' (우노만)")
    print("   • 'wave'   -> 3개 모터가 순차적으로 출렁이는 15초 파도타기 데모")
    print("   • 'home'   -> 3개 모터 150도 중립 정렬")
    print("   • 'auto'   -> ESP32 실시간 국제정세 긴장도 데이터 모드 복귀")
    print("   • 'q'      -> 종료")
    print("-" * 60 + "\n")

    try:
        while True:
            cmd = input("🎮 [각도/명령어 입력] >> ").strip().lower()
            if not cmd:
                continue

            if cmd in ["q", "quit", "exit"]:
                break
            elif cmd == "wave":
                run_wave_demo(15)
            elif cmd in ["home", "h"]:
                send_to_all("150")
                print("🎯 [3개 모터 전체 150도 홈 포지션 정렬 완료]")
            elif cmd in ["auto", "live", "a"]:
                send_to_port("Master ESP32", "auto")
                send_to_port("Slave ESP32", "auto")
                send_to_port("Arduino UNO", "150")
                print("🟢 [ESP32 실시간 국제정세 데이터 모드 복귀]")
            else:
                # 개별 보드 명령 분기: '1 130', '2 170', '3 150'
                parts = cmd.split()
                if len(parts) == 2 and parts[0] in ["1", "2", "3"]:
                    b_idx = parts[0]
                    target_ang = parts[1]
                    target_board = "Master ESP32" if b_idx == "1" else ("Slave ESP32" if b_idx == "2" else "Arduino UNO")
                    send_to_port(target_board, target_ang)
                    print(f"🎯 [{target_board}] -> {target_ang}° 개별 이동 완료")
                else:
                    # 전체 모터 동시 이동
                    try:
                        val = float(cmd)
                        if 0 <= val <= 180:
                            send_to_all(str(int(round(val))))
                            print(f"🎯 [3개 모터 전체 동시 회전] -> {int(round(val))}°")
                        else:
                            print("⚠️ 각도는 0 ~ 180도 사이로 입력해주세요!")
                    except ValueError:
                        print("⚠️ 올바른 숫자 또는 명령어를 입력해주세요 (예: 150, 130, wave, home)")

    except KeyboardInterrupt:
        pass
    finally:
        stop_threads = True
        for info in PORTS.values():
            if info["ser"] and info["ser"].is_open:
                info["ser"].close()
        print("\n👋 통합 제어 콘솔을 종료합니다.")

if __name__ == "__main__":
    main()
