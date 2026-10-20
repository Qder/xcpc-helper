#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define endl '\n'
#define ll long long
using namespace std;

int T;
ll a, b, m;

// 返回 gcd(a,b)，并解出 a * x + b * y = gcd(a,b)
ll exgcd(ll a, ll b, ll &x, ll &y)
{
	if(b == 0) { x = 1; y = 0; return a; }
	ll g = exgcd(b, a % b, y, x); //交换 x,y，少写一行中间变量
	y -= a / b * x;
	return g;
}

// 用法：求 a*x ≡ b (mod m) 的最小非负解，无解返回 -1
ll linear_congruence(ll a, ll b, ll m)
{
	if(a == 0) return b % m ? -1 : 0; //a=0 时方程退化成 0 ≡ b (mod m)：b 是 m 的倍数则任意 x 都行，取 x=0
	ll x, y;
	ll g = exgcd(a, m, x, y);
	if(b % g) return -1;	//裴蜀定理：g | b 才有解，否则 ax + my = b 无整数解
	ll mod = m / g;			//解在模 m/g 下唯一，最小非负解就落在 [0, m/g)
	x = (x % mod + mod) % mod;
	return (ll)((__int128)x * (b / g) % mod); //x 乘上 b/g 再取模，防溢出
}

// 用法：inv(a, m) 求 a 在模 m 下的逆元，要求 gcd(a,m)=1，不存在返回 -1
ll inv(ll a, ll m)
{
	if(m == 1) return 0; //模 1 时任何数都和 0 同余，逆元取 0
	ll x, y;
	ll g = exgcd(a, m, x, y);
	if(g != 1) return -1;
	return (x % m + m) % m;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> T;
	while(T --)
	{
		cin >> a >> b >> m;
		cout << linear_congruence(a, b, m) << endl;
	}
	return 0;
}
/*
Input:
6
2 1 3
4 2 6
4 3 6
0 0 7
0 3 7
5 0 9

Output:
2
2
-1
0
-1
0
*/
