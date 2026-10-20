#include<bits/stdc++.h>
#define maxn 200050
#define maxm 500050
#define endl '\n'
using namespace std;

int n, m, tot;
int head[maxn], nxt[maxm << 1], to[maxm << 1], eid[maxm << 1]; //eid 记这条弧属于第几条输入边

int dfn[maxn], low[maxn], tim;
bool cut[maxn];

void add_edge(int u, int v, int id)
{
	to[++ tot] = v; eid[tot] = id; nxt[tot] = head[u]; head[u] = tot;
	to[++ tot] = u; eid[tot] = id; nxt[tot] = head[v]; head[v] = tot;
}

//-- 迭代版 Tarjan 求割点：手写调用栈（递归版在长链上会爆栈，链长 2e4 实测 0xC00000FD）--
// 递归版只有两句判定：非根 u 有儿子 v 满足 low[v] >= dfn[u] 就是割点；根 u 单独判 son > 1 个儿子。
// 这里用 it[]（当前弧）+ par_id[]（走进来的边编号）+ son[] 展开递归，回溯时由儿子更新父亲。
int it[maxn], par_id[maxn], son[maxn], call_stk[maxn], cs;
int root; //当前连通块的 DFS 根，根要单独判

void tarjan_cut()
{
	for(int s = 1; s <= n; s ++)
	{
		if(dfn[s]) continue; //不保证连通
		root = s;
		cs = 0;
		dfn[s] = low[s] = ++ tim;
		it[s] = head[s];
		son[s] = 0;
		par_id[s] = 0; //0 表示没有父边（边编号从 1 起）
		call_stk[++ cs] = s;
		while(cs)
		{
			int u = call_stk[cs];
			if(it[u])
			{
				int e = it[u];
				it[u] = nxt[e];
				if(eid[e] == par_id[u]) continue; //只跳过走进来的那条边：用边编号判，重边照样能回到父亲
				int v = to[e];
				if(!dfn[v])
				{
					son[u] ++;
					dfn[v] = low[v] = ++ tim;
					it[v] = head[v];
					son[v] = 0;
					par_id[v] = eid[e];
					call_stk[++ cs] = v;
				}
				else low[u] = min(low[u], dfn[v]); //返祖边；自环也走这里，dfn[u] 已赋值，等于自己不影响 low
			}
			else
			{
				cs --;
				if(cs)
				{
					int p = call_stk[cs];
					low[p] = min(low[p], low[u]);
					if(p != root && low[u] >= dfn[p]) cut[p] = true; //儿子 u 绕不过 p，p 是割点
				}
				if(u == root && son[u] > 1) cut[u] = true;
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
		int u, v; cin >> u >> v;
		add_edge(u, v, i);
	}
	tarjan_cut();
	int cnt = 0;
	for(int i = 1; i <= n; i ++)
		if(cut[i]) cnt ++;
	cout << cnt << endl;
	for(int i = 1, k = 0; i <= n; i ++)
		if(cut[i])
		{
			cout << i;
			if(++ k < cnt) cout << " ";
		}
	cout << endl; //没有割点时这一行是空行
	return 0;
}
/*
Input:
5 6
1 2
2 3
3 1
2 4
2 4
4 5

Output:
2
2 4
*/
