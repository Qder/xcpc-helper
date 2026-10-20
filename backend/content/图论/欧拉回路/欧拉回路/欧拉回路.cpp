#include<bits/stdc++.h>
#define maxn 100050
#define maxm 500050
#define endl '\n'
using namespace std;

int n, m;
int head[maxn], to[maxm * 2 + 5], nxt[maxm * 2 + 5], tot = 1; //tot 从 1 开始，边 e 和 e^1 是同一条无向边的两个方向
int cur[maxn], deg[maxn], used[maxm * 2 + 5];
int ans[maxm], cnt;
int call_stk[maxm + 5], call_eid[maxm + 5], cs; //手写调用栈：call_stk 存点，call_eid 存走进这个点用的边编号
int fa[maxn];

void add_edge(int u, int v) { to[++ tot] = v; nxt[tot] = head[u]; head[u] = tot; }

int find_fa(int x)
{
	while(fa[x] != x) //路径减半写成迭代：并查集的链可能很长，递归一样会爆栈
	{
		fa[x] = fa[fa[x]];
		x = fa[x];
	}
	return x;
}

//-- 迭代版 Hierholzer：手写调用栈，避免递归在长链上爆栈（递归版 5 万点的链实测 0xC00000FD）--
// 当前弧优化：cur[u] 指向 u 还没走过的第一条边，走一条就把 cur[u] 往后挪，整张图每条边只看一次
// 递归版就是：走一条边先挪 cur[u]、dfs(v)，回溯时 ans[++ cnt] = 这条边的编号；这里用 call_stk 原样展开。
void dfs(int s)
{
	cs = 0;
	call_stk[++ cs] = s;
	call_eid[cs] = 0; //起点不是被哪条边走进来的
	while(cs)
	{
		int u = call_stk[cs];
		int &e = cur[u];
		while(e && used[e]) e = nxt[e]; //这条有向边用过了（它的反向边走过），跳过
		if(!e) //u 的边走完了，回溯
		{
			if(call_eid[cs]) ans[++ cnt] = call_eid[cs]; //回溯时记边，最后倒着输出就是回路
			cs --;
		}
		else
		{
			used[e] = used[e ^ 1] = true; //无向边两个方向一起标记，保证同一条边只走一次
			int v = to[e], eid = e >> 1; //成对存边：(2,3)、(4,5)……，编号都是 e >> 1
			e = nxt[e]; //先挪当前弧再往下走，回来时这条边已经不能走了
			call_stk[++ cs] = v;
			call_eid[cs] = eid;
		}
	}
}

// 无向图有欧拉回路 <=> 所有点度数为偶 且 度数 > 0 的点全在同一连通块
// 孤立点（deg == 0）没有边，不能因为它自成一个块就判 No，所以判连通时要跳过
bool check()
{
	for(int i = 1; i <= n; i ++)
		if(deg[i] & 1) return false;
	int rt = 0;
	for(int i = 1; i <= n; i ++)
	{
		if(deg[i] == 0) continue;
		if(rt == 0) rt = find_fa(i);
		else if(find_fa(i) != rt) return false; //有边的点分成了两块，回路连不起来
	}
	return true;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) fa[i] = i;
	for(int i = 1; i <= m; i ++)
	{
		int u, v; cin >> u >> v;
		add_edge(u, v); add_edge(v, u);
		deg[u] ++; deg[v] ++; //自环 u == v 会让 deg[u] 加 2，正好抵消，仍是偶度
		int ru = find_fa(u), rv = find_fa(v);
		if(ru != rv) fa[ru] = rv;
	}
	if(!check()) { cout << "No" << endl; return 0; }
	if(m == 0) { cout << "Yes" << endl << endl; return 0; } //没有边，空回路也算回路，第二行留空
	int st = 0;
	for(int i = 1; i <= n; i ++)
		if(deg[i]) { st = i; break; } //回路从哪个点出发都行，取第一个有边的点
	for(int i = 1; i <= n; i ++) cur[i] = head[i]; //当前弧初始指向第一条边
	dfs(st);
	cout << "Yes" << endl;
	for(int i = cnt; i >= 1; i --) cout << ans[i] << " \n"[i == 1];
	return 0;
}
/*
Input:
4 4
1 2
2 3
3 1
2 2

Output:
Yes
3 2 4 1
*/
