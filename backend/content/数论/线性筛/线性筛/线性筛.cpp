#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 10000010
#define endl '\n'
#define ll long long
using namespace std;

int n;
int primes[maxn / 10];	//1e7 内素数 664579 个，maxn / 10 够用
int phi[maxn];			//φ(i) <= i <= 1e7，int 够
signed char mu[maxn];	//μ 只取 -1/0/1，1 字节就够
signed char cnt_p[maxn];//最小质因子的指数，<= 23；同时 0 表示 i 还没被筛过（素数）
int d[maxn];			//约数个数
ll J2[maxn];			//J2(i) < i^2，1e7 时约 1e14，必须 ll

int pcnt;
ll sum_phi, sum_mu, sum_d;
__int128 sum_j2;		//ΣJ2 在 n = 1e7 时约 2e20，超 long long

void print128(__int128 x)
{
	if(x == 0) { cout << 0; return; }
	if(x < 0) { cout << '-'; x = -x; }
	string s;
	while(x > 0) { s += char('0' + (int)(x % 10)); x /= 10; }
	reverse(s.begin(), s.end());
	cout << s;
}

// 空间量级（n = 1e7）：phi 40MB + d 40MB + J2 80MB + mu 10MB + cnt_p 10MB + primes 4MB ≈ 184MB
// 若 5 个函数值全开 ll 就是 400MB+，所以 mu / cnt_p 压成 signed char，d / phi 用 int
void linear_sieve(int n)
{
	phi[1] = 1; mu[1] = 1; d[1] = 1; J2[1] = 1; //J2(1) = 1^2 * 空积 = 1
	for(int i = 2; i <= n; i ++)
	{
		if(cnt_p[i] == 0) //没被任何质数筛到，i 是素数
		{
			primes[++ pcnt] = i;
			phi[i] = i - 1;
			mu[i] = -1;
			d[i] = 2;
			cnt_p[i] = 1;
			J2[i] = (ll)i * i - 1; //J2(p) = p^2 * (1 - 1/p^2) = p^2 - 1
		}
		for(int j = 1; j <= pcnt; j ++)
		{
			int p = primes[j];
			if((ll)i * p > n) break;
			int m = i * p;
			if(i % p == 0)
			{
				//p 已是 i 的最小质因子，m 只是在 i 的最小质因子指数上 +1
				phi[m] = phi[i] * p;
				mu[m] = 0;					//含平方因子 p^2
				cnt_p[m] = cnt_p[i] + 1;
				d[m] = d[i] / (cnt_p[i] + 1) * (cnt_p[i] + 2); //d 用最小质因子次数递推：指数 e -> e+1
				J2[m] = J2[i] * p * p;		//J2(p^k) = p^(2k) - p^(2k-2)，再多乘一个 p 就是再乘 p^2
				break;						//每个合数只被最小质因子筛一次，保证 O(n)
			}
			phi[m] = phi[i] * (p - 1);
			mu[m] = -mu[i];					//μ 是积性函数，p 不整除 i 时直接乘 μ(p) = -1
			cnt_p[m] = 1;
			d[m] = d[i] * 2;
			J2[m] = J2[i] * ((ll)p * p - 1); //J2 也积性，p 不整除 i 时乘 J2(p)
		}
	}
	//边筛边只累加 5 个和，省掉 5 个 ll 前缀和数组（n = 1e7 时每个 80MB）
	for(int i = 1; i <= n; i ++)
	{
		sum_phi += phi[i];
		sum_mu += mu[i];
		sum_d += d[i];
		sum_j2 += J2[i];
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	linear_sieve(n);
	cout << pcnt << endl;
	cout << sum_phi << endl;
	cout << sum_mu << endl;
	cout << sum_d << endl;
	print128(sum_j2); cout << endl;
	return 0;
}
/*
Input:
10

Output:
4
32
-1
27
312
*/
