#include<bits/stdc++.h>
#define maxn 200005
#define endl '\n'
#define ll long long
using namespace std;

const ll llinf = 0x3f3f3f3f3f3f3f3f;

struct Edge { int to, rev; ll cap; };

int n, S, T;
vector<Edge> g[maxn];
int level[maxn], cur[maxn];
bool side[maxn]; //side[u] = true 表示 u 在最小割的 S 侧（源点一侧）

void add_edge(int u, int v, ll cap)
{
	g[u].push_back((Edge){v, (int)g[v].size(), cap});
	g[v].push_back((Edge){u, (int)g[u].size() - 1, 0});
}

// 用法：init(n) 清空并设置点数（多测必须调用），点编号 1-based；
//       add_edge(u, v, cap) 建 u -> v 的有向边，无向边两个方向各加一次；
//       min_cut(s, t) 返回最小割容量（= 最大流），并把割的 S 侧标进 side[]。
// 建模套路（代码不用改，只改建图）：
//   1. 割点 / 点权：把点 u 拆成 u 和 u + n，中间连一条容量为点权的边，
//      原图的边 u -> v 改成 u + n -> v（容量 INF），割这条拆点边就等于选这个点；
//   2. 最大权闭合子图：正权点 S -> u 容量为权值，负权点 u -> T 容量为 -权值，
//      原依赖边 u -> v 容量 INF，答案 = 正权之和 - 最小割；
//   3. 二分图最小点覆盖 / 最大独立集：与最大匹配互相转化（König 定理）。
void init(int n_)
{
	n = n_;
	for(int i = 1; i <= n; i ++) g[i].clear();
}

bool bfs()
{
	for(int i = 1; i <= n; i ++) level[i] = -1;
	queue<int> q;
	level[S] = 0; q.push(S);
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		for(int i = 0; i < (int)g[u].size(); i ++)
		{
			Edge &e = g[u][i];
			if(e.cap > 0 && level[e.to] < 0)
			{
				level[e.to] = level[u] + 1;
				q.push(e.to);
			}
		}
	}
	return level[T] >= 0;
}

ll dfs(int u, ll f)
{
	if(u == T) return f;
	for(int &i = cur[u]; i < (int)g[u].size(); i ++)
	{
		Edge &e = g[u][i];
		if(e.cap > 0 && level[e.to] == level[u] + 1)
		{
			ll d = dfs(e.to, min(f, e.cap));
			if(d > 0)
			{
				e.cap -= d;
				g[e.to][e.rev].cap += d;
				return d;
			}
		}
	}
	return 0;
}

ll min_cut(int s, int t)
{
	S = s; T = t;
	ll flow = 0;
	while(bfs())
	{
		for(int i = 1; i <= n; i ++) cur[i] = 0;
		ll f;
		while((f = dfs(S, llinf)) > 0) flow += f;
	}
	// 最大流跑完后，残量网络里从 S 能走到的点就是最小割的 S 侧；
	// 所有「S 侧 -> T 侧」且已满流的原边合成一个最小割。
	for(int i = 1; i <= n; i ++) side[i] = false;
	queue<int> q;
	side[S] = true; q.push(S);
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		for(int i = 0; i < (int)g[u].size(); i ++)
		{
			Edge &e = g[u][i];
			if(e.cap > 0 && !side[e.to]) { side[e.to] = true; q.push(e.to); }
		}
	}
	return flow;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int m, s, t;
	cin >> n >> m >> s >> t;
	init(n);
	for(int i = 1; i <= m; i ++)
	{
		int u, v; ll w;
		cin >> u >> v >> w;
		add_edge(u, v, w);
	}
	cout << min_cut(s, t) << endl; //最小割容量，等于最大流
	int cnt = 0;
	for(int i = 1; i <= n; i ++) if(side[i]) cnt ++;
	cout << cnt << endl; //S 侧点数
	for(int i = 1; i <= n; i ++)
		if(side[i]) cout << i << ' ';
	cout << endl;
	return 0;
}
/*
Input:
4 4 1 4
1 2 3
1 3 2
2 4 2
3 4 3

Output:
4
2
1 2
*/
