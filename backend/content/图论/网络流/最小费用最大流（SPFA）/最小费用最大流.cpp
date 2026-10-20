#include<bits/stdc++.h>
#define maxn 200005
#define endl '\n'
#define ll long long
using namespace std;

const ll llinf = 0x3f3f3f3f3f3f3f3f;

// 残量网络的边：cap 剩余容量，cost 单位流量费用；反向边容量 0、费用取反
struct Edge { int to, rev; ll cap, cost; };

int n;
vector<Edge> g[maxn];
ll dis[maxn], pre_v[maxn];
int pre_e[maxn];
bool inq[maxn];

void add_edge(int u, int v, ll cap, ll cost)
{
	g[u].push_back((Edge){v, (int)g[v].size(), cap, cost});
	g[v].push_back((Edge){u, (int)g[u].size() - 1, 0, -cost});
}

// 用法：init(n) 清空并设置点数，多测必须重新调用；点编号 1-based。
//       add_edge(u, v, cap, cost) 建 u -> v 的有向边（capacity, cost per unit）。
//       mcmf(s, t) 返回 make_pair(最大流, 最小费用)；费用用 long long，可含负费用，
//       但不能存在从 s 可达的负费用环，否则最短路不存在、最小费用无意义。
void init(int n_)
{
	n = n_;
	for(int i = 1; i <= n; i ++) g[i].clear();
}

// SPFA 按费用找最短路（允许负费用）；不可达时返回 false
bool spfa(int s, int t)
{
	for(int i = 1; i <= n; i ++) { dis[i] = llinf; pre_v[i] = -1; inq[i] = false; }
	queue<int> q;
	dis[s] = 0; q.push(s); inq[s] = true;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		inq[u] = false;
		for(int i = 0; i < (int)g[u].size(); i ++)
		{
			Edge &e = g[u][i];
			if(e.cap > 0 && dis[u] + e.cost < dis[e.to]) //只沿还有残量的边松弛
			{
				dis[e.to] = dis[u] + e.cost;
				pre_v[e.to] = u;
				pre_e[e.to] = i;
				if(!inq[e.to]) { q.push(e.to); inq[e.to] = true; }
			}
		}
	}
	return dis[t] < llinf;
}

pair<ll, ll> mcmf(int s, int t)
{
	ll flow = 0, cost = 0;
	while(spfa(s, t))
	{
		ll f = llinf;
		for(int v = t; v != s; v = pre_v[v]) f = min(f, g[pre_v[v]][pre_e[v]].cap); //这条最短路能推的流量
		for(int v = t; v != s; v = pre_v[v])
		{
			Edge &e = g[pre_v[v]][pre_e[v]];
			e.cap -= f;
			g[v][e.rev].cap += f;
		}
		flow += f;
		cost += f * dis[t]; //沿当前最短路增广 f，费用增加 f * 单位最短路费用
	}
	return make_pair(flow, cost);
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
		int u, v; ll w, c;
		cin >> u >> v >> w >> c;
		add_edge(u, v, w, c);
	}
	pair<ll, ll> ans = mcmf(s, t);
	cout << ans.first << ' ' << ans.second << endl;
	return 0;
}
/*
Input:
4 5 4 3
4 2 30 2
4 3 20 3
2 3 20 1
2 1 30 9
1 3 40 5

Output:
50 280
*/
