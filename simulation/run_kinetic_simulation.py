import numpy as np
import matplotlib.pyplot as plt
from scipy.integrate import solve_ivp

# ==============================================================================
# project_spec.md 명세서 반영: 36개 모터 2.5m 정삼각형 키네틱 데이터 아트 시뮬레이션
# ==============================================================================

# 1. 2.5m 정삼각형 및 36개 모터 삼각 격자 좌표 생성
S = 2.5                                # 정삼각형 한 변의 길이 (2.5m)
H = (np.sqrt(3) / 2) * S                # 높이 (약 2.165m)
L = 1.0                                # 유리섬유 길이 (1.0m)
BASE_ANGLE = 150.0                     # 기본 대기 각도 (150°)

num_rows = 8                           # 8개 행 (1+2+3+4+5+6+7+8 = 36개)
motors = []
for r in range(num_rows):
    count_in_row = r + 1
    row_y = H * (1 - r / (num_rows - 1))
    row_w = S * (r / (num_rows - 1))
    for i in range(count_in_row):
        row_x = 0.0 if count_in_row == 1 else -row_w / 2.0 + (row_w / (count_in_row - 1)) * i
        motors.append({'id': len(motors), 'x': row_x, 'y': row_y})

motors = np.array(motors)
print(f"✅ [성공] 2.5m 정삼각형 삼각 격자 내 총 {len(motors)}개 모터 좌표 생성 완료.")

# 2. 물리 파라미터 (1m 유리섬유 + 30g 종)
g = 9.81
d = 0.0025; r_rod = d / 2; A_rod = np.pi * r_rod**2; I_rod = np.pi * r_rod**4 / 4
E = 40e9; rho = 2000
m_bell = 0.030; m_rod = rho * A_rod * L

k_beam = (3 * E * I_rod) / (L**3)
k_eff = max(0.001, k_beam - (m_bell + 0.5 * m_rod) * g / L)
m_eq = m_bell + (33/140) * m_rod
zeta = 0.05
wn = np.sqrt(k_eff / m_eq)
c_eq = 2 * m_eq * wn * zeta
fn = wn / (2 * np.pi)

print(f"시스템 고유 진동수: {fn:.2f} Hz, 유효 강성: {k_eff:.4f} N/m")

# 3. 사망자 수 데이터 N에 따른 파도 시뮬레이션
def simulate_casualty_wave(N, t_end=4.0):
    # 사망자 데이터 N (50 ~ 1200명) 매핑
    norm = np.clip((N - 50) / (1200 - 50), 0, 1)
    amp = 5.0 + norm * 15.0               # 진폭: ±5° ~ ±20° (project_spec.md 반영)
    period = 2.8 - norm * 2.2             # 주기: 2.8s ~ 0.6s
    w = 2 * np.pi / period
    
    # 공간 파도 파수 k (파장 lambda = 1.2m)
    wavelength = 1.2
    k_space = 2 * np.pi / wavelength
    dir_rad = np.radians(45)              # 45도 방향 바람 파도
    kx = k_space * np.cos(dir_rad)
    ky = k_space * np.sin(dir_rad)
    
    t_eval = np.linspace(0, t_end, 400)
    motor_angles = np.zeros((len(motors), len(t_eval)))
    bell_positions = np.zeros((len(motors), len(t_eval)))
    
    for idx, m in enumerate(motors):
        phase = kx * m['x'] + ky * m['y']
        
        # 각 모터 조종 수식: theta_i(t) = 150° + Amp * sin(w*t - phase)
        target_angles = BASE_ANGLE + amp * np.sin(w * t_eval - phase)
        motor_angles[idx, :] = target_angles
        
        # 탄성 처짐 ODE
        def ode_func(t, y_state):
            x_def, v_def = y_state
            alpha = np.radians(amp * (-w**2 * np.sin(w * t - phase)))
            F_ext = -m_eq * L * alpha
            return [v_def, (F_ext - c_eq * v_def - k_eff * x_def) / m_eq]
            
        sol = solve_ivp(ode_func, [0, t_end], [0, 0], t_eval=t_eval, rtol=1e-5)
        
        base_pos_cm = L * np.sin(np.radians(target_angles - BASE_ANGLE)) * 100
        bell_positions[idx, :] = base_pos_cm + sol.y[0] * 100
        
    return t_eval, motor_angles, bell_positions, amp, period

# 4. 시뮬레이션 결과 시각화
t_eval, angles_low, bells_low, amp_low, period_low = simulate_casualty_wave(N=120)
t_eval, angles_high, bells_high, amp_high, period_high = simulate_casualty_wave(N=950)

fig, axes = plt.subplots(2, 2, figsize=(13, 8))
fig.suptitle('Project Spec: 36-Motor 2.5m Equilateral Triangle Spatial Kinetic Art Simulation', fontsize=13, fontweight='bold')

# (1) 사망자 적은 날 (N=120, 잔잔한 미풍 파도)
axes[0, 0].set_title(f'Low Casualty (N=120): Amp=±{amp_low:.1f}°, Period={period_low:.1f}s')
for i in range(0, 36, 4): # 주요 모터 샘플링
    axes[0, 0].plot(t_eval, angles_low[i, :], label=f'Motor #{i+1}')
axes[0, 0].set_ylabel('Motor Angle (°)')
axes[0, 0].grid(True)
axes[0, 0].legend(loc='upper right', fontsize=8)

axes[1, 0].set_title('Low Casualty: Top Bell Tip Spatial Wave Position (cm)')
for i in range(0, 36, 4):
    axes[1, 0].plot(t_eval, bells_low[i, :], label=f'Bell #{i+1}')
axes[1, 0].set_xlabel('Time (s)')
axes[1, 0].set_ylabel('Bell Pos (cm)')
axes[1, 0].grid(True)

# (2) 사망자 많은 날 (N=950, 강한 파도 & 거친 출렁임)
axes[0, 1].set_title(f'High Casualty (N=950): Amp=±{amp_high:.1f}°, Period={period_high:.1f}s')
for i in range(0, 36, 4):
    axes[0, 1].plot(t_eval, angles_high[i, :], label=f'Motor #{i+1}')
axes[0, 1].set_ylabel('Motor Angle (°)')
axes[0, 1].grid(True)

axes[1, 1].set_title('High Casualty: Top Bell Tip Spatial Wave Position (cm)')
for i in range(0, 36, 4):
    axes[1, 1].plot(t_eval, bells_high[i, :], label=f'Bell #{i+1}')
axes[1, 1].set_xlabel('Time (s)')
axes[1, 1].set_ylabel('Bell Pos (cm)')
axes[1, 1].grid(True)

plt.tight_layout()
plt.show()
