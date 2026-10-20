#include<bits/stdc++.h>
#define endl '\n'
#define ll long long
using namespace std;

//-- BSGS（Baby-Step Giant-Step）：解 a^x ≡ b (mod p)，要求 gcd(a, p) = 1 --

ll exgcd(ll a, ll b, ll &x, ll &y)
{
	if(b == 0) { x = 1; y = 0; return a; }
	ll g = exgcd(b, a % b, y, x);
	y -= a / b * x;
	return g;
}

// 用法：inv_mod(a, p) 返回 a 在模 p 下的逆元，要求 gcd(a, p) = 1
ll inv_mod(ll a, ll p)
{
	ll x, y;
	exgcd(a, p, x, y);
	return (x % p + p) % p;
}

// 用法：bsgs(a, b, p) 返回最小的非负整数 x 使 a^x ≡ b (mod p)，无解返回 -1。
// 前提 gcd(a, p) = 1（p 不一定是素数，只要互质即可）；不互质请用 exBSGS 版。
// 复杂度 O(sqrt(p)) 时间与空间；a、b 先对 p 取模，p <= 2^31 时乘法不会溢出 long long。
ll bsgs(ll a, ll b, ll p)
{
	a %= p; b %= p;
	if(p == 1) return b == 0 ? 0 : -1; //模 1 时所有数都是 0
	if(b == 1) return 0; //a^0 = 1
	ll m = (ll)sqrt((double)p) + 1; //块长，x = i * m + j，0 <= j < m
	unordered_map<ll, int> baby;
	baby.reserve((size_t)m * 2);
	ll e = 1;
	for(ll j = 0; j < m; j ++) //小步：把 a^j 全部塞进哈希表，保留最小的 j
	{
		if(baby.find(e) == baby.end()) baby[e] = (int)j;
		e = e * a % p;
	}
	//e 此时是 a^m；大步：b * (a^{-m})^i，命中 a^j 就得到 x = i * m + j
	ll inv_am = inv_mod(e, p);
	ll cur = b;
	for(ll i = 0; i <= m; i ++)
	{
		unordered_map<ll, int>::iterator it = baby.find(cur);
		if(it != baby.end()) return i * m + it->second; //i 从小到大扫，第一次命中就是最小解
		cur = cur * inv_am % p;
	}
	return -1;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	ll p, base, target;
	cin >> p >> base >> target; //题目顺序：模数 p、底数、目标值（原题记作 p, b, n）
	ll x = bsgs(base, target, p);
	if(x < 0) cout << "no solution" << endl;
	else cout << x << endl;
	return 0;
}
/*
Input:
5 2 3

Output:
3
*/
