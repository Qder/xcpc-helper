#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1000005
#define mod 1000000007
#define endl '\n'
#define ll long long
using namespace std;

/*
//-- 1..n 的逆元线性递推：只要逆元、不要阶乘时用它，比 init_fac 省一半空间 --
ll inv[maxn];

void init_inv(int n)
{
	inv[1] = 1;
	for(int i = 2; i <= n; i ++) inv[i] = mod - mod / i * inv[mod % i] % mod; //mod 是质数时成立
}
*/

int T, n, k;
ll fac[maxn], inv_fac[maxn];

ll qpow(ll a, ll b, ll p) //p 作为参数，Lucas 换模数时能直接复用
{
	ll res = 1;
	while(b)
	{
		if(b & 1) res = res * a % p;
		a = a * a % p;
		b >>= 1;
	}
	return res;
}

// 逆元的三种求法：费马小定理 a^(p-2)（p 为质数，本模板用的就是它）；
// exgcd 解 ax+py=1（只要 gcd(a,p)=1，p 不质数也行）；线性递推（p 为质数，一次求 1..n，见文件上方注释块）
// 用法：init_fac(N) 之后 C(n,k) 就是 O(1)；只对 fac[N] 求一次逆元再倒推，比逐个快速幂少一个 log
void init_fac(int N)
{
	fac[0] = 1;
	for(int i = 1; i <= N; i ++) fac[i] = fac[i - 1] * i % mod;
	inv_fac[N] = qpow(fac[N], mod - 2, mod); //N < mod，所以 fac[N] 与 mod 互质，逆元一定存在
	for(int i = N; i >= 1; i --) inv_fac[i - 1] = inv_fac[i] * i % mod; //(i-1)! 的逆元 = i! 的逆元 * i
}

// 用法：cout << C(n, k);  要求 k <= n 且 n <= 预处理上界，越界返回 0
ll C(int n, int k)
{
	if(k < 0 || k > n) return 0;
	return fac[n] * inv_fac[k] % mod * inv_fac[n - k] % mod;
}

// Lucas：p 是小质数（p <= maxn）时用，n、k 可以到 1e18；内部按模数惰性建表，p 不变就不重建
int lucas_p = -1;
ll lfac[maxn], lifac[maxn];

void lucas_init(int p)
{
	if(lucas_p == p) return;
	lucas_p = p;
	lfac[0] = 1;
	for(int i = 1; i < p; i ++) lfac[i] = lfac[i - 1] * i % p;
	lifac[p - 1] = qpow(lfac[p - 1], p - 2, p);
	for(int i = p - 1; i >= 1; i --) lifac[i - 1] = lifac[i] * i % p;
}

// 用法：cout << lucas(n, k, p);  // 大组合数取模小质数，按 p 进制逐位拆，任一位 C(a,b)=0 就整串为 0
ll lucas(ll n, ll k, int p)
{
	if(k < 0 || k > n) return 0;
	if(k == 0) return 1;
	lucas_init(p);
	int a = n % p, b = k % p;
	if(b > a) return 0; //这一位的组合数就是 0，别再往下递归
	return lfac[a] * lifac[b] % p * lifac[a - b] % p * lucas(n / p, k / p, p) % p;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	init_fac(1000000); //一次预处理到 1e6，之后每个询问 O(1)
	cin >> T;
	while(T --)
	{
		cin >> n >> k;
		cout << C(n, k) << endl;
	}
	return 0;
}
/*
Input:
5
5 2
0 0
1000000 500000
10 10
6 3

Output:
10
1
996692777
1
20
*/
