#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
#define ll long long
using namespace std;

ll fac[maxn], inv_fac[maxn];

ll qpow(ll x, ll n, ll p)
{
	ll res = 1 % p;
	while(n)
	{
		if(n & 1) res = res * x % p;
		x = x * x % p;
		n >>= 1;
	}
	return res;
}

// 用法：init_fac(p) 之后才能调 lucas(n, m, p)；p 必须是素数。
// 预处理 O(p)，单次组合数 O(log_p n)，总复杂度 O(p + log_p n)。
// 注意 p 大于 n + m 时 Lucas 只用一层，但仍然建议按 p 预处理。
void init_fac(ll p)
{
	fac[0] = 1;
	for(int i = 1; i < p; i ++) fac[i] = fac[i - 1] * i % p;
	inv_fac[p - 1] = qpow(fac[p - 1], p - 2, p); //费马小定理求阶乘逆元
	for(int i = p - 1; i >= 1; i --) inv_fac[i - 1] = inv_fac[i] * i % p;
}

// C(n, m) mod p，要求 n, m < p
ll comb_small(ll n, ll m, ll p)
{
	if(m > n) return 0;
	return fac[n] * inv_fac[m] % p * inv_fac[n - m] % p;
}

// 用法：lucas(n, m, p) 返回 C(n, m) mod p；p 为素数，n, m 可以远大于 p
ll lucas(ll n, ll m, ll p)
{
	if(m == 0) return 1 % p;
	return comb_small(n % p, m % p, p) * lucas(n / p, m / p, p) % p;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int t;
	cin >> t;
	while(t --)
	{
		ll n, m, p;
		cin >> n >> m >> p;
		init_fac(p);
		cout << lucas(n + m, n, p) << endl;
	}
	return 0;
}
/*
Input:
2
1 2 5
2 1 5

Output:
3
3
*/
