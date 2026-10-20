#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100005
#define mod 1000000007
#define endl '\n'
#define ll long long
using namespace std;

int n, rk; //rk = 秩 = 基向量个数
int d[31]; //d[i] = 最高位恰好在第 i 位的基向量；a_i < 2^31，所以 0..30 位就够
int p[31], tot; //rebuild 之后的基，p[0..tot-1] 按主元从低到高排

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

// 用法：insert(x)  // 返回 true 表示 x 贡献了一个新基向量；x 被消成 0 时返回 false
bool insert(int x)
{
	for(int i = 30; i >= 0; i --)
	{
		if(!(x >> i & 1)) continue; //第 i 位是 0，这个基向量消不掉这一位
		if(!d[i]) { d[i] = x; rk ++; return true; }
		x ^= d[i];
	}
	return false; //被消成 0：0 不进基，rk 也不加，只说明原集合线性相关
}

// 用法：query_max()  // 子集异或最大值；高位贪心，第 i 位的权比所有低位之和还大，能变大就异或
int query_max()
{
	int res = 0;
	for(int i = 30; i >= 0; i --)
		if((res ^ d[i]) > res) res ^= d[i];
	return res;
}

// 化标准型：每个基向量只在自己的主元位有 1，之后才能按二进制拆 k
void rebuild()
{
	for(int i = 30; i >= 0; i --)
		for(int j = i - 1; j >= 0; j --)
			if((d[i] >> j & 1)) d[i] ^= d[j];
	tot = 0;
	for(int i = 0; i <= 30; i ++) if(d[i]) p[tot ++] = d[i];
}

// 用法：kth_min(k)  // 第 k 小异或值，k 从 1 开始（空集异或出的 0 恒为第 1 小），超出 2^rank 返回 -1
ll kth_min(ll k)
{
	if(k <= 0) return -1;
	rebuild();
	if(k > (1LL << tot)) return -1;
	k --; //rank 个基向量能异或出 2^rank 个数（含 0），0 排第一，先扣掉它再按二进制拆
	ll res = 0;
	for(int i = 0; i < tot; i ++)
		if((k >> i) & 1) res ^= p[i];
	return res;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) { int x; cin >> x; insert(x); }
	cout << query_max() << endl;
	cout << qpow(2, rk) << endl; //不同异或值个数 = 2^rank
	return 0;
}
/*
Input:
6
0 3 3 5 6 7

Output:
7
8
*/
