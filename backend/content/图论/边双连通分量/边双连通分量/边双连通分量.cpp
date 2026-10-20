#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100050
#define maxm 200050
#define endl '\n'
#define ll long long
using namespace std;

int n, m, tot, tim, ecc_cnt;
int eu[maxm], ev[maxm];
int head[maxn], nxt[maxm * 2], to[maxm * 2], eid[maxm * 2];
int dfn[maxn], low[maxn], par_eid[maxn];
bool is_bridge[maxm];
int dfs_stk[maxn], cur[maxn], q[maxn]; //q 是 BFS 队列
int comp[maxn]; //comp[i] = 点 i 所属边双的编号

void add_edge(int u, int v, int id)
{
	to[++ tot] = v;
	eid[tot] = id;
	nxt[tot] = head[u];
	head[u] = tot;
}

// 求桥：low[v] > dfn[u]；父边靠「边编号 != 父边编号」跳过，重边不算桥。
// 递归 Tarjan 在 2e4 个点的链上会 0xC00000FD 爆栈，所以这里用数组手写栈。
void tarjan_bridge()
{
	for(int root = 1; root <= n; root ++)
	{
		if(dfn[root]) continue;
		int dtop = 0;
		dfn[root] = low[root] = ++ tim;
		par_eid[root] = 0;
		cur[root] = head[root];
		dfs_stk[++ dtop] = root;
		while(dtop)
		{
			int u = dfs_stk[dtop], e = cur[u];
			if(e)
			{
				cur[u] = nxt[e];
				int v = to[e], id = eid[e];
				if(id == par_eid[u]) continue;
				if(!dfn[v])
				{
					dfn[v] = low[v] = ++ tim;
					par_eid[v] = id;
					cur[v] = head[v];
					dfs_stk[++ dtop] = v;
				}
				else low[u] = min(low[u], dfn[v]);
			}
			else
			{
				dtop --;
				if(dtop)
				{
					int p = dfs_stk[dtop];
					low[p] = min(low[p], low[u]);
					if(low[u] > dfn[p]) is_bridge[par_eid[u]] = true;
				}
			}
		}
	}
}

// 边双 = 删掉所有桥之后的连通块。这里用 BFS 染色（DFS 等价，只是不用递归、不怕长链爆栈）。
// 按点号从小到大扫，每个没编过号的点一定是它所在边双里最小的点，于是顺次编号就天然满足
// 「按最小点编号升序」。
// 另一种写法：Tarjan 时另开一个边栈，low[v] > dfn[u] 时弹边，弹出来的边集就是一个边双，
// 不用先求桥再搜第二遍（省一遍遍历，但边集转点集要多判一次重）。
void build_ecc()
{
	for(int s = 1; s <= n; s ++)
	{
		if(comp[s]) continue;
		ecc_cnt ++;
		int h = 1, t = 0;
		q[++ t] = s;
		comp[s] = ecc_cnt;
		while(h <= t)
		{
			int u = q[h ++];
			for(int e = head[u]; e; e = nxt[e])
			{
				if(is_bridge[eid[e]]) continue; //桥不能走
				int v = to[e];
				if(comp[v]) continue;
				comp[v] = ecc_cnt;
				q[++ t] = v;
			}
		}
	}
}

int tree_head[maxn], tree_nxt[maxm * 2], tree_to[maxm * 2], tree_tot; //缩点树最多 2 * 桥数 条弧

void add_tree_edge(int u, int v)
{
	tree_to[++ tree_tot] = v;
	tree_nxt[tree_tot] = tree_head[u];
	tree_head[u] = tree_tot;
}

// 用法：build_ecc() 之后调 ecc_tree()，每个边双缩成一个点、每条桥当一条树边，
//       得到一棵 ecc_cnt 个点的树（连通图就是标准意义的桥树）；
//       之后在 tree_head/tree_to 上跑 LCA，树上两点路径的边数就是它们之间必须经过的桥数。
void ecc_tree()
{
	for(int i = 1; i <= m; i ++)
		if(is_bridge[i])
		{
			add_tree_edge(comp[eu[i]], comp[ev[i]]);
			add_tree_edge(comp[ev[i]], comp[eu[i]]);
		}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= m; i ++)
	{
		cin >> eu[i] >> ev[i];
		add_edge(eu[i], ev[i], i);
		add_edge(ev[i], eu[i], i);
	}
	tarjan_bridge();
	build_ecc();
	cout << ecc_cnt << endl;
	for(int i = 1; i <= n; i ++)
	{
		cout << comp[i];
		if(i < n) cout << " ";
	}
	cout << endl;
	return 0;
}
/*
Input:
8 8
1 2
2 1
2 3
3 4
4 3
4 5
5 6
6 7

Output:
6
1 1 2 2 3 4 5 6
*/
