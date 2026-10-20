#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 200050
#define endl '\n'
#define ll long long
using namespace std;

int n, centroid, best = inf; //centroid 是重心编号，best 是删掉它之后最大连通块的大小
vector<int> g[maxn];
int sz[maxn], mss[maxn]; //mss[u]：删掉 u 之后最大连通块的大小

// 一遍 DFS 求 sz 和 mss：mss[u] = max(最大子树, n - sz[u])，后一项是 u 头顶那一块
void centroid_dfs(int u, int fa)
{
	sz[u] = 1;
	int mx = 0;
	for(int v : g[u]) if(v != fa)
	{
		centroid_dfs(v, u);
		sz[u] += sz[v];
		mx = max(mx, sz[v]);
	}
	mss[u] = max(mx, n - sz[u]);
	if(mss[u] < best || (mss[u] == best && u < centroid)) best = mss[u], centroid = u; //有两个重心时取编号小的
}

ll dsum;
void dsum_dfs(int u, int fa, int d) //以重心为根，深度就是到重心的距离
{
	dsum += d;
	for(int v : g[u]) if(v != fa) dsum_dfs(v, u, d + 1);
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i < n; i ++)
	{
		int u, v; cin >> u >> v;
		g[u].push_back(v); g[v].push_back(u);
	}
	centroid_dfs(1, 0);
	dsum_dfs(centroid, 0, 0);
	cout << centroid << ' ' << dsum;
	return 0;
}
// 递归深度就是链长：本机实测链长 2e4 就爆栈（0xC00000FD），长链要改成 BFS 定序 order[]、逆序回代
/*
Input:
4
1 2
2 3
3 4

Output:
2 4
*/
