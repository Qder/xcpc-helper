#include<bits/stdc++.h>
#define maxn 205
#define mod 1000000007
#define endl '\n'
#define ll long long
using namespace std;

int n;
int a[maxn][maxn], b[maxn]; //原矩阵 A 与右端常数 b（都已化到 [0, mod)）
int m[maxn][maxn], aug[maxn][maxn]; //m 是 det 的副本，aug 是 [A | b]
ll x[maxn];

ll qpow(ll a, ll b)
{
	ll res = 1;
	while(b)
	{
		if(b & 1) res = res * a % mod;
		a = a * a % mod;
		b >>= 1;
	}
	return res;
}

int norm(ll v)
{
	v %= mod;
	if(v < 0) v += mod;
	return (int)v;
}

// 模意义下高斯消元求行列式：交换两行 det 变号，把一行乘 k 加到另一行 det 不变。
// p = 1e9+7 是素数，非 0 主元都有逆元，所以消元时直接乘逆元即可。
ll det()
{
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= n; j ++) m[i][j] = a[i][j];
	ll res = 1;
	for(int i = 1; i <= n; i ++)
	{
		int p = i;
		while(p <= n && m[p][i] == 0) p ++; //主元为 0，找下面第一个非 0 行
		if(p > n) return 0;
		if(p != i)
		{
			for(int j = 1; j <= n; j ++) swap(m[i][j], m[p][j]);
			res = (mod - res) % mod; //换一行，行列式变号
		}
		res = res * m[i][i] % mod;
		ll inv = qpow(m[i][i], mod - 2);
		for(int r = i + 1; r <= n; r ++)
		{
			if(m[r][i] == 0) continue;
			ll f = (ll)m[r][i] * inv % mod;
			for(int j = i; j <= n; j ++)
				m[r][j] = norm(m[r][j] - f * m[i][j]);
		}
	}
	return res;
}

// 返回 1 表示唯一解，0 表示多解，-1 表示无解。
// 模意义下秩 < n 且没有矛盾行时，自由元可以任取，所以一定是无穷多解。
int gauss()
{
	for(int i = 1; i <= n; i ++)
	{
		for(int j = 1; j <= n; j ++) aug[i][j] = a[i][j];
		aug[i][n + 1] = b[i];
	}
	int rank = 0;
	for(int col = 1; col <= n; col ++)
	{
		int p = -1;
		for(int i = rank + 1; i <= n; i ++)
			if(aug[i][col] != 0) { p = i; break; }
		if(p == -1) continue; //这一列全是 0，是自由元
		rank ++;
		if(p != rank)
			for(int j = 1; j <= n + 1; j ++) swap(aug[rank][j], aug[p][j]);
		ll inv = qpow(aug[rank][col], mod - 2);
		for(int r = rank + 1; r <= n; r ++)
		{
			if(aug[r][col] == 0) continue;
			ll f = (ll)aug[r][col] * inv % mod;
			for(int j = col; j <= n + 1; j ++)
				aug[r][j] = norm(aug[r][j] - f * aug[rank][j]);
		}
	}
	for(int i = 1; i <= n; i ++) //出现全 0 行且 b 非 0：方程矛盾
	{
		bool zero = true;
		for(int j = 1; j <= n; j ++)
			if(aug[i][j] != 0) { zero = false; break; }
		if(zero && aug[i][n + 1] != 0) return -1;
	}
	if(rank < n) return 0; //秩 < n 且不矛盾：多解
	for(int i = n; i >= 1; i --) //唯一解，回代
	{
		ll s = aug[i][n + 1];
		for(int j = i + 1; j <= n; j ++)
			s = norm(s - (ll)aug[i][j] * x[j] % mod);
		x[i] = norm(s * qpow(aug[i][i], mod - 2) % mod);
	}
	return 1;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= n; j ++)
		{
			ll v; cin >> v;
			a[i][j] = norm(v);
		}
	for(int i = 1; i <= n; i ++)
	{
		ll v; cin >> v;
		b[i] = norm(v);
	}
	cout << det() << endl;
	int flag = gauss();
	if(flag == 1)
	{
		for(int i = 1; i <= n; i ++) cout << x[i] << (i == n ? '\n' : ' ');
	}
	else if(flag == -1) cout << "None" << endl;
	else cout << "Many" << endl;
	return 0;
}
/*
Input:
3
0 2 1
1 1 1
2 1 3
5
3
8

Output:
1000000004
1000000006 1 3
*/
