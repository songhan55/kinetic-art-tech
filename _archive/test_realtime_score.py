import time
import urllib.request
import json

print("==================================================")
print("  실시간 트렌드 데이터 수신 테스트 (Real-time Score)")
print("==================================================")

url = "https://api.gdeltproject.org/api/v2/doc/doc?query=war&mode=timelinevol&format=json"

headers = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"
}

try:
    req = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(req) as response:
        raw_data = response.read().decode('utf-8')
        json_data = json.loads(raw_data)
        
        # Get latest data point
        latest = json_data['timeline'][0]['data'][-1]
        timestamp = latest.get('datetime', 'N/A')
        volume_val = latest.get('value', 0)
        
        # Normalize/Scale score to 0 ~ 100
        # Typical volume value is around 0.5 ~ 3.5%
        score_100 = min(100.0, max(0.0, (volume_val / 4.0) * 100.0))
        mapped_angle = 5.0 + (score_100 / 100.0) * 15.0  # 5 deg ~ 20 deg swing
        
        print(f"[수신 시간]: {timestamp}")
        print(f"[실시간 트렌드 점수]: {score_100:.1f} / 100")
        print(f"[키네틱 모터 매핑 각도]: ±{mapped_angle:.1f}° (150° 기준)")
        print("==================================================")
        print(" SUCCESS: 실시간 데이터를 성공적으로 받아왔습니다!")
        print("==================================================")

except Exception as e:
    print(f"[오류 발생]: {e}")
