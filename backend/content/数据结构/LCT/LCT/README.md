---
updated: '2026-10-20'
tags:
- 动态树
- LCT
- Splay
- 区间翻转
source: 洛谷 P3690
page: https://www.luogu.com.cn/problem/P3690
priority: 3
difficulty: 7
---

Link-Cut Tree 维护动态森林：支持加边、删边、路径查询与路径修改，均摊 $O(\log n)$。

## 核心思想

- 原树剖成若干条偏爱路径，每条路径用一棵 splay 维护，中序遍历就是路径上的顺序；
- `access(x)` 把根到 x 的路径变成一条偏爱路径，x 旋到辅助树根；
- `makeroot(x)` 在 `access` 后给整条路径打翻转标记，把 x 变成原树的根；
- `split(x, y)` = `makeroot(x)` + `access(y)`，之后 y 的子树信息就是路径信息；
- `link` / `cut` 先用 `findroot` 判连通，再接上或断开虚边。

## 复杂度

- 均摊时间 $O(\log n)$ 每次操作，$m$ 次操作 $O(m\log n)$；
- 空间 $O(n)$。

## 使用与易错点

- 点编号 1-based；多测要清空 `ch / fa / rev / xr`；
- `splay` 前必须从辅助树根往下 `push_down`，否则旋转时左右儿子的含义是反的；
- `link` 前先判 `findroot(y) != x`，否则会连出自环；
- `cut` 只对存在的边有效，判定条件是 `ch[y][0] == x && !ch[x][1]`；
- 修改点权要先 `access(x); splay(x)`，再 `push_up(x)`；
- 本模板维护的是点权异或和，换成求和/最值只需改 `push_up` 与查询用的 `xr[]`。

## 常见变形

- 维护边权：把边拆成点，点权即边权；
- 维护子树信息（虚子树大小等）：在 `access` 换右儿子时同步加减虚儿子贡献；
- 只有加边的动态连通性可以只用并查集，省掉 `findroot`。

## 相关模板

- 数据结构/平衡树（Splay、文艺平衡树）
