#include<bits/stdc++.h>
#define maxn 200050
#define maxm 500050
#define endl '\n'
#define ll long long
using namespace std;

int n, m, tot;
int head[maxn], nxt[maxm], to[maxm];

int dfn[maxn], low[maxn], tim; //dfn 是 DFS 序，low 是子树能回溯到的最小 dfn
int stk[maxn], top; //栈里存"还没确定归属 SCC"的点
bool in_stk[maxn];
int scc[maxn], scc_cnt; //scc[u] = u 所在 SCC 的原始编号（弹栈顺序）

void add_edge(int u, int v)
{
	to[++ tot] = v;
	nxt[tot] = head[u];
	head[u] = tot;
}

//-- 迭代版 Tarjan：手写调用栈，避免递归在长链上爆栈（本机递归链长 2e4 就 0xC00000FD）--
// 递归版就是三句：未访问的 v 递归后 low[u] = min(low[u], low[v])；在栈里的 v 让 low[u] = min(low[u], dfn[v])；
// 出边扫完时若 dfn[u] == low[u] 就弹栈弹到 u。这里用 call_stk + it[]（当前弧）把它原样展开。
int it[maxn], call_stk[maxn], cs;

void tarjan()
{
	for(int s = 1; s <= n; s ++)
	{
		if(dfn[s]) continue; //图不保证连通，每个没访问的点都要当一次根
		cs = 0;
		dfn[s] = low[s] = ++ tim;
		stk[++ top] = s;
		in_stk[s] = true;
		it[s] = head[s];
		call_stk[++ cs] = s;
		while(cs)
		{
			int u = call_stk[cs];
			if(it[u]) //还有出边没扫
			{
				int e = it[u];
				it[u] = nxt[e];
				int v = to[e];
				if(!dfn[v])
				{
					dfn[v] = low[v] = ++ tim;
					stk[++ top] = v;
					in_stk[v] = true;
					it[v] = head[v];
					call_stk[++ cs] = v;
				}
				else if(in_stk[v]) low[u] = min(low[u], dfn[v]); //重边、自环都走这里
			}
			else //出边扫完，模拟递归回溯
			{
				cs --;
				if(cs) low[call_stk[cs]] = min(low[call_stk[cs]], low[u]); //就是递归里那句 low[父亲] = min(low[父亲], low[u])
				if(dfn[u] == low[u]) //u 是这个 SCC 在 DFS 树上最高的点，弹栈弹到 u
				{
					scc_cnt ++;
					int x;
					do
					{
						x = stk[top --];
						in_stk[x] = false;
						scc[x] = scc_cnt;
					} while(x != u);
				}
			}
		}
	}
}

int mn[maxn], ord[maxn], new_id[maxn]; //mn[c] = 第 c 个 SCC 里的最小点编号

// 编号规则：每个 SCC 用最小点编号当代表，按代表点升序编号 1..k。
// Tarjan 弹栈出来是逆拓扑序，和这个规则不一致，所以必须重排一次，不能直接输出 scc[]。
void renumber()
{
	for(int i = 1; i <= scc_cnt; i ++) mn[i] = n + 1;
	for(int u = 1; u <= n; u ++) mn[scc[u]] = min(mn[scc[u]], u);
	for(int i = 1; i <= scc_cnt; i ++) ord[i] = i;
	sort(ord + 1, ord + scc_cnt + 1, [](int x, int y) { return mn[x] < mn[y]; });
	for(int i = 1; i <= scc_cnt; i ++) new_id[ord[i]] = i;
	for(int u = 1; u <= n; u ++) scc[u] = new_id[scc[u]];
}

//-- 缩点建 DAG（带点权 SUM / MAX 的写法）--
int w[maxn]; //原图点权（题目不给就不用管）
int dhead[maxn], dnxt[maxm], dto[maxm], dtot;
int dag_in[maxn];
ll scc_sum[maxn], scc_max[maxn];

// 用法：tarjan() + renumber() 之后调，scc[] 已是 1..k；scc_sum / scc_max 是合并后的点权，可直接在 DAG 上 DP；
//       有重边时 dag_in 会多算，接拓扑排序前要自己判重；多测要清 scc_sum、dag_in 并把 dtot 归零
void build_dag()
{
	for(int i = 1; i <= scc_cnt; i ++) scc_max[i] = -(1LL << 60); //点权可能为负，不能拿 0 起
	for(int u = 1; u <= n; u ++)
	{
		scc_sum[scc[u]] += w[u];
		scc_max[scc[u]] = max(scc_max[scc[u]], (ll)w[u]);
	}
	for(int u = 1; u <= n; u ++)
		for(int e = head[u]; e; e = nxt[e])
		{
			int v = to[e];
			if(scc[u] == scc[v]) continue; //SCC 内部的边缩掉
			dto[++ dtot] = scc[v];
			dnxt[dtot] = dhead[scc[u]];
			dhead[scc[u]] = dtot;
			dag_in[scc[v]] ++;
		}
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
	tarjan();
	renumber();
	cout << scc_cnt << endl;
	for(int u = 1; u <= n; u ++)
	{
		cout << scc[u];
		if(u < n) cout << " ";
	}
	cout << endl;
	return 0;
}
/*
Input:
3 3
1 2
2 3
3 2

Output:
2
1 2 2
*/
