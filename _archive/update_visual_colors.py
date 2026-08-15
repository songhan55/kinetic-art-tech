import os, re

files_to_update = [
    r"C:\art_tech\exhibition_visual.html",
    r"C:\art_tech\dist\exhibition_visual.html",
    r"C:\art_tech\netlify_deploy\exhibition_visual.html",
    r"C:\art_tech\presentation\exhibition_visual.html",
    r"C:\art_tech\dist\presentation\exhibition_visual.html",
    r"C:\art_tech\netlify_deploy\presentation\exhibition_visual.html"
]

target_pattern = re.compile(
    r"const specT = Math\.min\(1\.0, Math\.max\(0\.0, \(currentDamp - 0\.15\) / 0\.85\)\);.*?"
    r"colAttr\.needsUpdate = true;",
    re.DOTALL
)

replacement_code = """// 4-Stage Calibrated Spectrum based on Effective Threat Level (effScore)
      const riskRatio = Math.min(1.0, Math.max(0.0, effScore / 100.0));

      let baseR, baseG, baseB;
      if (riskRatio < 0.30) {
        // [0 ~ 30%] Peace / Calm: Emerald Green (#10b981) -> Warm Gold
        const k = riskRatio / 0.30;
        baseR = 0.08 + (0.80 - 0.08) * k;
        baseG = 0.78 + (0.64 - 0.78) * k;
        baseB = 0.52 + (0.35 - 0.52) * k;
      } else if (riskRatio < 0.65) {
        // [30 ~ 65%] Everyday Trend (e.g. 39.5 pts): Elegant Champagne Amber Gold (#cba258)
        const k = (riskRatio - 0.30) / 0.35;
        baseR = 0.80 + (0.96 - 0.80) * k;
        baseG = 0.64 + (0.50 - 0.64) * k;
        baseB = 0.35 + (0.12 - 0.35) * k;
      } else if (riskRatio < 0.85) {
        // [65 ~ 85%] High Tension: Vibrant Tangerine Orange (#f97316)
        const k = (riskRatio - 0.65) / 0.20;
        baseR = 0.96 + (0.95 - 0.96) * k;
        baseG = 0.50 + (0.22 - 0.50) * k;
        baseB = 0.12 + (0.08 - 0.12) * k;
      } else {
        // [85 ~ 100%] Extreme War Crisis (e.g. Feb 2022 Eve 98.4 pts): Crimson Blood Red (#ef4444)
        const k = (riskRatio - 0.85) / 0.15;
        baseR = 0.95 + (0.99 - 0.95) * k;
        baseG = 0.22 + (0.08 - 0.22) * k;
        baseB = 0.08 + (0.08 - 0.08) * k;
      }

      const gaugeColor = `rgb(${Math.round(baseR * 255)}, ${Math.round(baseG * 255)}, ${Math.round(baseB * 255)})`;
      barSpeed.style.backgroundColor = gaugeColor;
      barAmp.style.backgroundColor = gaugeColor;
      barSuction.style.backgroundColor = gaugeColor;
      barRot.style.backgroundColor = gaugeColor;

      const time = clock.getElapsedTime() * currNoiseSpeed;
      const posAttr = geometry.attributes.position;
      const colAttr = geometry.attributes.color;
      const posArray = posAttr.array;
      const colArray = colAttr.array;

      for (let i = 0; i < particleCount; i++) {
        const idx = i * 3;
        const ox = origPositions[idx];
        const oy = origPositions[idx + 1];
        const oz = origPositions[idx + 2];

        const noiseVal = simplex.noise3D(
          (ox * 0.02) + time * 0.2,
          (oy * 0.02) + time * 0.2,
          (oz * 0.02) + time * 0.2
        );

        const displacement = 1.0 + (noiseVal * currNoiseAmp);
        const suctionPull = 1.0 - (currSuction * 0.45 * (1.0 - Math.abs(noiseVal)));
        const finalScale = displacement * suctionPull;

        posArray[idx] = ox * finalScale;
        posArray[idx + 1] = oy * finalScale;
        posArray[idx + 2] = oz * finalScale;

        // Dynamic Luminance Variation with Base Color Spectrum
        const pNorm = (noiseVal + 1.0) * 0.5;
        colArray[idx]     = Math.min(1.0, baseR + pNorm * 0.12);
        colArray[idx + 1] = Math.min(1.0, baseG + pNorm * 0.08);
        colArray[idx + 2] = Math.min(1.0, baseB + pNorm * 0.06);
      }

      posAttr.needsUpdate = true;
      colAttr.needsUpdate = true;"""

updated = []
for f in files_to_update:
    if os.path.exists(f):
        with open(f, 'r', encoding='utf-8') as fp:
            content = fp.read()
        if target_pattern.search(content):
            new_content = target_pattern.sub(replacement_code, content)
            with open(f, 'w', encoding='utf-8') as fp:
                fp.write(new_content)
            updated.append(f)

print(f"Updated {len(updated)} files: {updated}")
