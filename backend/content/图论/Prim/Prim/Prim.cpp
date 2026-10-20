#include<bits/stdc++.h>
#define maxn 5005
#define endl '\n'
#define ll long long
using namespace std;

struct Edge { int v; ll w; };

int n, m;
vector<Edge> g[maxn];
bool used[maxn];

// 用法：prim() 返回最小生成树边权和，图不连通返回 -1；点编号 1-based。
// 堆优化版 O((n+m) log n)，适合稀疏图；稠密图把堆换成每轮 O(n) 找最近点更省常数。
// 无向边要正反各加一次；重边、自环都能直接处理（used 会跳过已入树的点）。
ll prim()
{
	for(int i = 1; i <= n; i ++) used[i] = false;
	//模板参数写全，不用 greater<> 简写：C++14 起才认
	priority_queue<pair<ll, int>, vector<pair<ll, int> >, greater<pair<ll, int> > > pq;
	pq.push(make_pair(0, 1)); //从 1 号点开始，起点代价 0
	ll res = 0;
	int cnt = 0;
	while(!pq.empty())
	{
		pair<ll, int> t = pq.top(); pq.pop();
		int u = t.second;
		if(used[u]) continue; //堆里的旧值：u 已经通过更短的边入树了
		used[u] = true;
		res += t.first;
		cnt ++;
		for(int i = 0; i < (int)g[u].size(); i ++)
			if(!used[g[u][i].v]) pq.push(make_pair(g[u][i].w, g[u][i].v));
	}
	return cnt == n ? res : -1; //有没进树的点，说明图不连通
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= m; i ++)
	{
		int u, v; ll w;
		cin >> u >> v >> w;
		g[u].push_back((Edge){v, w});
		g[v].push_back((Edge){u, w});
	}
	ll ans = prim();
	if(ans < 0) cout << "orz" << endl;
	else cout << ans << endl;
	return 0;
}
/*
Input:
4 5
1 2 2
1 3 2
1 4 3
2 3 4
3 4 3

Output:
7
*/
