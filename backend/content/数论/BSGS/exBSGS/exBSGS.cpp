#include<bits/stdc++.h>
#define endl '\n'
#define ll long long
using namespace std;

//-- 扩展 BSGS：解 a^x ≡ b (mod p)，不要求 gcd(a, p) = 1 --

ll exgcd(ll a, ll b, ll &x, ll &y)
{
	if(b == 0) { x = 1; y = 0; return a; }
	ll g = exgcd(b, a % b, y, x);
	y -= a / b * x;
	return g;
}

ll gcd2(ll a, ll b) { return b == 0 ? a : gcd2(b, a % b); }

ll inv_mod(ll a, ll p) //要求 gcd(a, p) = 1
{
	ll x, y;
	exgcd(a, p, x, y);
	return (x % p + p) % p;
}

// 标准 BSGS，只在 gcd(a, p) = 1 时调用
ll bsgs(ll a, ll b, ll p)
{
	a %= p; b %= p;
	if(p == 1) return b == 0 ? 0 : -1;
	if(b == 1) return 0;
	ll m = (ll)sqrt((double)p) + 1;
	unordered_map<ll, int> baby;
	baby.reserve((size_t)m * 2);
	ll e = 1;
	for(ll j = 0; j < m; j ++)
	{
		if(baby.find(e) == baby.end()) baby[e] = (int)j;
		e = e * a % p;
	}
	ll inv_am = inv_mod(e, p), cur = b;
	for(ll i = 0; i <= m; i ++)
	{
		unordered_map<ll, int>::iterator it = baby.find(cur);
		if(it != baby.end()) return i * m + it->second;
		cur = cur * inv_am % p;
	}
	return -1;
}

// 用法：exbsgs(a, b, p) 返回最小的非负整数 x 使 a^x ≡ b (mod p)，无解返回 -1。
// 思路：反复把 gcd(a, p) 除掉，每次给等式两边同除 g，记下已提出来的因子 mul = a^k / g...
//       直到 a 与模数互质，再用普通 BSGS 解出剩余指数，答案加回 k。
// 复杂度 O(sqrt(p))；p <= 1e9 时中间乘积在 long long 内。
ll exbsgs(ll a, ll b, ll p)
{
	a %= p; b %= p;
	if(b == 1 || p == 1) return 0; //a^0 = 1，模 1 时所有数都为 0
	ll k = 0, mul = 1; //mul = a^k / (每次除掉的 g 的乘积)
	while(true)
	{
		ll g = gcd2(a, p);
		if(g == 1) break;
		if(b % g) return -1; //g 不整除 b，方程无解
		b /= g; p /= g;
		mul = mul * (a / g) % p;
		k ++;
		if(mul == b) return k; //已经凑出来了，不必再往下除
		if(p == 1) return k;
	}
	ll y = bsgs(a, b * inv_mod(mul, p) % p, p);
	if(y < 0) return -1;
	return y + k;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	ll a, p, b;
	while(cin >> a >> p >> b)
	{
		if(a == 0 && p == 0 && b == 0) break; //题目约定 0 0 0 结束
		ll x = exbsgs(a, b, p);
		if(x < 0) cout << "No Solution" << endl;
		else cout << x << endl;
	}
	return 0;
}
/*
Input:
5 58 33
2 4 3
0 0 0

Output:
9
No Solution
*/
