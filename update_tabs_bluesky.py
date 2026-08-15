import os, shutil

deck_files = [
    r"C:\art_tech\team_presentation_deck.html",
    r"C:\art_tech\dist\team_presentation_deck.html",
    r"C:\art_tech\netlify_deploy\team_presentation_deck.html",
    r"C:\art_tech\presentation\team_presentation_deck.html",
    r"C:\art_tech\dist\presentation\team_presentation_deck.html",
    r"C:\art_tech\netlify_deploy\presentation\team_presentation_deck.html"
]

for f in deck_files:
    if os.path.exists(f):
        with open(f, 'r', encoding='utf-8') as fp:
            content = fp.read()
        
        content = content.replace("2. X 데이터 & 전쟁 확률", "2. Bluesky 데이터 & 전쟁 확률")
        content = content.replace("3. 실시간 10초 반영", "3. 실시간 데이터 연동")
        
        with open(f, 'w', encoding='utf-8') as fp:
            fp.write(content)
        print(f"Updated Navigation Tab in: {f}")

shutil.make_archive(r'C:\art_tech\art_tech_deploy', 'zip', r'C:\art_tech\dist')
print("Re-packaged art_tech_deploy.zip")
