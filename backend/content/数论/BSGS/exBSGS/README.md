---
updated: '2026-10-20'
tags:
- 数论
- 离散对数
- BSGS
source: 洛谷 P4195
page: https://www.luogu.com.cn/problem/P4195
priority: 3
difficulty: 7
---

扩展 BSGS：解 $a^x \equiv b \pmod p$，不要求 $\gcd(a,p)=1$。

## 核心思想

- 每次令 $g = \gcd(a,p)$：若 $g \nmid b$ 则无解；
- 否则等式两边同除 $g$，模数变成 $p/g$，把提出的因子乘进 `mul`，次数记为 $k$；
- 重复到 $\gcd(a,p)=1$，此时用普通 BSGS 解 $a^y \equiv b \cdot mul^{-1} \pmod{p'}$；
- 答案 $x = y + k$。

## 复杂度

- 时间 $O(\sqrt p \log p)$，空间 $O(\sqrt p)$。

## 使用与易错点

- 输入按 P4195 约定 `a p b`，多测以 `0 0 0` 结束；
- 循环中若 `mul == b` 可以提前返回 $k$；
- 模数缩到 1 时直接返回 $k$（模 1 下所有数都是 0）；
- 逆元只在 $\gcd(mul,p)=1$ 之后使用，此时一定存在；
- `b` 要先对原模数取模，再进入除法循环。

## 相关模板

- 数论/BSGS、数论/exGCD与线性同余
