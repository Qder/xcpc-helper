#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 500050
#define endl '\n'
using namespace std;

int n, m, LOG;
vector<int> g[maxn];
int lg[maxn], dep[maxn], fa[maxn][20];
int order[maxn], ocnt;

// lg 预处理：LOG = lg[n] 是满足 2^LOG ≤ n 的最大值，第二维开 20 够 n ≤ 5e5
void init_lg()
{
	lg[1] = 0;
	for(int i = 2; i <= n; i ++) lg[i] = lg[i >> 1] + 1;
	LOG = lg[n];
}

// BFS 定序：不递归，顺路把 dep、fa[][0] 和整张倍增表填好（长链也不会爆栈）
void bfs_build(int rt)
{
	queue<int> q; q.push(rt); fa[rt][0] = 0; dep[rt] = 1; //根深度设为 1，向上跳超出根时才会停在 fa=0 上
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		order[++ ocnt] = u; //逆序就是"从叶子往根"，树剖部分要用
		for(int v : g[u]) if(v != fa[u][0])
		{
			dep[v] = dep[u] + 1; fa[v][0] = u;
			for(int i = 1; i <= LOG; i ++) fa[v][i] = fa[fa[v][i - 1]][i - 1];
			q.push(v);
		}
	}
}

int lca(int u, int v)
{
	if(dep[u] < dep[v]) swap(u, v);
	int d = dep[u] - dep[v];
	for(int i = 0; i <= LOG; i ++) if(d >> i & 1) u = fa[u][i]; //深度对齐：先把 u 单跳到 v 的深度
	if(u == v) return u;
	for(int i = LOG; i >= 0; i --)
		if(fa[u][i] != fa[v][i]) u = fa[u][i], v = fa[v][i]; //一起跳到 LCA 的下一层
	return fa[u][0];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i < n; i ++)
	{
		int u, v; cin >> u >> v;
		g[u].push_back(v); g[v].push_back(u);
	}
	init_lg();
	bfs_build(1);
	for(int i = 1; i <= m; i ++)
	{
		int a, b; cin >> a >> b;
		cout << lca(a, b) << endl;
	}
	return 0;
}
//-- 树链剖分求 LCA（预处理 O(n)，查询 O(log n)，常数比倍增小，还能兼容路径修改）--

int sz[maxn], son[maxn], top[maxn], dfn[maxn], tim;

// 用法：dep/fa[][0]/order 已经由上面的 bfs_build 填好了，直接用
// 逆序回代求 sz 和重儿子（不递归），再手写栈模拟"先重儿子、后轻儿子"的 DFS 求 dfn/top
void hld_build(int rt)
{
	for(int i = 1; i <= n; i ++) sz[i] = 1, son[i] = 0;
	for(int i = ocnt; i >= 2; i --)
	{
		int u = order[i];
		sz[fa[u][0]] += sz[u];
		if(sz[u] > sz[son[fa[u][0]]]) son[fa[u][0]] = u;
	}
	stack<pair<int, int> > st; st.push(make_pair(rt, rt)); //长链时递归 dfs2 会爆栈，改成手写栈
	while(!st.empty())
	{
		int u = st.top().first, tp = st.top().second; st.pop();
		top[u] = tp; dfn[u] = ++ tim;
		for(int v : g[u]) if(v != fa[u][0] && v != son[u]) st.push(make_pair(v, v)); //轻儿子各开一条链
		if(son[u]) st.push(make_pair(son[u], tp)); //重儿子最后压栈、最先出栈，同一条链的 dfn 才连续
	}
}

int lca_hld(int u, int v)
{
	while(top[u] != top[v])
	{
		if(dep[top[u]] < dep[top[v]]) swap(u, v); //链头浅的那条先往上跳
		u = fa[top[u]][0];
	}
	return dep[u] < dep[v] ? u : v; //同一条链里浅的就是 LCA
}
// 用法：init_lg(); bfs_build(1); hld_build(1); 然后把 main 里的 lca(a, b) 换成 lca_hld(a, b)
/*
Input:
5 4
1 2
2 3
3 4
2 5
1 3
4 5
1 1
4 2

Output:
1
2
1
2
*/
