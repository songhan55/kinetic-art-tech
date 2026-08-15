import os, re, shutil

# 1. Update Presentation Decks (Replace X logo with Bluesky Butterfly Logo)
deck_files = [
    r"C:\art_tech\team_presentation_deck.html",
    r"C:\art_tech\dist\team_presentation_deck.html",
    r"C:\art_tech\netlify_deploy\team_presentation_deck.html",
    r"C:\art_tech\presentation\team_presentation_deck.html",
    r"C:\art_tech\dist\presentation\team_presentation_deck.html",
    r"C:\art_tech\netlify_deploy\presentation\team_presentation_deck.html"
]

x_logo_block = re.compile(
    r'<!-- X Logo \(SVG\) -->.*?'
    r'전 세계 실시간 소식의 오픈 플랫폼, Open Social & Global News Stream\s*</div>',
    re.DOTALL
)

bluesky_logo_block = """<!-- Bluesky Logo (Official Butterfly SVG) -->
          <div style="width: 92px; height: 92px; background: linear-gradient(135deg, #0285FF 0%, #0062cc 100%); border-radius: 24px; display: flex; align-items: center; justify-content: center; box-shadow: 0 10px 30px rgba(2,133,255,0.45); border: 1px solid rgba(255,255,255,0.35);">
            <svg viewBox="0 0 568 501" width="56" height="56" fill="#ffffff">
              <path d="M123.121 33.664C188.241 82.553 258.281 181.68 284 234.873c25.719-53.192 95.759-152.32 160.879-201.21C491.866-1.611 568-28.906 568 57.947c0 17.346-9.945 145.713-15.778 166.555-20.275 72.453-94.155 90.933-159.875 79.748C507.222 323.8 536.444 388.56 473.333 453.32c-119.25 122.39-166.16-30.82-185.333-93.57-4.04-13.23-4-13.23-8 0-19.173 62.75-66.083 215.96-185.333 93.57-63.111-64.76-33.889-129.52 80.986-149.07-65.72 11.185-139.6-7.295-159.875-79.748C9.945 203.66 0 75.293 0 57.947 0-28.906 76.135-1.612 123.121 33.664Z"/>
            </svg>
          </div>

          <div style="font-size: 16px; font-weight: 800; color: #60a5fa; letter-spacing: 0.08em; text-transform: uppercase;">
            차세대 오픈 소셜 미디어 플랫폼, Bluesky (블루스카이)
          </div>"""

for f in deck_files:
    if os.path.exists(f):
        with open(f, 'r', encoding='utf-8') as fp:
            content = fp.read()
        content = x_logo_block.sub(bluesky_logo_block, content)
        content = content.replace("전 세계 X(트위터)", "전 세계 오픈 소셜 미디어(Bluesky)")
        content = content.replace("X(트위터)", "Bluesky(블루스카이)")
        content = content.replace("X 키워드", "Bluesky 키워드")
        with open(f, 'w', encoding='utf-8') as fp:
            fp.write(content)
        print(f"Updated Logo in: {f}")

# 2. Update project_spec.md
spec_files = [
    r"C:\art_tech\project_spec.md",
    r"C:\art_tech\dist\project_spec.md",
    r"C:\art_tech\netlify_deploy\project_spec.md"
]

for f in spec_files:
    if os.path.exists(f):
        with open(f, 'r', encoding='utf-8') as fp:
            content = fp.read()
        content = content.replace("X(트위터) 실시간 언급량", "Bluesky(블루스카이) 실시간 오픈 소셜 언급량")
        content = content.replace("X(트위터) 수집 데이터", "Bluesky 오픈 소셜 수집 데이터")
        content = content.replace("GDELT/X", "Bluesky/GDELT")
        with open(f, 'w', encoding='utf-8') as fp:
            fp.write(content)
        print(f"Updated Spec: {f}")

# 3. Re-package dist to art_tech_deploy.zip
shutil.make_archive(r'C:\art_tech\art_tech_deploy', 'zip', r'C:\art_tech\dist')
print("Successfully re-packaged art_tech_deploy.zip")
