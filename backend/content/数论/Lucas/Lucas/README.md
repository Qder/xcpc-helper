---
updated: '2026-10-20'
tags:
- 数论
- 组合数
- Lucas
source: 洛谷 P3807
page: https://www.luogu.com.cn/problem/P3807
priority: 4
---

Lucas 定理：$p$ 为素数时求 $\binom{n}{m} \bmod p$，$n, m$ 可以远大于 $p$。

## 核心思想

- 把 $n, m$ 写成 $p$ 进制，逐位求小组合数再相乘：
  $$\binom{n}{m} \equiv \binom{n \bmod p}{m \bmod p} \cdot \binom{\lfloor n/p \rfloor}{\lfloor m/p \rfloor} \pmod p$$
- 小组合数 $\binom{a}{b}$（$a, b < p$）用阶乘与阶乘逆元 $O(1)$ 算；
- 递归到 $m = 0$ 为止，层数 $O(\log_p n)$。

## 复杂度

- 预处理 $O(p)$（阶乘与逆元），单次询问 $O(\log_p n)$，空间 $O(p)$。

## 使用与易错点

- 前提是 $p$ 为素数；合数模数要用 exLucas（扩展卢卡斯）；
- $p$ 可能小于 $n + m$，所以不能直接预处理到 $n + m$，必须按 $p$ 开数组；
- `fac` 只需要开到 $p-1$，`fac[p]` 在模 $p$ 下是 0，不能参与逆元；
- $m > n$ 时组合数为 0，函数里先判掉；
- 费马小定理求逆元要求 $p$ 为素数，与 Lucas 的前提一致；
- 多测每组都要按自己的 $p$ 重新 `init_fac(p)`。

## 相关模板

- 数论/乘法逆元与组合数、数论/快速幂
