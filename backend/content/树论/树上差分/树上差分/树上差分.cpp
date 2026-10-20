#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100050
#define endl '\n'
using namespace std;

int n, m, LOG;
vector<int> g[maxn];
int dep[maxn], fa[maxn][20];
int diff[maxn], order[maxn], ocnt;

// BFS 定序：顺路把 fa[][0]、dep、倍增表全部填好，全程不递归（长链也不会爆栈）
void bfs_order(int rt)
{
	queue<int> q; q.push(rt); fa[rt][0] = 0; dep[rt] = 1;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		order[++ ocnt] = u;
		for(int v : g[u]) if(v != fa[u][0])
		{
			fa[v][0] = u; dep[v] = dep[u] + 1;
			for(int i = 1; i <= LOG; i ++) fa[v][i] = fa[fa[v][i - 1]][i - 1];
			q.push(v);
		}
	}
}
int lca(int u, int v)
{
	if(dep[u] < dep[v]) swap(u, v);
	int d = dep[u] - dep[v];
	for(int i = 0; i <= LOG; i ++) if(d >> i & 1) u = fa[u][i];
	if(u == v) return u;
	for(int i = LOG; i >= 0; i --) if(fa[u][i] != fa[v][i]) u = fa[u][i], v = fa[v][i];
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
	LOG = 0; while((1 << (LOG + 1)) <= n) LOG ++; //LOG 是满足 2^LOG ≤ n 的最大值
	bfs_order(1);
	for(int i = 1; i <= m; i ++)
	{
		int a, b; cin >> a >> b;
		int l = lca(a, b);
		// 点差分：a、b 各 +1，lca 和 fa[lca] 各 -1（fa[lca] 那次是拦住向上传播）
		// 边差分不同：diff[a]++, diff[b]++, diff[lca] -= 2，最后 sum[u] 表示 u→fa[u] 这条边
		diff[a] ++; diff[b] ++; diff[l] --;
		if(fa[l][0]) diff[fa[l][0]] --;
	}
	for(int i = ocnt; i >= 1; i --) //逆序 BFS 序 = 后序，子树和往父亲身上累加
	{
		int u = order[i];
		diff[fa[u][0]] += diff[u];
	}
	for(int i = 1; i <= n; i ++) cout << diff[i] << endl;
	return 0;
}
/*
Input:
5 3
1 2
2 3
3 4
2 5
1 3
4 5
1 1

Output:
2
2
2
1
1
*/
