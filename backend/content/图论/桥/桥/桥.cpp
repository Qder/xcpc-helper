#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100050
#define maxm 200050
#define endl '\n'
#define ll long long
using namespace std;

int n, m, tot, tim;
int eu[maxm], ev[maxm]; //存下输入的每条边，输出桥的时候要用
int head[maxn], nxt[maxm * 2], to[maxm * 2], eid[maxm * 2]; //一条无向边拆两条弧，共用同一个边编号
int dfn[maxn], low[maxn], par_eid[maxn]; //par_eid[u] = DFS 树里进 u 的那条边的编号
bool is_bridge[maxm];
int dfs_stk[maxn], cur[maxn]; //手写栈：dfs_stk 存点，cur 存每个点已经看到哪条弧

void add_edge(int u, int v, int id)
{
	to[++ tot] = v;
	eid[tot] = id;
	nxt[tot] = head[u];
	head[u] = tot;
}

// 桥：low[v] > dfn[u]（严格大于，割点是 >=）。
// 父边只能按「边的编号 != 父边编号」跳过：若按 v != fa 判，u、v 之间的重边会被跳过，
// low[v] 回不到 dfn[u]，两条重边就都被误判成桥了。
// 这里用数组手写栈代替递归：递归 Tarjan 在 2e4 个点的链上就会 0xC00000FD 爆栈；
// 另一条改法是先 BFS/DFS 定序，再按逆序把 low 回推给父亲，连栈都不用。
void tarjan_bridge()
{
	for(int root = 1; root <= n; root ++)
	{
		if(dfn[root]) continue; //图不保证连通，每个连通块都要跑一次
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
				if(id == par_eid[u]) continue; //只跳过父亲用过的那条边；重边编号不同，照样能走过去
				if(!dfn[v]) //树边
				{
					dfn[v] = low[v] = ++ tim;
					par_eid[v] = id;
					cur[v] = head[v];
					dfs_stk[++ dtop] = v;
				}
				else low[u] = min(low[u], dfn[v]); //返祖边用 dfn[v]，用 low[v] 会把横向边也算进来
			}
			else //u 的弧走完了：出栈，把 low 交给父亲
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
	vector<pair<int, int> > ans;
	for(int i = 1; i <= m; i ++)
		if(is_bridge[i])
		{
			int u = eu[i], v = ev[i];
			if(u > v) swap(u, v); //保证 u < v
			ans.push_back(make_pair(u, v));
		}
	sort(ans.begin(), ans.end()); //pair 自带「先 u 再 v」的比较
	ans.erase(unique(ans.begin(), ans.end()), ans.end()); //同一条点对不可能是两条桥，这里只是保险
	cout << ans.size() << endl;
	for(int i = 0; i < (int)ans.size(); i ++)
		cout << ans[i].first << " " << ans[i].second << endl;
	return 0;
}
/*
Input:
7 8
1 2
2 1
2 3
3 4
4 3
4 5
5 6
6 7

Output:
4
2 3
4 5
5 6
6 7
*/
