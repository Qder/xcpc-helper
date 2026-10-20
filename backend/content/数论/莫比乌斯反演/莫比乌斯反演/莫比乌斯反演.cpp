#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1000005
#define endl '\n'
#define ll long long
using namespace std;

int T, n, m;
int primes[maxn], pcnt;
bool comp[maxn];
int mu[maxn]; //只取 -1 / 0 / 1，用 int 存是为了前缀和做差
ll smu[maxn]; //μ 的前缀和 M(x) = Σ_{i<=x} μ(i)

// 线性筛 μ：每个合数只被它的最小质因子筛一次；μ 是积性函数，
// i % p == 0 时 i*p 含平方因子 p²，μ = 0；否则 μ(i*p) = -μ(i)
void init_mu(int N)
{
	mu[1] = 1;
	for(int i = 2; i <= N; i ++)
	{
		if(!comp[i]) { primes[++ pcnt] = i; mu[i] = -1; }
		for(int j = 1; j <= pcnt && i * primes[j] <= N; j ++)
		{
			int t = i * primes[j];
			comp[t] = true;
			if(i % primes[j] == 0) { mu[t] = 0; break; }
			mu[t] = -mu[i];
		}
	}
	for(int i = 1; i <= N; i ++) smu[i] = smu[i - 1] + mu[i];
}

// 用法：cout << coprime_pairs(n, m);  // Σ_{i<=n} Σ_{j<=m} [gcd(i,j)==1]
// [gcd(i,j)==1] = Σ_{d|gcd(i,j)} μ(d)，交换求和号得 Σ_d μ(d)⌊n/d⌋⌊m/d⌋，块内 μ 的和取前缀和
ll coprime_pairs(int n, int m)
{
	if(n > m) swap(n, m); //上界必须取 min(n,m)：d 超过它就有一项整除值是 0，分块也对不齐
	ll res = 0;
	for(int l = 1, r; l <= n; l = r + 1)
	{
		r = min(n / (n / l), m / (m / l)); //两个右端点取 min，保证块内两个整除值都不变
		res += (smu[r] - smu[l - 1]) * (ll)(n / l) * (m / l);
	}
	return res;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	init_mu(1000000); //题目上限 1e6，多组询问就一次筛完 + 前缀和，每次询问才 O(√n)
	cin >> T;
	while(T --)
	{
		cin >> n >> m;
		cout << coprime_pairs(n, m) << endl;
	}
	return 0;
}
/*
Input:
4
3 3
1 1
10 7
100 100

Output:
7
1
47
6087
*/
