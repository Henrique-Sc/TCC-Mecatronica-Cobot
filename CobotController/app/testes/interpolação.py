import matplotlib.pyplot as plt
import numpy as np

# Ponto inicial
x0, y0 = 0, 0

# Comprimentos
L1 = 5
L2 = 3
L3 = 1

# Ângulos originais
angle1_orig = 60
angle2_orig = 300

# Função para calcular posição final
def calc_pos(angle1, angle2):
    x1 = x0 + L1 * np.cos(np.radians(angle1))
    y1 = y0 + L1 * np.sin(np.radians(angle1))
    x2 = x1 + L2 * np.cos(np.radians(angle2))
    y2 = y1 + L2 * np.sin(np.radians(angle2))
    x3 = x2
    y3 = y2 - L3
    return x3, y3

# Ponto final original
x3_orig, y3_orig = calc_pos(angle1_orig, angle2_orig)
x3_target = x3_orig + 1

# Busca por ângulos que resultem em x3_target e y3 próximo
best_score = float('inf')
best_angles = (angle1_orig, angle2_orig)

for a1 in range(angle1_orig - 10, angle1_orig + 10):
    for a2 in range(angle2_orig - 10, angle2_orig + 10):
        x3, y3 = calc_pos(a1, a2)
        score = abs(x3 - x3_target) + abs(y3 - y3_orig)  # soma dos erros
        if score < best_score:
            best_score = score
            best_angles = (a1, a2)

angle1_new, angle2_new = best_angles
x3_new, y3_new = calc_pos(angle1_new, angle2_new)

# --- Braço original ---
x1 = x0 + L1 * np.cos(np.radians(angle1_orig))
y1 = y0 + L1 * np.sin(np.radians(angle1_orig))
x2 = x1 + L2 * np.cos(np.radians(angle2_orig))
y2 = y1 + L2 * np.sin(np.radians(angle2_orig))
x3 = x2
y3 = y2 - L3

# --- Braço novo ---
x1_new = x0 + L1 * np.cos(np.radians(angle1_new))
y1_new = y0 + L1 * np.sin(np.radians(angle1_new))
x2_new = x1_new + L2 * np.cos(np.radians(angle2_new))
y2_new = y1_new + L2 * np.sin(np.radians(angle2_new))
x3_new = x2_new
y3_new = y2_new - L3

# --- Visualização ---
plt.figure(figsize=(6,6))
plt.plot([x0, x1], [y0, y1], 'ro-', linewidth=4, label='Original')
plt.plot([x1, x2], [y1, y2], 'ro-', linewidth=4)
plt.plot([x2, x3], [y2, y3], 'ro-', linewidth=4)

plt.plot([x0, x1_new], [y0, y1_new], 'bo-', linewidth=2, label='Novo (+1 X)')
plt.plot([x1_new, x2_new], [y1_new, y2_new], 'bo-', linewidth=2)
plt.plot([x2_new, x3_new], [y2_new, y3_new], 'bo-', linewidth=2)

plt.plot(x3, y3, 'rx', markersize=10)
plt.plot(x3_new, y3_new, 'bx', markersize=10)

plt.xlim(-6, 6)
plt.ylim(-6, 6)
plt.grid(True)
plt.gca().set_aspect('equal', adjustable='box')
plt.title(f'Movimento Horizontal: X+1\nÂngulos novos: {angle1_new}°, {angle2_new}°')
plt.legend()
plt.show()
