import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# ─────────────────────────────────────────────
#  ARCHITECTURE MODELE #82 (reference, sans altitude)
#  Input: 3 features | Dense(64) | Dense(32) | Output: 1
# ─────────────────────────────────────────────

LAYERS = [3, 12, 8, 1]   # representatif de 3 -> 64 -> 32 -> 1
LABELS_INPUT  = ['Temp', 'Hum', 'Pres']
LABELS_OUTPUT = ['Pluie /\nBeau temps']
LAYER_NAMES   = ['Input Layer\n(3 Features)', 'Hidden Layer 1\n(64 neurons)',
                 'Hidden Layer 2\n(32 neurons)', 'Output Layer\n(Prediction)']

fig, ax = plt.subplots(figsize=(14, 9))
fig.patch.set_facecolor('white')
ax.set_facecolor('white')
ax.axis('off')

x_positions = [0.12, 0.38, 0.65, 0.88]

col_input  = '#4fc3f7'
col_hidden = '#4fc3f7'
col_output = '#ff8a80'


def get_y_positions(n, y_center=0.5, spacing=0.07):
    total = (n - 1) * spacing
    return [y_center - total / 2 + i * spacing for i in range(n)]


node_positions = []
for i, n in enumerate(LAYERS):
    if i == 0:
        spacing = 0.09
    elif i == 1:
        spacing = 0.065
    elif i == 2:
        spacing = 0.075
    else:
        spacing = 0.09
    node_positions.append(get_y_positions(n, spacing=spacing))

for layer_idx in range(len(LAYERS) - 1):
    x1 = x_positions[layer_idx]
    x2 = x_positions[layer_idx + 1]
    for y1 in node_positions[layer_idx]:
        for y2 in node_positions[layer_idx + 1]:
            ax.plot([x1, x2], [y1, y2],
                    color='#cccccc', linewidth=0.5, alpha=0.6, zorder=1)

for layer_idx, (x, ys) in enumerate(zip(x_positions, node_positions)):
    if layer_idx == 0:
        color = col_input
    elif layer_idx == len(LAYERS) - 1:
        color = col_output
    else:
        color = col_hidden

    for j, y in enumerate(ys):
        ellipse = mpatches.Ellipse(
            (x, y), width=0.055, height=0.048,
            facecolor=color, edgecolor='#555555',
            linewidth=1.2, zorder=3
        )
        ax.add_patch(ellipse)

        if layer_idx == 0:
            ax.text(x - 0.07, y, LABELS_INPUT[j],
                    ha='right', va='center',
                    fontsize=13, fontweight='bold', color='#222222')

        if layer_idx == len(LAYERS) - 1:
            ax.text(x + 0.07, y, LABELS_OUTPUT[j],
                    ha='left', va='center',
                    fontsize=13, fontweight='bold', color='#222222')

for layer_idx in [1, 2]:
    x = x_positions[layer_idx]
    ys = node_positions[layer_idx]
    n_real = 64 if layer_idx == 1 else 32
    ax.text(x, max(ys) + 0.045, f'({n_real} neurones)',
            ha='center', va='bottom', fontsize=9,
            color='#666666', style='italic')

for i, (x, name) in enumerate(zip(x_positions, LAYER_NAMES)):
    ax.text(x, 0.03, name,
            ha='center', va='bottom',
            fontsize=11, fontweight='bold', color='#333333')

ax.set_title('Neural Network Architecture: Weather Prediction Model\n'
             'ADAM | RMS loss | ReLU | batch=64 | lr=0.01 | Train acc=82.5%',
             fontsize=14, fontweight='bold', color='#111111', pad=20)

for i, (x1, x2) in enumerate(zip(x_positions[:-1], x_positions[1:])):
    xm = (x1 + x2) / 2
    label = 'ReLU' if i < 2 else 'Sigmoid'
    ax.text(xm, 0.96, label,
            ha='center', va='top',
            fontsize=10, color='#0077aa',
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#e8f4fd',
                      edgecolor='#0077aa', linewidth=1))

ax.set_xlim(0, 1)
ax.set_ylim(0, 1)

plt.tight_layout()
plt.savefig('architecture_reseau.png', dpi=150, bbox_inches='tight', facecolor='white')
plt.close()
print("OK : architecture_reseau.png regenere avec 3 features (sans altitude)")
