#include<bits/stdc++.h>
#define maxn 2000005
#define maxm 2000005
#define endl '\n'
using namespace std;

int n, m;
int head[maxn], nxt[maxm], to[maxm], tot;
int dfn[maxn], low[maxn], tim, stk[maxn], top;
int cur_e[maxn], call_stk[maxn], cs; //手写调用栈：call_stk 存点，cur_e 存这个点当前扫到哪条弧
bool in_stk[maxn];
int scc[maxn], scc_cnt;

// 文字编号：变量 i 取真用点 i，取假用点 i + n
int id(int x, int a) { return x + (a ^ 1) * n; }

void add_edge(int u, int v) { to[++ tot] = v; nxt[tot] = head[u]; head[u] = tot; }

// 用法：add_clause(i, a, j, b) 表示 (x_i == a) ∨ (x_j == b)；两条蕴含边 ¬(i==a) -> (j==b)、¬(j==b) -> (i==a)
void add_clause(int i, int a, int j, int b)
{
	add_edge(id(i, a ^ 1), id(j, b));
	add_edge(id(j, b ^ 1), id(i, a));
}

//-- 迭代版 Tarjan：手写调用栈，避免递归在长链上爆栈（本机链长 20000 实测 0xC00000FD）--
// 递归版就三句：没访问的 v 递归后 low[u] = min(low[u], low[v])；在栈里的 v 让 low[u] = min(low[u], dfn[v])；
// 出边扫完时若 dfn[u] == low[u] 就弹栈弹到 u。这里用 call_stk + cur_e 把它原样展开。
void tarjan()
{
	for(int s = 1; s <= 2 * n; s ++)
	{
		if(dfn[s]) continue; //图不一定连通，每个没访问的点都要当一次根
		cs = 0;
		dfn[s] = low[s] = ++ tim;
		stk[++ top] = s;
		in_stk[s] = true;
		cur_e[s] = head[s];
		call_stk[++ cs] = s;
		while(cs)
		{
			int u = call_stk[cs];
			if(cur_e[u]) //还有出边没扫
			{
				int e = cur_e[u];
				cur_e[u] = nxt[e];
				int v = to[e];
				if(!dfn[v])
				{
					dfn[v] = low[v] = ++ tim;
					stk[++ top] = v;
					in_stk[v] = true;
					cur_e[v] = head[v];
					call_stk[++ cs] = v;
				}
				else if(in_stk[v]) low[u] = min(low[u], dfn[v]); //只有栈里的点连出的边才是同一 SCC 的后向边
			}
			else //出边扫完，模拟递归回溯
			{
				cs --;
				if(cs) low[call_stk[cs]] = min(low[call_stk[cs]], low[u]);
				if(dfn[u] == low[u])
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

// 用法：返回 false 无解；true 时 ans[i] = (scc[i] < scc[i + n]) 就是一组解
// 为什么取 SCC 编号小的那侧：Tarjan 是"汇点先出栈"，一个 SCC 弹出时它能到达的 SCC 全弹完了，
// 所以编号越小越靠拓扑序的后面（编号是拓扑逆序）。赋值要沿蕴含边封闭（真文字能推到的都得真），
// 取拓扑序靠后的那侧，它的后继全在它后面，怎么推都不会推回矛盾；取编号大的那侧会出错，
// 例如 (x1 ∨ x2) 两个文字都没有出边，取大就把两个都赋成 0。
// 注：网上常见的 scc[x] > scc[x+n] 是"i+n 当真点"那套编号，这里约定 i 真、i+n 假，方向正好反过来。
bool two_sat()
{
	tarjan();
	for(int i = 1; i <= n; i ++)
		if(scc[i] == scc[i + n]) return false;
	return true;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= m; i ++)
	{
		int a, va, b, vb; cin >> a >> va >> b >> vb;
		add_clause(a, va, b, vb);
	}
	if(!two_sat()) { cout << "No" << endl; return 0; }
	cout << "Yes" << endl;
	for(int i = 1; i <= n; i ++) cout << (scc[i] < scc[i + n]) << " \n"[i == n];
	return 0;
}
/*
Input:
3 3
1 1 2 0
2 1 3 0
3 0 1 0

Output:
Yes
1 1 0
*/
