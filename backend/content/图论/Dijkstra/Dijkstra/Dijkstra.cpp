#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100005
#define maxm 200005
#define endl '\n'
#define ll long long
using namespace std;

const ll llinf = 0x3f3f3f3f3f3f3f3f;

struct Edge { int v, w; };

int n, m;
vector<Edge> g[maxn]; //有向图，只存 u -> v 这一条
ll dis[maxn];

// 用法：dijkstra(s) 之后 dis[1..n] 就是答案，>= llinf 表示不可达；边权必须非负
void dijkstra(int s)
{
	for(int i = 1; i <= n; i ++) dis[i] = llinf; //多测要在这里重跑，别指望全局清零
	dis[s] = 0;
	//模板参数写全，不用 greater<> 简写：那要 c++14 以上才认，老评测机（c++11）直接编译错
	priority_queue<pair<ll, int>, vector<pair<ll, int> >, greater<pair<ll, int> > > pq;
	pq.push(make_pair(0, s));
	while(!pq.empty())
	{
		pair<ll, int> t = pq.top(); pq.pop();
		int u = t.second;
		if(t.first > dis[u]) continue; //堆里的旧值：u 已经被更短的路径更新过了
		for(Edge e : g[u])
			if(dis[u] + e.w < dis[e.v])
			{
				dis[e.v] = dis[u] + e.w;
				pq.push(make_pair(dis[e.v], e.v));
			}
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int s;
	cin >> n >> m >> s;
	for(int i = 1; i <= m; i ++)
	{
		int u, v, w; cin >> u >> v >> w;
		g[u].push_back({v, w});
	}
	dijkstra(s);
	for(int i = 1; i <= n; i ++)
	{
		if(dis[i] >= llinf) cout << -1;
		else cout << dis[i];
		cout << " \n"[i == n];
	}
	return 0;
}

//-- 朴素 O(n^2) Dijkstra：邻接矩阵，稠密图（m ≈ n^2）比堆优化常数小 --
const int MAXD = 505; //邻接矩阵只开得起 n <= 500，更大的图还是用上面的堆优化
ll mat[MAXD][MAXD];

// 用法：memset(mat, 0x3f, sizeof mat) 后把 mat[i][i] 置 0，读边时 mat[u][v] = min(mat[u][v], w)，
//       再把 main 里的 dijkstra(s) 换成 dijkstra_naive(s)，答案同样在 dis[1..n]
void dijkstra_naive(int s)
{
	for(int i = 1; i <= n; i ++) dis[i] = llinf;
	dis[s] = 0;
	int used[MAXD] = {0};
	for(int it = 1; it <= n; it ++)
	{
		int u = 0;
		for(int i = 1; i <= n; i ++)
			if(!used[i] && (u == 0 || dis[i] < dis[u])) u = i;
		if(u == 0 || dis[u] >= llinf) break; //剩下的点全不可达，再扫也扫不出东西
		used[u] = 1;
		for(int v = 1; v <= n; v ++)
			if(mat[u][v] < llinf && dis[u] + mat[u][v] < dis[v]) dis[v] = dis[u] + mat[u][v];
	}
}
/*
Input:
7 7 1
1 2 2
1 3 5
2 3 1
2 4 6
3 5 3
4 5 1
5 6 4

Output:
0 2 3 8 6 10 -1
*/
