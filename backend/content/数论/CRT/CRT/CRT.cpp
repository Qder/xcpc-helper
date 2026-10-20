#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 15
#define endl '\n'
#define ll long long
using namespace std;

int n;
ll a[maxn], m[maxn];

ll exgcd(ll a, ll b, ll &x, ll &y)
{
	if(b == 0) { x = 1; y = 0; return a; }
	ll g = exgcd(b, a % b, y, x);
	y -= a / b * x;
	return g;
}

// 用法：ex_crt() 解 x ≡ a[i] (mod m[i])，模数任意（不要求互质），无解返回 -1
// 答案本身要落回 long long：这里默认 lcm(所有 m[i]) 不超 long long，中间乘积用 __int128 接
ll ex_crt()
{
	ll ans = 0, M = 1; //当前解为 x ≡ ans (mod M)
	for(int i = 1; i <= n; i ++)
	{
		ll x, y;
		ll g = exgcd(M, m[i], x, y);
		ll diff = ((a[i] % m[i] - ans % m[i]) % m[i] + m[i]) % m[i]; //要凑出来的差 a[i] - ans
		if(diff % g) return -1;	//g 不整除差：两个同余式矛盾
		ll mod = m[i] / g;		//k 的周期是 m[i]/g，不是 m[i]
		ll k = (ll)((__int128)(x % mod + mod) % mod * (diff / g) % mod);
		__int128 lcm = (__int128)M * mod; //lcm(M, m[i]) = M / g * m[i]，先乘可能超 ll
		ans = (ll)(((__int128)k * M + ans) % lcm);
		M = (ll)lcm;
	}
	return (ll)(((__int128)ans % M + M) % M); //ans 已在 [0,M) 内，但 M 近 1e18 时 ans + M 会溢出，用 __int128 收尾
}

ll inv(ll a, ll m)
{
	ll x, y;
	exgcd(a, m, x, y);
	return (x % m + m) % m;
}

// 用法：模数两两互质时用经典 CRT 构造，返回 x；此时 ∏m[i] 也要在 long long 内
ll crt_coprime()
{
	ll M = 1, ans = 0;
	for(int i = 1; i <= n; i ++) M *= m[i]; //M = ∏m[i]
	for(int i = 1; i <= n; i ++)
	{
		ll Mi = M / m[i];
		ll ti = inv(Mi % m[i], m[i]); //Mi 在模 m[i] 下的逆元
		ll term = (ll)((__int128)a[i] * Mi % M * ti % M);
		ans = (ll)(((__int128)ans + term) % M); //M 接近 1e18 时 ans + term 会超 ll
	}
	return (ll)(((__int128)ans % M + M) % M); //同上，收尾的 + M 也要用 __int128
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i] >> m[i];
	cout << ex_crt() << endl;
	return 0;
}
/*
Input:
3
2 3
3 5
2 7

Output:
23
*/
