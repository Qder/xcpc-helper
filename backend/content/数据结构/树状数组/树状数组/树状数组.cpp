#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 500050
#define endl '\n'
#define ll long long
using namespace std;

int n, m;
ll tr[maxn];

int lowbit(int x) { return x & -x; }

// 用法：基础版就三件事——单点加 bit_add(i, x)，前缀和 bit_sum(x)，区间和 bit_sum(r) - bit_sum(l - 1)
// 下标一律 1-based：x == 0 时 lowbit(0) == 0，add 里的循环会原地打转
void bit_add(int x, ll v)
{
	for(int i = x; i <= n; i += lowbit(i)) tr[i] += v;
}

ll bit_sum(int x)
{
	ll res = 0;
	for(int i = x; i > 0; i -= lowbit(i)) res += tr[i];
	return res;
}

//-- 区间加 + 单点查：树状数组维护差分 d[i] = a[i] - a[i - 1] --
// 用法：diff_init(N) 之后，diff_add(l, r, k) 给 [l, r] 整体加 k，point_query(x) 查 a[x]
int dn;
ll dtr[maxn];

void diff_init(int N)
{
	dn = N;
	for(int i = 1; i <= dn; i ++) dtr[i] = 0;
}

void diff_add(int l, int r, ll k)
{
	for(int i = l; i <= dn; i += lowbit(i)) dtr[i] += k;
	for(int i = r + 1; i <= dn; i += lowbit(i)) dtr[i] -= k; //r + 1 越界时循环自然不执行
}

ll point_query(int x)
{
	ll res = 0;
	for(int i = x; i > 0; i -= lowbit(i)) res += dtr[i];
	return res;
}

//-- 区间加 + 区间查：两个树状数组，tr1 维护 d[i]，tr2 维护 i * d[i] --
// 用法：rb_init(N) 之后，rb_range_add(l, r, k) 区间加，rb_range_sum(l, r) 区间和
// 原理：前缀和 sum[1..x] = (x + 1) * Σd[i] - Σ(i * d[i])
int rbn;
ll tr1[maxn], tr2[maxn];

void rb_init(int N)
{
	rbn = N;
	for(int i = 1; i <= rbn; i ++) tr1[i] = tr2[i] = 0;
}

void rb_add(int x, ll k)
{
	for(int i = x; i <= rbn; i += lowbit(i)) tr1[i] += k, tr2[i] += (ll)x * k; //tr2 里记的是下标 * d[下标]
}

void rb_range_add(int l, int r, ll k)
{
	rb_add(l, k);
	if(r + 1 <= rbn) rb_add(r + 1, -k); //右端点在数组末尾时就只改 d[l]
}

ll rb_prefix(int x)
{
	ll s1 = 0, s2 = 0;
	for(int i = x; i > 0; i -= lowbit(i)) s1 += tr1[i], s2 += tr2[i];
	return (ll)(x + 1) * s1 - s2;
}

ll rb_range_sum(int l, int r)
{
	return rb_prefix(r) - rb_prefix(l - 1);
}

//-- 树状数组求逆序对：值域可能到 1e9，先离散化到 1..cnt 再统计 --
// 用法：把数存进 ia[1..N]，cout << inversion_count(N);  // 返回 i < j 且 ia[i] > ia[j] 的对数
ll ia[maxn];
ll inv_tr[maxn];
int inv_n;

ll inversion_count(int N)
{
	vector<ll> b(ia + 1, ia + N + 1);
	sort(b.begin(), b.end());
	b.erase(unique(b.begin(), b.end()), b.end());
	inv_n = (int)b.size();
	for(int i = 1; i <= inv_n; i ++) inv_tr[i] = 0;
	ll res = 0;
	for(int i = N; i >= 1; i --) //从右往左扫，已经进树状数组的都是下标更大的
	{
		int id = lower_bound(b.begin(), b.end(), ia[i]) - b.begin() + 1;
		for(int j = id - 1; j > 0; j -= lowbit(j)) res += inv_tr[j]; //比 ia[i] 小的已出现个数
		for(int j = id; j <= inv_n; j += lowbit(j)) inv_tr[j] ++;
	}
	return res;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++)
	{
		ll x; cin >> x;
		bit_add(i, x);
	}
	while(m --)
	{
		int op; cin >> op;
		if(op == 1)
		{
			int i; ll x; cin >> i >> x;
			bit_add(i, x);
		}
		else
		{
			int l, r; cin >> l >> r;
			cout << bit_sum(r) - bit_sum(l - 1) << endl;
		}
	}
	return 0;
}
/*
Input:
5 5
1 2 3 4 5
2 1 5
1 3 10
2 1 5
2 3 3
2 2 4

Output:
15
25
13
19
*/
