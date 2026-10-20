---
updated: '2026-10-20'
tags:
- 数论
- 欧拉函数
- 积性函数
priority: 4
difficulty: 4
---

欧拉函数 $\varphi(n)$：$1..n$ 中与 $n$ 互质的数的个数；包含单点试除与线性筛表两种写法。

## 核心思想

- 单点：对 $n$ 试除质因子，$\varphi(n) = n\prod_{p\mid n}(1 - 1/p)$，边除边乘避免溢出；
- 筛表：线性筛保证每个合数只被最小质因子筛一次：
  - $p \nmid i$ 时 $\varphi(ip) = \varphi(i)(p-1)$（积性）；
  - $p \mid i$ 时 $\varphi(ip) = \varphi(i)p$。

## 复杂度

- 单点：$O(\sqrt n)$；
- 筛表：$O(n)$ 时间、$O(n)$ 空间。

## 常用性质

- $\varphi(1)=1$，$\varphi(p)=p-1$，$\varphi(p^k)=p^k-p^{k-1}$；
- 积性：$\gcd(a,b)=1 \Rightarrow \varphi(ab)=\varphi(a)\varphi(b)$；
- $\sum_{d\mid n}\varphi(d)=n$；
- 欧拉定理：$\gcd(a,m)=1 \Rightarrow a^{\varphi(m)}\equiv 1 \pmod m$，可用于求逆元与降幂。

## 使用与易错点

- `phi_single` 的循环条件 `i * i <= n` 用 `long long`，$n$ 接近 $10^{12}$ 时不溢出；
- 先除后乘 `res / i * (i - 1)`，避免中间乘积超过 `long long`；
- `phi_sieve` 多测可以直接重复调用，内部会清空 `comp` 与素数表；
- 筛表的 `phi` 用 `int`，$n \le 10^7$ 时够用；再大要换 `long long` 或分段筛。

## 相关模板

- 数论/线性筛、数论/快速幂
