#include<bits/stdc++.h>
#define maxn 100050
#define maxm 100050
#define endl '\n'
using namespace std;

//-- 带修莫队：单点修改 + 区间不同数个数，块长 n^(2/3)，三维排序，n、m 同阶时约 O(n^(5/3)) --

int n, m, block, distinct;
int a[maxn], cnt[maxn + maxm], ans[maxm]; //cnt 要开够：初始 n 种值 + 每次修改各引入 1 种新值
int val[maxn + maxm], tot; //初始值和所有修改后的值一起离散化，否则 cnt 下标会越界
int cur[maxn]; //读入时模拟用：记录每个位置当前的值，用来填修改的「修改前」

struct Query
{
	int l, r, t, id; //t = 该询问之前已经执行了多少次修改（时间维）
} q[maxm];

struct Modify
{
	int pos, bef, aft; //位置 pos 上的值从 bef 变成 aft（都是离散化后的值）
} md[maxm];

int qcnt, mcnt;
int curL = 1, curR = 0, curT = 0; //curT = 当前已经施加了前多少次修改

void add(int pos)
{
	if(++ cnt[a[pos]] == 1) distinct ++;
}

void del(int pos)
{
	if(-- cnt[a[pos]] == 0) distinct --;
}

//施加第 t 次修改（t 从 0 起）：forward 为真改成新值，为假退回旧值
//修改位置在当前区间内时，必须先删旧值、再写 a[pos]、最后加新值；不在区间内只改数组，distinct 不变
void apply(int t, bool forward)
{
	int pos = md[t].pos, nv = forward ? md[t].aft : md[t].bef;
	if(curL <= pos && pos <= curR)
	{
		del(pos);
		a[pos] = nv;
		add(pos);
	}
	else a[pos] = nv;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++)
	{
		cin >> a[i];
		val[++ tot] = a[i];
		cur[i] = a[i];
	}
	for(int i = 1; i <= m; i ++)
	{
		int op; cin >> op;
		if(op == 1)
		{
			int p, x; cin >> p >> x;
			md[mcnt].pos = p;
			md[mcnt].bef = cur[p]; //此刻位置 p 的值才是「修改前」，不能直接用初始数组 a
			md[mcnt].aft = x;
			mcnt ++;
			cur[p] = x;
			val[++ tot] = x;
		}
		else
		{
			int l, r; cin >> l >> r;
			q[qcnt].l = l; q[qcnt].r = r;
			q[qcnt].t = mcnt; //时间 = 到这一刻为止的修改次数
			q[qcnt].id = qcnt;
			qcnt ++;
		}
	}
	//离散化；a 一直是初始数组（读入时只动过 cur），这里直接映射成初始版本
	sort(val + 1, val + tot + 1);
	tot = (int)(unique(val + 1, val + tot + 1) - val - 1);
	for(int i = 1; i <= n; i ++) a[i] = (int)(lower_bound(val + 1, val + tot + 1, a[i]) - val) - 1;
	for(int i = 0; i < mcnt; i ++)
	{
		md[i].bef = (int)(lower_bound(val + 1, val + tot + 1, md[i].bef) - val) - 1;
		md[i].aft = (int)(lower_bound(val + 1, val + tot + 1, md[i].aft) - val) - 1;
	}
	
	//带修莫队的块长取 n^(2/3)：左右块各有约 n^(1/3) 个，块内右端点共走 n，
	//再加上时间维的来回移动，总复杂度约 O(n^(5/3))（n、m 同阶）
	block = max(1, (int)pow((double)n, 2.0 / 3.0));
	//三维排序：左端点块 -> 右端点块（蛇形）-> 时间（同一右块内也按奇偶反向）
	sort(q, q + qcnt, [](const Query &x, const Query &y)
	{
		int lx = (x.l - 1) / block, ly = (y.l - 1) / block;
		if(lx != ly) return lx < ly;
		int rx = (x.r - 1) / block, ry = (y.r - 1) / block;
		if(rx != ry) return (lx & 1) ? rx > ry : rx < ry;
		return (rx & 1) ? x.t > y.t : x.t < y.t;
	});
	
	for(int i = 0; i < qcnt; i ++)
	{
		int l = q[i].l, r = q[i].r;
		//先把左右指针移到位，再动时间维：这样 apply 里的「是否在当前区间内」用的就是本次询问的区间
		while(curL > l) add(-- curL);
		while(curR < r) add(++ curR);
		while(curL < l) del(curL ++);
		while(curR > r) del(curR --);
		//回退时先 --curT 再撤销，撤销的才是刚刚多施加的那一次修改
		while(curT < q[i].t) apply(curT ++, true);
		while(curT > q[i].t) apply(-- curT, false);
		ans[q[i].id] = distinct;
	}
	for(int i = 0; i < qcnt; i ++) cout << ans[i] << endl;
	return 0;
}
/*
Input:
5 5
1 1 2 1 3
2 1 5
1 2 3
2 1 5
2 2 2
2 1 1

Output:
3
3
1
1
*/
