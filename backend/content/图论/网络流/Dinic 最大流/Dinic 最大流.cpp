#include<bits/stdc++.h>
#define maxn 200005
#define endl '\n'
#define ll long long
using namespace std;

const ll llinf = 0x3f3f3f3f3f3f3f3f;

// 残量网络的一条边：to 是终点，rev 是反向边在 g[to] 里的下标，cap 是剩余容量
struct Edge { int to, rev; ll cap; };

int n, S, T;
vector<Edge> g[maxn];
int level[maxn], cur[maxn]; //level：BFS 分层；cur：当前弧，已经增广完的边不再重扫

void add_edge(int u, int v, ll cap)
{
	g[u].push_back((Edge){v, (int)g[v].size(), cap});
	g[v].push_back((Edge){u, (int)g[u].size() - 1, 0}); //反向边容量 0，用来退流
}

// 用法：init(n) 清空整张图并设置点数，多测必须重新调用；点编号 1-based。
//       add_edge(u, v, cap) 建一条 u -> v 的有向边，无向边把两个方向各加一次。
//       dinic(s, t) 返回最大流；容量用 long long，总流量可以超过 int。
void init(int n_)
{
	n = n_;
	for(int i = 1; i <= n; i ++) g[i].clear();
}

bool bfs()
{
	for(int i = 1; i <= n; i ++) level[i] = -1;
	queue<int> q;
	level[S] = 0;
	q.push(S);
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		for(int i = 0; i < (int)g[u].size(); i ++)
		{
			Edge &e = g[u][i];
			if(e.cap > 0 && level[e.to] < 0) //只走还有残量的边，分层图里每个点只标一次
			{
				level[e.to] = level[u] + 1;
				q.push(e.to);
			}
		}
	}
	return level[T] >= 0; //汇点不可达说明没有增广路了
}

ll dfs(int u, ll f)
{
	if(u == T) return f;
	for(int &i = cur[u]; i < (int)g[u].size(); i ++) //当前弧：i 是引用，回溯后 cur[u] 自动前进
	{
		Edge &e = g[u][i];
		if(e.cap > 0 && level[e.to] == level[u] + 1)
		{
			ll d = dfs(e.to, min(f, e.cap));
			if(d > 0)
			{
				e.cap -= d;
				g[e.to][e.rev].cap += d; //反向边加流，等价于把已经推过去的流退回来
				return d;
			}
		}
	}
	return 0;
}

ll dinic(int s, int t)
{
	S = s; T = t;
	ll flow = 0;
	while(bfs()) //每轮 BFS 重建分层图，DFS 只沿 level + 1 的边走，保证单轮无环
	{
		for(int i = 1; i <= n; i ++) cur[i] = 0;
		ll f;
		while((f = dfs(S, llinf)) > 0) flow += f; //一次 DFS 找到一条增广路就立刻累计
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
	cout << dinic(s, t) << endl;
	return 0;
}
/*
Input:
4 5 4 3
4 2 30
4 3 20
2 3 20
2 1 30
1 3 30

Output:
50
*/
