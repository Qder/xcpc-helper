#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define endl '\n'
#define ll long long
using namespace std;

int T;
ll n;

ll pow_mod(ll a, ll b, ll m)
{
	ll res = 1;
	a %= m;
	while(b)
	{
		if(b & 1) res = (ll)((__int128)res * a % m); //ll * ll 能到 1e36，必须 __int128 接一下
		a = (ll)((__int128)a * a % m);
		b >>= 1;
	}
	return res;
}

bool miller(ll n, ll a)
{
	ll d = n - 1;
	int s = 0;
	while((d & 1) == 0) { d >>= 1; s ++; } //n - 1 = d * 2^s，d 为奇数
	ll x = pow_mod(a, d, n);
	if(x == 1 || x == n - 1) return true;
	for(int i = 1; i < s; i ++) //再做 s - 1 次平方，中途见到 n - 1 才算过
	{
		x = (ll)((__int128)x * x % n);
		if(x == n - 1) return true;
		if(x == 1) return false; //出现非平凡平方根，n 必为合数
	}
	return false;
}

// 用法：is_prime(n) 对 n <= 1e18 是确定性的（不是概率）
bool is_prime(ll n)
{
	if(n < 2) return false;
	const int base[12] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
	for(int i = 0; i < 12; i ++)
	{
		if(n % base[i] == 0) return n == base[i]; //小素数自己，或直接被小素数整除
	}
	//前 9 个底数 2..23 已能保证 n < 3.8e18 无假阳性，这里取前 12 个覆盖 1e18 绰绰有余
	//（前 12 个素数的确定性上界到 3.3e23，所以 1e18 内不用随机底数）
	for(int i = 0; i < 12; i ++) if(!miller(n, base[i])) return false;
	return true;
}

ll gcd_ll(ll a, ll b) { return b ? gcd_ll(b, a % b) : a; }

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());

// 返回 n 的一个非平凡因子（n 必须是合数）
ll pollard(ll n)
{
	if(n % 2 == 0) return 2;
	if(n % 3 == 0) return 3;
	while(true)
	{
		ll c = (ll)(rng() % (unsigned long long)(n - 1)) + 1;
		ll x = (ll)(rng() % (unsigned long long)n), y = x, d = 1;
		auto f = [&](ll v) { return (ll)(((__int128)v * v + c) % n); };
		while(d == 1) //Floyd 判圈：x 走一步，y 走两步
		{
			x = f(x);
			y = f(f(y));
			d = gcd_ll(x > y ? x - y : y - x, n);
		}
		if(d != n) return d; //d == n 说明这轮的 c 不行，换一个重来
	}
}

// 用法：vector<ll> f; factorize(n, f); 跑完 f 里就是 n 的全部质因子（多重集，顺序任意）
void factorize(ll n, vector<ll> &f)
{
	if(n == 1) return;
	if(is_prime(n)) { f.push_back(n); return; }
	ll d = pollard(n);
	factorize(d, f);
	factorize(n / d, f);
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> T;
	while(T --)
	{
		cin >> n;
		if(n < 2) { cout << "C 1" << endl; continue; } //题目保证 n >= 2，这里只是兜底
		if(is_prime(n)) { cout << "P" << endl; continue; }
		vector<ll> f;
		factorize(n, f);
		cout << "C " << *min_element(f.begin(), f.end()) << endl; //最小质因子 = 质因子多重集的最小值
	}
	return 0;
}
/*
Input:
6
2
1000000007
1000000000000000000
999999830000006741
2305843009213693951
561

Output:
P
P
C 2
C 999999893
P
C 3
*/
