import os, re

files = [
    r"C:\art_tech\team_presentation_deck.html",
    r"C:\art_tech\dist\team_presentation_deck.html",
    r"C:\art_tech\netlify_deploy\team_presentation_deck.html",
    r"C:\art_tech\presentation\team_presentation_deck.html",
    r"C:\art_tech\dist\presentation\team_presentation_deck.html",
    r"C:\art_tech\netlify_deploy\presentation\team_presentation_deck.html"
]

def update_deck(file_path):
    if not os.path.exists(file_path):
        return False
    with open(file_path, 'r', encoding='utf-8') as fp:
        html = fp.read()

    # 1. Slide 0 Cover Subtitle & Pillar
    html = html.replace(
        "전 세계 실시간 뉴스 및 X(트위터) 빅데이터 수집과 36개 키네틱 종 파도, 3D 미디어아트를 결합하여",
        "전 세계 실시간 오픈 소셜 미디어(Bluesky) 및 글로벌 빅데이터 수집과 36개 키네틱 종 파도, 3D 미디어아트를 결합하여"
    )
    html = html.replace(
        "전 세계 X(트위터) 및 글로벌 뉴스에서 '전쟁/분쟁' 키워드 점유율을 10초마다 실시간 수집·정규화",
        "오픈 소셜 미디어(Bluesky) 및 글로벌 뉴스에서 '전쟁/분쟁' 키워드 언급량을 실시간 수집·정규화 (ESP32 듀얼코어)"
    )

    # 2. Slide 2: X Logo / Social Platform Header
    html = html.replace(
        "전 세계 실시간 소식의 집합소, X (Twitter)",
        "전 세계 실시간 소식의 오픈 플랫폼, Open Social & Global News Stream"
    )

    # 3. Slide 3: Real-Time Actuation Title & Steps
    html = html.replace(
        '<h1 class="slide-title">10초마다 X 데이터를 받아와 영상과 모터에 실시간 반영</h1>',
        '<h1 class="slide-title">실시간 소셜·뉴스 데이터를 받아와 3D 영상과 36개 모터에 동기화</h1>'
    )
    html = html.replace(
        '<p class="slide-subtitle">수신된 분쟁 데이터 지수에 따라 즉각적으로 변화하는 시각과 청각 연출</p>',
        '<p class="slide-subtitle">수신된 실시간 분쟁 지수(0~100점)에 따라 4단계 색상 스펙트럼과 물리적 파도로 즉각 반응</p>'
    )
    html = html.replace(
        '<strong style="color: var(--gold);">10초 주기 실시간 데이터 수신:</strong><br>\n                전 세계 전쟁 관련 뉴스 및 X 키워드 점유율 지수($S(t)$) 실시간 정규화',
        '<strong style="color: var(--gold);">실시간 오픈 소셜 & 빅데이터 수신:</strong><br>\n                전 세계 전쟁/분쟁 언급 지수($S(t)$) 실시간 정규화 (ESP32 Core 0 백그라운드)'
    )
    html = html.replace(
        '<strong style="color: var(--gold);">전면 3D 미디어아트 영상 반영:</strong><br>\n                28,000개 파티클 구체의 난류와 핏빛 루비 레드 빛 폭발',
        '<strong style="color: var(--gold);">전면 3D 미디어아트 4단계 스펙트럼:</strong><br>\n                [0~30점 에메랄드] → [30~65점 샴페인 골드(현재)] → [65~85점 오렌지] → [85~100점 핏빛 크림슨 레드]'
    )
    html = html.replace(
        '<strong style="color: var(--gold);">36개 키네틱 모터 & 종 반영:</strong><br>\n                최대 ±20° 회전 진폭 확장과 강렬한 공간 종 파도 소리 울림',
        '<strong style="color: var(--gold);">36개 키네틱 모터 & 종 반영 (ESP32 중앙제어):</strong><br>\n                SMPS 5V 20A 기반 1m 유리섬유 최대 ±20° 회전 진폭 확장과 강렬한 공간 종 파도 소리 울림'
    )
    html = html.replace(
        '• <strong>일상적 상태 시 (평온):</strong><br>\n                진폭이 줄어들며 잔잔한 호흡 모션으로 대기합니다.',
        '• <strong>일상적 상태 (30~65점, 현재 약 40점대):</strong><br>\n                따뜻한 샴페인 골드빛 파티클과 부드러운 중진폭(±11°) 종소리로 일상적 기류를 연출합니다.<br><br>\n                • <strong>극단적 전쟁 위기 (85점 이상 / 2022 러-우 위기 98.4점):</strong><br>\n                3D 영상은 격렬한 소용돌이와 핏빛 크림슨 레드로 폭발하고, 36개의 모터는 최대 ±20°로 빠르게 회전하며 경각심의 종소리를 냅니다.'
    )

    with open(file_path, 'w', encoding='utf-8') as fp:
        fp.write(html)
    return True

updated_count = 0
for f in files:
    if update_deck(f):
        print(f"Updated: {f}")
        updated_count += 1

print(f"Total updated decks: {updated_count}")
