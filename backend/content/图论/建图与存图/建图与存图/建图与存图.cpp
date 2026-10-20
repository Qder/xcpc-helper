#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100005
#define maxm 400005
#define endl '\n'
#define ll long long
using namespace std;

struct Edge { int v, w; };

int n, m;
vector<Edge> g[maxn]; //无向图，一条边正反各存一次

int vis[maxn], ord[maxn], cnt;

// 用法：读完图调 bfs(1)，访问序列在 ord[1..cnt]（只走到 1 所在的连通块）
void bfs(int s)
{
	queue<int> q;
	q.push(s); vis[s] = 1;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		ord[++ cnt] = u;
		for(Edge e : g[u]) //vector 是尾插，邻居天然按读入顺序出现
		{
			if(vis[e.v]) continue;
			vis[e.v] = 1; //进队时就打标记，否则一条边会把同层点塞两遍
			q.push(e.v);
		}
	}
}

// 用法：dfs(1)，结果同样在 ord[1..cnt]；用栈模拟递归，链长 1e5 也不会爆栈
void dfs(int s)
{
	stack<int> st;
	st.push(s);
	while(!st.empty())
	{
		int u = st.top(); st.pop();
		if(vis[u]) continue; //这个点可能早被别的分支压进来了
		vis[u] = 1; ord[++ cnt] = u;
		for(int i = (int)g[u].size() - 1; i >= 0; i --) //倒着压栈，出栈才是读入顺序
			if(!vis[g[u][i].v]) st.push(g[u][i].v);
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= m; i ++)
	{
		int u, v, w; cin >> u >> v >> w;
		g[u].push_back({v, w}); g[v].push_back({u, w});
	}
	bfs(1);
	for(int i = 1; i <= cnt; i ++) cout << ord[i] << " \n"[i == cnt];
	cnt = 0;
	for(int i = 1; i <= n; i ++) vis[i] = 0; //第二遍遍历前清标记
	dfs(1);
	for(int i = 1; i <= cnt; i ++) cout << ord[i] << " \n"[i == cnt];
	return 0;
}

//-- 链式前向星：head/nxt/to/ew 四个数组，常数比 vector 小，无向边要加两次（maxm 开到 2m 以上）--
int head[maxn], nxt[maxm], to[maxm], ew[maxm], tot;

// 用法：无向边要 add_edge(u, v, w) 和 add_edge(v, u, w) 各调一次；
//       遍历 u 的出边：for(int e = head[u]; e; e = nxt[e])，邻居是 to[e]、边权是 ew[e]
//       头插会让出边变成读入倒序，所以下面两个遍历先把邻居收进 buf 再倒扫，结果和 vector 版一模一样
void add_edge(int u, int v, int w)
{
	to[++ tot] = v; ew[tot] = w;
	nxt[tot] = head[u]; head[u] = tot;
}

int buf[maxm];

void cs_bfs(int s)
{
	for(int i = 1; i <= n; i ++) vis[i] = 0;
	cnt = 0;
	queue<int> q;
	q.push(s); vis[s] = 1;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		ord[++ cnt] = u;
		int tc = 0;
		for(int e = head[u]; e; e = nxt[e]) buf[++ tc] = to[e];
		for(int i = tc; i >= 1; i --) //buf 是倒序，倒扫就是读入顺序
			if(!vis[buf[i]]) { vis[buf[i]] = 1; q.push(buf[i]); }
	}
}

void cs_dfs(int s)
{
	for(int i = 1; i <= n; i ++) vis[i] = 0;
	cnt = 0;
	stack<int> st;
	st.push(s);
	while(!st.empty())
	{
		int u = st.top(); st.pop();
		if(vis[u]) continue;
		vis[u] = 1; ord[++ cnt] = u;
		int tc = 0;
		for(int e = head[u]; e; e = nxt[e]) buf[++ tc] = to[e];
		for(int i = 1; i <= tc; i ++) //正着压栈，栈顶就是第一条读入的边
			if(!vis[buf[i]]) st.push(buf[i]);
	}
}
/*
Input:
6 6
1 2 4
1 3 2
2 4 5
2 5 1
3 6 8
4 6 2

Output:
1 2 3 4 5 6
1 2 4 6 3 5
*/
