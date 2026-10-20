#include<bits/stdc++.h>
#define maxn 100005
#define maxm 100005
#define endl '\n'
using namespace std;

/*
// 线段树套平衡树：外层线段树按**下标**分区间，每个节点挂一棵平衡树（或 multiset）维护该区间的所有值。
// 适用场合：单点修改 + 区间查排名 / 区间前驱后继这类“不一定要第 k 小”的题。
// 复杂度：建树 O(n log n)，单点修改 O(log²n)，查排名 / 前驱后继 O(log²n)；区间第 k 小要在值域上二分答案再查排名，O(log³n)。
// 常数很大，能整体二分 / CDQ 就别用它；下面活跃版换成树状数组套权值线段树，第 k 小一次下探 O(log²n)。
*/

//-- 树状数组套权值线段树（动态区间第 k 小，P2617）--

int n, m;
int a[maxn]; //a[i] 存离散化后的值，范围 1 ~ vtot
int val[maxn + maxm], vtot; //离线离散化：初始值 + 所有修改值，排序去重后就是内层线段树的值域

struct Que { int op, x, y, k; } q[maxm]; //操作先读进来存着，才能把修改值也一起离散化

int rt[maxn]; //rt[i]：树状数组节点 i 上挂的那棵权值线段树的根
int lc[maxn * 200], rc[maxn * 200], cnt[maxn * 200], tot; //动态开点，1e5 规模实测约 1.2e7 个点

// 在一棵权值线段树里把 pos 的出现次数加 delta（delta = ±1）
void update(int &u, int l, int r, int pos, int delta)
{
	if(!u) u = ++ tot;
	cnt[u] += delta;
	if(l == r) return;
	int mid = (l + r) >> 1;
	if(pos <= mid) update(lc[u], l, mid, pos, delta);
	else update(rc[u], mid + 1, r, pos, delta);
}

// 位置 p 上的值 pos 出现次数加 delta：树状数组往上跳，每跳到一个节点就在它的线段树里改
void modify(int p, int pos, int delta)
{
	for(int i = p; i <= n; i += i & -i) update(rt[i], 1, vtot, pos, delta);
}

int lnod[25], rnod[25]; //[l, r] 拆出来的 O(log n) 棵树，左边减右边；25 > log2(1e5)

// 区间 [l, r] 第 k 小，返回离散化下标；k 越界（k < 1 或 k > 区间长度）时返回 0
int query_kth(int l, int r, int k)
{
	if(k < 1 || k > r - l + 1) return 0;
	int ln = 0, rn = 0;
	for(int i = r; i; i -= i & -i) rnod[++ rn] = rt[i]; //前缀 r 的 O(log n) 棵树
	for(int i = l - 1; i; i -= i & -i) lnod[++ ln] = rt[i]; //前缀 l-1 的 O(log n) 棵树，和上面做差
	int L = 1, R = vtot;
	while(L < R)
	{
		int mid = (L + R) >> 1, c = 0;
		for(int i = 1; i <= ln; i ++) c -= cnt[lc[lnod[i]]]; //左子树的真实个数 = 前缀 r 减前缀 l-1
		for(int i = 1; i <= rn; i ++) c += cnt[lc[rnod[i]]];
		if(k <= c)
		{
			for(int i = 1; i <= ln; i ++) lnod[i] = lc[lnod[i]]; //第 k 小在左子树：所有树一起往左走
			for(int i = 1; i <= rn; i ++) rnod[i] = lc[rnod[i]];
			R = mid;
		}
		else
		{
			k -= c; //在右子树：先撤掉左子树的个数
			for(int i = 1; i <= ln; i ++) lnod[i] = rc[lnod[i]];
			for(int i = 1; i <= rn; i ++) rnod[i] = rc[rnod[i]];
			L = mid + 1;
		}
	}
	return L;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) { cin >> a[i]; val[++ vtot] = a[i]; }
	for(int i = 1; i <= m; i ++)
	{
		cin >> q[i].op;
		if(q[i].op == 1) { cin >> q[i].x >> q[i].y; val[++ vtot] = q[i].y; } //修改值也要进离散化
		else cin >> q[i].x >> q[i].y >> q[i].k;
	}
	sort(val + 1, val + vtot + 1);
	vtot = unique(val + 1, val + vtot + 1) - val - 1;
	for(int i = 1; i <= n; i ++)
	{
		a[i] = lower_bound(val + 1, val + vtot + 1, a[i]) - val;
		modify(i, a[i], 1);
	}
	for(int i = 1; i <= m; i ++)
	{
		if(q[i].op == 1)
		{
			int nw = lower_bound(val + 1, val + vtot + 1, q[i].y) - val;
			modify(q[i].x, a[q[i].x], -1); //先删旧值
			a[q[i].x] = nw;
			modify(q[i].x, a[q[i].x], 1); //再插新值
		}
		else cout << val[query_kth(q[i].x, q[i].y, q[i].k)] << endl;
	}
	return 0;
}
/*
Input:
5 3
1 5 3 2 4
2 2 4 2
1 3 6
2 1 5 3

Output:
3
4
*/
