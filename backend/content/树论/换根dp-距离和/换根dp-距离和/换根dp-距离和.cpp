#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 200050
#define endl '\n'
#define ll long long
using namespace std;

struct Edge { int v, w; }; //带权边

int n;
vector<Edge> g[maxn];
int sz[maxn];
ll dp[maxn], ans[maxn]; //dp[u]：u 子树内所有点到 u 的距离和；ans[u]：整棵树所有点到 u 的距离和

void dfs1(int u, int fa) //第一遍自底向上：求 sz 和 dp[1]
{
	sz[u] = 1;
	for(Edge e : g[u]) if(e.v != fa)
	{
		dfs1(e.v, u);
		sz[u] += sz[e.v];
		dp[u] += dp[e.v] + (ll)e.w * sz[e.v]; //子树里每个点都要多走这条边一次
	}
}
void dfs2(int u, int fa) //第二遍自顶向下换根
{
	for(Edge e : g[u]) if(e.v != fa)
	{
		ans[e.v] = ans[u] + (ll)e.w * (n - 2 * sz[e.v]);
		dfs2(e.v, u);
	}
}
// 为什么是 n - 2*sz[v]：根从 u 挪到 v，v 那一侧的 sz[v] 个点各近 w，另外 n - sz[v] 个点各远 w，
// 变化量 = w*(n - sz[v]) - w*sz[v] = w*(n - 2*sz[v])，这个差和 u 的答案本身无关，所以能 O(1) 换根

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i < n; i ++)
	{
		int u, v, w; cin >> u >> v >> w;
		g[u].push_back({v, w}); g[v].push_back({u, w});
	}
	dfs1(1, 0);
	ans[1] = dp[1];
	dfs2(1, 0);
	for(int i = 1; i <= n; i ++) cout << ans[i] << endl;
	return 0;
}
// 递归深度就是链长：本机实测链长 2e4 就爆栈（0xC00000FD），长链要改成 BFS 定序 order[]、先逆序求 sz/dp 再正序换根
//-- 带点权版本：求 Σ val[j] * dist(i, j) --

ll val[maxn], pt_sz[maxn], pt_dp[maxn], pt_ans[maxn], pt_total; //pt_sz 是子树的点权和

void pt_dfs1(int u, int fa)
{
	pt_sz[u] = val[u]; pt_dp[u] = 0;
	for(Edge e : g[u]) if(e.v != fa)
	{
		pt_dfs1(e.v, u);
		pt_sz[u] += pt_sz[e.v];
		pt_dp[u] += pt_dp[e.v] + (ll)e.w * pt_sz[e.v];
	}
}
void pt_dfs2(int u, int fa)
{
	for(Edge e : g[u]) if(e.v != fa)
	{
		pt_ans[e.v] = pt_ans[u] + (ll)e.w * (pt_total - 2 * pt_sz[e.v]); //点数换成权值和，转移式形状不变
		pt_dfs2(e.v, u);
	}
}
// 用法：读点权进 val[1..n]；pt_total = Σval；pt_dfs1(1, 0); pt_ans[1] = pt_dp[1]; pt_dfs2(1, 0);
//       之后 pt_ans[i] 就是 Σ val[j]*dist(i,j)，取最小的 i 就是带点权重心
/*
Input:
4
1 2 1
2 3 2
3 4 3

Output:
10
8
8
14
*/
