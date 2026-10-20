#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxm 55
#define endl '\n'
#define ll long long
#define mod 1000000007
using namespace std;

//-- 2 × 2 版：求斐波那契 F(n) = F(n-1) + F(n-2)，F(0) = 0、F(1) = 1 --
// 转移矩阵 M = [[1, 1], [1, 0]]，有 [F(n+1), F(n)]^T = M^n [F(1), F(0)]^T
// 这里少乘一次，用 M^(n-1)，所以答案就是 (M^(n-1))[0][0]

struct Mat2
{
	ll a[2][2];
};

Mat2 mul2(const Mat2 &x, const Mat2 &y)
{
	Mat2 z;
	memset(z.a, 0, sizeof(z.a));
	for(int i = 0; i < 2; i ++)
		for(int k = 0; k < 2; k ++)
		{
			if(x.a[i][k] == 0) continue; //转移矩阵里只有一个 0，跳过能省掉四分之一的乘法
			for(int j = 0; j < 2; j ++)
				z.a[i][j] = (z.a[i][j] + x.a[i][k] * y.a[k][j]) % mod; //每步都取模：(1e9+7)^2 < 2^63，ll 不会溢出
		}
	return z;
}

// 用法：fib(n) 返回 F(n) mod 1e9+7，n 到 1e18 也是 O(log n)
ll fib(ll n)
{
	if(n == 0) return 0; //n = 0 必须特判：下面 k = n - 1 = -1，有符号右移永远到不了 0，会死循环
	Mat2 res, base;
	memset(res.a, 0, sizeof(res.a));
	res.a[0][0] = res.a[1][1] = 1; //单位阵不能省：写全 0 恒得 0，直接拿 base 当 res 会白乘一次 M
	base.a[0][0] = base.a[0][1] = base.a[1][0] = 1;
	base.a[1][1] = 0;
	ll k = n - 1;
	while(k)
	{
		if(k & 1) res = mul2(res, base); //二进制拆分：这一位是 1 就把 M^(2^i) 乘进来
		base = mul2(base, base);
		k >>= 1;
	}
	return res.a[0][0];
}

//-- 通用版：n × n 矩阵快速幂，n <= 50 --
// 用法：把矩阵填进 A[0..n-1][0..n-1]，调 mat_pow(n, k)，之后 A 就是 A^k（k = 0 得单位阵）
// 复杂度 O(n^3 log k)：n = 50 时是 50^3 * log k，k 到 1e18 也只要 50^3 * 60 ≈ 7.5e6 次乘法，随便跑
ll A[maxm][maxm], B[maxm][maxm], C[maxm][maxm], T[maxm][maxm];

// z = x * y：z 不能和 x、y 是同一块内存（x 和 y 可以相同，比如 B * B），所以自乘时借 T 中转
void mat_mul(ll x[][maxm], ll y[][maxm], ll z[][maxm], int n)
{
	for(int i = 0; i < n; i ++)
		for(int j = 0; j < n; j ++) z[i][j] = 0;
	for(int i = 0; i < n; i ++)
		for(int k = 0; k < n; k ++)
		{
			if(x[i][k] == 0) continue; //转移矩阵通常很稀疏，跳过 0 是常数级优化
			for(int j = 0; j < n; j ++)
				z[i][j] = (z[i][j] + x[i][k] * y[k][j]) % mod;
		}
}

void mat_pow(int n, ll k)
{
	for(int i = 0; i < n; i ++)
		for(int j = 0; j < n; j ++) C[i][j] = (i == j); //C 置单位阵，同 2 × 2 版一样不能省
	for(int i = 0; i < n; i ++)
		for(int j = 0; j < n; j ++) B[i][j] = A[i][j];
	while(k)
	{
		if(k & 1)
		{
			mat_mul(C, B, T, n);
			for(int i = 0; i < n; i ++)
				for(int j = 0; j < n; j ++) C[i][j] = T[i][j]; //结果先落在 T 里，再拷回去
		}
		mat_mul(B, B, T, n);
		for(int i = 0; i < n; i ++)
			for(int j = 0; j < n; j ++) B[i][j] = T[i][j];
		k >>= 1;
	}
	for(int i = 0; i < n; i ++)
		for(int j = 0; j < n; j ++) A[i][j] = C[i][j];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int T; cin >> T;
	while(T --)
	{
		ll n; cin >> n;
		cout << fib(n) << endl;
	}
	return 0;
}
/*
Input:
5
0
1
2
10
1000000000000000000

Output:
0
1
1
55
209783453
*/
