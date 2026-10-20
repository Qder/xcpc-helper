# 图 / 树布局函数库

骨架 [../visual-skeleton.html](../visual-skeleton.html) 已把下列函数放在 `/* === ALGO START === */` **之前**。生成 visual.html 时**只调用、不要改写、不要删除**。

用途：`edgeSvg` 画边（按半径缩短，可选箭头）；`svgBox` 算 viewBox；`textSvg` 给文字加描边光晕，专门用在边权标签上，避免和线重叠。

```javascript
function layoutCircle(n, cx, cy, r) {
  return Array.from({ length: n }, (_, i) => {
    const a = -Math.PI / 2 + (2 * Math.PI * i) / n;
    return { x: cx + r * Math.cos(a), y: cy + r * Math.sin(a) };
  });
}

function layoutBinary(root, left, right, dx = 44, dy = 60) {
  const pos = {};
  let k = 0;
  (function dfs(u, d) {
    if (u == null) return;
    dfs(left[u], d + 1);
    pos[u] = { x: k++ * dx, y: d * dy };
    dfs(right[u], d + 1);
  })(root, 0);
  return pos;
}

function layoutTree(root, children, dx = 48, dy = 64) {
  const pos = {};
  let leaf = 0;
  (function dfs(u, d) {
    const cs = children[u] || [];
    if (cs.length === 0) { pos[u] = { x: leaf++ * dx, y: d * dy }; return; }
    cs.forEach((c) => dfs(c, d + 1));
    pos[u] = { x: (pos[cs[0]].x + pos[cs[cs.length - 1]].x) / 2, y: d * dy };
  })(root, 0);
  return pos;
}

function edgeSvg(a, b, o = {}) {
  const { r = 18, directed = false, color = 'var(--faint)', width = 2, dash = '' } = o;
  const dx = b.x - a.x, dy = b.y - a.y, len = Math.hypot(dx, dy) || 1, ux = dx / len, uy = dy / len;
  const x1 = a.x + ux * r, y1 = a.y + uy * r, x2 = b.x - ux * r, y2 = b.y - uy * r;
  let s = '<line x1="' + x1 + '" y1="' + y1 + '" x2="' + x2 + '" y2="' + y2 + '" stroke="' + color +
    '" stroke-width="' + width + '"' + (dash ? ' stroke-dasharray="' + dash + '"' : '') + '/>';
  if (directed) {
    const hx = x2 - ux * 9, hy = y2 - uy * 9;
    s += '<polygon points="' + x2 + ',' + y2 + ' ' + (hx - uy * 5) + ',' + (hy + ux * 5) + ' ' +
      (hx + uy * 5) + ',' + (hy - ux * 5) + '" fill="' + color + '"/>';
  }
  return s;
}

function svgBox(points, pad = 30) {
  const xs = points.map((p) => p.x), ys = points.map((p) => p.y);
  const x0 = Math.min(...xs) - pad, y0 = Math.min(...ys) - pad;
  return x0 + ' ' + y0 + ' ' + (Math.max(...xs) + pad - x0) + ' ' + (Math.max(...ys) + pad - y0);
}

function textSvg(x, y, text, o = {}) {
  const { size = 12, weight = 400, fill = 'var(--text)', anchor = 'middle' } = o;
  return '<text x="' + x + '" y="' + y + '" font-size="' + size + '" font-weight="' + weight + '" text-anchor="' + anchor +
    '" fill="' + fill + '" stroke="var(--surface)" stroke-width="3" paint-order="stroke" stroke-linejoin="round">' + text + '</text>';
}
```
