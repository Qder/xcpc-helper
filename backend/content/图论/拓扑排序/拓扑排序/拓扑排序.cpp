#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100050
#define maxm 200050
#define endl '\n'
#define ll long long
using namespace std;

int n, m, tot;
int head[maxn], nxt[maxm], to[maxm], in_deg[maxn], ans[maxn];

// 用法：add_edge(u, v) 建一条 u -> v 的有向边（顺手维护入度），建完图调用 topo()，
//       返回 false 表示有环；返回 true 时答案在 ans[1..n]
void add_edge(int u, int v)
{
	to[++ tot] = v;
	nxt[tot] = head[u];
	head[u] = tot;
	in_deg[v] ++; //自环 u -> u 也会让 in_deg[u] 加一，于是它永远入不了队，最后正好判成有环
}

// 注意：topo() 会把 in_deg 减空；多测就要清 head[]、in_deg[]（要清到出现过的最大 n）并把 tot 归零
bool topo()
{
	priority_queue<int, vector<int>, greater<int> > q; //小根堆：每次取编号最小的入度 0 点，得到字典序最小拓扑序
	for(int i = 1; i <= n; i ++)
		if(in_deg[i] == 0) q.push(i);
	int cnt = 0;
	while(!q.empty())
	{
		int u = q.top(); q.pop();
		ans[++ cnt] = u;
		for(int e = head[u]; e; e = nxt[e])
		{
			in_deg[to[e]] --; //重边就减两次，减到 0 才入队，不影响正确性
			if(in_deg[to[e]] == 0) q.push(to[e]);
		}
	}
	return cnt == n; //出队点数不足 n，说明剩下的点都被环卡住了
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= m; i ++)
	{
		int u, v; cin >> u >> v;
		add_edge(u, v);
	}
	if(!topo()) cout << -1 << endl;
	else
	{
		for(int i = 1; i + 1 <= n; i ++) cout << ans[i] << " ";
		cout << ans[n] << endl;
	}
	return 0;
}
/*
Input:
5 5
4 1
5 2
4 3
3 5
1 5

Output:
4 1 3 5 2
*/
