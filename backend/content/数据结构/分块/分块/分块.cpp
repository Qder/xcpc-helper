#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
#define ll long long
using namespace std;

//-- 分块：区间加 + 区间和（P3372 的分块平替）--
// 大段维护、小块朴素：整块只动懒标记，两头的散块暴力改，单次 O(sqrt(n))

int n, m, blk, btot;
ll a[maxn], sum[maxn], add[maxn]; //sum[b] 是块 b 内 a[i] 的和（不含 add[b]），add[b] 是加在整块上的量
int bel[maxn], L[maxn], R[maxn];

void build()
{
	blk = sqrt(n) + 1; //块长取 sqrt(n) + 1：比 sqrt(n) 略大，块数略少一点，常数更稳
	btot = (n + blk - 1) / blk;
	for(int b = 1; b <= btot; b ++)
	{
		L[b] = (b - 1) * blk + 1;
		R[b] = min(n, b * blk); //最后一块可能不满
	}
	for(int i = 1; i <= n; i ++)
	{
		bel[i] = (i - 1) / blk + 1;
		sum[bel[i]] += a[i];
	}
}

void range_add(int l, int r, ll k)
{
	if(bel[l] == bel[r]) //同一块里没有“整块”可言，直接暴力
	{
		for(int i = l; i <= r; i ++) { a[i] += k; sum[bel[i]] += k; }
		return;
	}
	for(int i = l; i <= R[bel[l]]; i ++) { a[i] += k; sum[bel[i]] += k; } //左边散块
	for(int i = L[bel[r]]; i <= r; i ++) { a[i] += k; sum[bel[i]] += k; } //右边散块
	for(int b = bel[l] + 1; b <= bel[r] - 1; b ++) add[b] += k; //中间整块只动懒标记，O(1) 一块
}

ll range_sum(int l, int r)
{
	ll res = 0;
	if(bel[l] == bel[r])
	{
		for(int i = l; i <= r; i ++) res += a[i] + add[bel[i]];
		return res;
	}
	for(int i = l; i <= R[bel[l]]; i ++) res += a[i] + add[bel[i]]; //散块：真实值 = a[i] + 块懒标记
	for(int i = L[bel[r]]; i <= r; i ++) res += a[i] + add[bel[i]];
	for(int b = bel[l] + 1; b <= bel[r] - 1; b ++)
		res += sum[b] + add[b] * (R[b] - L[b] + 1); //整块：块内和 + 懒标记 × 这一块的真实长度
	return res;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	build(); //建块要在读入之后，sum 只能统计一次
	while(m --)
	{
		int op, l, r; cin >> op >> l >> r;
		if(op == 1) { ll k; cin >> k; range_add(l, r, k); }
		else cout << range_sum(l, r) << endl;
	}
	return 0;
}
/*
Input:
5 4
1 2 3 4 5
2 2 4
1 1 3 10
2 1 5
2 5 5

Output:
9
45
5
*/
