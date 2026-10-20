#include<bits/stdc++.h>
#define maxn 1000005
#define endl '\n'
#define ll long long
using namespace std;

//-- 欧拉函数 φ(n)：1..n 中与 n 互质的数的个数 --

// 用法：phi_single(n) 单点求值，O(sqrt n)，n 可达 1e12 左右（再大就要换 Pollard-Rho）。
// 注意 n = 1 时返回 1（按定义 φ(1) = 1）。
ll phi_single(ll n)
{
	ll res = n;
	for(ll i = 2; i * i <= n; i ++) //只试到 sqrt(n)，剩下的 n > 1 必然是素数
		if(n % i == 0)
		{
			res = res / i * (i - 1); //先除后乘，避免 res * i 溢出
			while(n % i == 0) n /= i;
		}
	if(n > 1) res = res / n * (n - 1);
	return res;
}

// 用法：phi_sieve(N) 之后 phi[1..N] 就是欧拉函数表，O(N)。
// 多测直接重复调用即可（内部会清空 comp 与素数表）。
int phi[maxn], primes[maxn / 10], pcnt;
bool comp[maxn];

void phi_sieve(int N)
{
	pcnt = 0;
	for(int i = 1; i <= N; i ++) comp[i] = false;
	phi[1] = 1;
	for(int i = 2; i <= N; i ++)
	{
		if(!comp[i]) //i 是素数：φ(p) = p - 1
		{
			primes[++ pcnt] = i;
			phi[i] = i - 1;
		}
		for(int j = 1; j <= pcnt && (ll)i * primes[j] <= N; j ++)
		{
			int m = i * primes[j];
			comp[m] = true;
			if(i % primes[j] == 0)
			{
				//p 已是 i 的最小质因子：φ(i * p) = φ(i) * p
				phi[m] = phi[i] * primes[j];
				break;
			}
			//p 不整除 i：φ 是积性函数，φ(i * p) = φ(i) * (p - 1)
			phi[m] = phi[i] * (primes[j] - 1);
		}
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	ll n;
	int N;
	cin >> n >> N;
	cout << phi_single(n) << endl;
	phi_sieve(N);
	for(int i = 1; i <= N; i ++) cout << phi[i] << " \n"[i == N];
	return 0;
}
/*
Input:
12 10

Output:
4
1 1 2 2 4 2 6 4 6 4
*/
