---
updated: '2026-10-20'
tags:
- 数论
- 离散对数
- BSGS
source: 洛谷 P3846
page: https://www.luogu.com.cn/problem/P3846
priority: 4
---

BSGS 解离散对数 $a^x \equiv b \pmod p$，要求 $\gcd(a,p)=1$。

## 核心思想

- 令 $x = im + j$（$0 \le j < m$，$m = \lceil\sqrt p\rceil$），则 $a^j \equiv b \cdot a^{-im} \pmod p$；
- 小步：把 $a^0, a^1, \dots, a^{m-1}$ 存进哈希表，同值只保留最小的 $j$；
- 大步：从 $b$ 开始每次乘 $a^{-m}$，查表命中就得到 $x = im + j$；
- $i$ 从小到大扫，第一次命中即最小非负解。

## 复杂度

- 时间 $O(\sqrt p)$，空间 $O(\sqrt p)$。

## 使用与易错点

- 前提是 $\gcd(a,p)=1$（$p$ 不要求是素数）；不互质请用同目录的 exBSGS 版本；
- $a, b$ 先对 $p$ 取模；$b \equiv 1$ 时答案为 0（对应 $a^0$）；
- 注意题目输入顺序：P3846 按 `p b n` 给出，即模数、底数、目标值；
- $p \le 2^{31}$ 时乘法在 `long long` 内，不需要龟速乘；
- 哈希表用 `unordered_map`，`reserve` 一下避免反复 rehash。

## 相关模板

- 数论/exBSGS、数论/exGCD与线性同余
