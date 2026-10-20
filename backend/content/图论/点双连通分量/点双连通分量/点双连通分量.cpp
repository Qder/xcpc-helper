#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100050
#define maxm 200050
#define endl '\n'
#define ll long long
using namespace std;

int n, m, tot, tim, pt_top, cnt;
int head[maxn], nxt[maxm * 2], to[maxm * 2];
int dfn[maxn], low[maxn], par[maxn];
int dfs_stk[maxn], cur[maxn]; //手写递归栈：dfs_stk 存点，cur 存当前看到哪条弧
int pt_stk[maxn]; //点栈：弹出来的一段（直到儿子 v）再加上割点 u 就是一个点双
vector<int> blk[maxm]; //blk[i] = 第 i 个块的点集，块数最多 m
int ord[maxm];

void add_edge(int u, int v)
{
	to[++ tot] = v;
	nxt[tot] = head[u];
	head[u] = tot;
}

// 点双：dfn[u] <= low[v] 时弹栈（v 是 u 的树儿子），割点 u 同时属于相邻的每个块，所以不去重。
// 题目保证无重边自环，父边才能用「v != par[u]」判；有重边要改成按边编号判。
// 递归 Tarjan 在 2e4 个点的链上会 0xC00000FD 爆栈，这里用数组手写栈，
// 弹块时机和递归版完全一致（u 的所有弧走完、真正回溯给父亲的那一刻）。
void tarjan_dcc()
{
	for(int root = 1; root <= n; root ++)
	{
		if(dfn[root]) continue;
		int dtop = 0;
		dfn[root] = low[root] = ++ tim;
		par[root] = 0;
		cur[root] = head[root];
		dfs_stk[++ dtop] = root;
		pt_stk[++ pt_top] = root;
		while(dtop)
		{
			int u = dfs_stk[dtop], e = cur[u];
			if(e)
			{
				cur[u] = nxt[e];
				int v = to[e];
				if(v == par[u]) continue;
				if(!dfn[v])
				{
					par[v] = u;
					dfn[v] = low[v] = ++ tim;
					cur[v] = head[v];
					pt_stk[++ pt_top] = v;
					dfs_stk[++ dtop] = v;
				}
				else low[u] = min(low[u], dfn[v]); //返祖边，用 dfn[v] 不是 low[v]
			}
			else
			{
				dtop --;
				if(!dtop) continue; //根没有父亲，不用弹块（root 自己留在点栈底也无所谓）
				int p = dfs_stk[dtop];
				low[p] = min(low[p], low[u]);
				if(low[u] >= dfn[p]) //p 是割点（或根），u 这一支和 p 单独成块
				{
					cnt ++;
					while(true)
					{
						int x = pt_stk[pt_top --];
						blk[cnt].push_back(x);
						if(x == u) break;
					}
					blk[cnt].push_back(p); //割点 p 也要算进这个块，且它会出现在相邻的每个块里
				}
			}
		}
	}
}

bool cmp_blk(int a, int b) //先按最小点升序，最小点相同再按点集字典序
{
	if(blk[a][0] != blk[b][0]) return blk[a][0] < blk[b][0];
	return blk[a] < blk[b];
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
		add_edge(v, u);
	}
	tarjan_dcc();
	for(int i = 1; i <= cnt; i ++)
	{
		sort(blk[i].begin(), blk[i].end()); //块内点集升序
		ord[i] = i;
	}
	sort(ord + 1, ord + cnt + 1, cmp_blk);
	cout << cnt << endl;
	for(int i = 1; i <= cnt; i ++)
	{
		int k = ord[i];
		for(int j = 0; j + 1 < (int)blk[k].size(); j ++) cout << blk[k][j] << " ";
		cout << blk[k].back() << endl;
	}
	return 0;
}
/*
Input:
6 7
1 2
2 3
1 3
3 4
4 5
5 6
6 4

Output:
3
1 2 3
3 4
4 5 6
*/
