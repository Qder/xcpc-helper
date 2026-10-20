#include<bits/stdc++.h>
#define maxn 200050
#define endl '\n'
#define ll long long
using namespace std;

int n, m, root, mod; //mod ≥ 1 都能跑（全程只有 % 没有除法），mod = 1 时所有答案恒为 0
int a[maxn], wt[maxn]; //a 是原点权，wt 是按 dfn 排好序的点权，线段树上用 wt
vector<int> g[maxn];
int dep[maxn], fa[maxn], sz[maxn], son[maxn];
int top[maxn], dfn[maxn], rnk[maxn], tim;
int order[maxn], ocnt;

// 第一遍：BFS 定序（不递归，长链也不会爆栈），顺手把 fa、dep 填好
void bfs_build(int rt)
{
	queue<int> q; q.push(rt); fa[rt] = 0; dep[rt] = 1;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		order[++ ocnt] = u; //BFS 序里儿子一定在父亲后面，逆序回代就能当"后序"用
		for(int v : g[u]) if(v != fa[u])
		{
			fa[v] = u; dep[v] = dep[u] + 1;
			q.push(v);
		}
	}
}

// 逆序回代求 sz 和重儿子 son：儿子的 sz 都算完了才会轮到父亲，全程不递归
void get_sz_son()
{
	for(int i = 1; i <= n; i ++) sz[i] = 1, son[i] = 0;
	for(int i = ocnt; i >= 2; i --)
	{
		int u = order[i];
		sz[fa[u]] += sz[u];
		if(sz[u] > sz[son[fa[u]]]) son[fa[u]] = u; //sz[0]、son[0] 恒为 0，不需要特判
	}
}

// 第二遍：手写栈模拟 DFS（先重儿子后轻儿子），根那侧的 dfs2 递归在长链时会爆栈
void get_dfn_top(int rt)
{
	stack<pair<int, int> > st; st.push(make_pair(rt, rt));
	while(!st.empty())
	{
		int u = st.top().first, tp = st.top().second; st.pop();
		top[u] = tp; dfn[u] = ++ tim; rnk[tim] = u;
		for(int v : g[u]) if(v != fa[u] && v != son[u]) st.push(make_pair(v, v)); //轻儿子各开一条链
		if(son[u]) st.push(make_pair(son[u], tp)); //重儿子最后压栈、最先出栈，同一条链的 dfn 连续
	} //所以任意子树仍是 dfn 上连续的一段 [dfn[u], dfn[u] + sz[u] - 1]
}

ll sum[maxn << 2], lazy[maxn << 2];

void build(int p, int l, int r)
{
	if(l == r) { sum[p] = wt[l] % mod; return; }
	int mid = (l + r) >> 1;
	build(p << 1, l, mid); build(p << 1 | 1, mid + 1, r);
	sum[p] = (sum[p << 1] + sum[p << 1 | 1]) % mod;
}

void push_down(int p, int l, int r)
{
	if(!lazy[p]) return;
	int mid = (l + r) >> 1;
	lazy[p << 1] = (lazy[p << 1] + lazy[p]) % mod;
	lazy[p << 1 | 1] = (lazy[p << 1 | 1] + lazy[p]) % mod;
	sum[p << 1] = (sum[p << 1] + lazy[p] * (mid - l + 1)) % mod;
	sum[p << 1 | 1] = (sum[p << 1 | 1] + lazy[p] * (r - mid)) % mod;
	lazy[p] = 0;
}

void range_add(int p, int l, int r, int ql, int qr, ll k)
{
	if(ql <= l && r <= qr)
	{
		sum[p] = (sum[p] + k * (r - l + 1)) % mod;
		lazy[p] = (lazy[p] + k) % mod;
		return;
	}
	push_down(p, l, r);
	int mid = (l + r) >> 1;
	if(ql <= mid) range_add(p << 1, l, mid, ql, qr, k);
	if(qr > mid) range_add(p << 1 | 1, mid + 1, r, ql, qr, k);
	sum[p] = (sum[p << 1] + sum[p << 1 | 1]) % mod;
}

ll range_sum(int p, int l, int r, int ql, int qr)
{
	if(ql <= l && r <= qr) return sum[p];
	push_down(p, l, r);
	int mid = (l + r) >> 1; ll res = 0;
	if(ql <= mid) res += range_sum(p << 1, l, mid, ql, qr);
	if(qr > mid) res += range_sum(p << 1 | 1, mid + 1, r, ql, qr);
	return res % mod;
}

// 路径加：每次把链头深的那条链整段加掉，再跳到链头的父亲
void path_add(int u, int v, ll k)
{
	while(top[u] != top[v])
	{
		if(dep[top[u]] < dep[top[v]]) swap(u, v);
		range_add(1, 1, n, dfn[top[u]], dfn[u], k);
		u = fa[top[u]];
	}
	if(dep[u] > dep[v]) swap(u, v);
	range_add(1, 1, n, dfn[u], dfn[v], k); //同一条链上 dfn 连续，直接区间加
}

ll path_sum(int u, int v)
{
	ll res = 0;
	while(top[u] != top[v])
	{
		if(dep[top[u]] < dep[top[v]]) swap(u, v);
		res += range_sum(1, 1, n, dfn[top[u]], dfn[u]);
		u = fa[top[u]];
	}
	if(dep[u] > dep[v]) swap(u, v);
	res += range_sum(1, 1, n, dfn[u], dfn[v]);
	return res % mod;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m >> root >> mod;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	for(int i = 1; i < n; i ++)
	{
		int u, v; cin >> u >> v;
		g[u].push_back(v); g[v].push_back(u);
	}
	bfs_build(root);
	get_sz_son();
	get_dfn_top(root);
	for(int i = 1; i <= n; i ++) wt[i] = a[rnk[i]]; //线段树建在 dfn 序上
	build(1, 1, n);
	for(int i = 1; i <= m; i ++)
	{
		int op, u, v; ll k;
		cin >> op;
		if(op == 1) { cin >> u >> v >> k; path_add(u, v, k % mod); }
		else if(op == 2) { cin >> u >> v; cout << path_sum(u, v) << endl; }
		else if(op == 3) { cin >> u >> k; range_add(1, 1, n, dfn[u], dfn[u] + sz[u] - 1, k % mod); }
		else { cin >> u; cout << range_sum(1, 1, n, dfn[u], dfn[u] + sz[u] - 1) << endl; }
	}
	return 0;
}
/*
Input:
5 5 1 100
1 2 3 4 5
1 2
1 3
3 4
3 5
1 4 5 10
2 4 5
3 3 7
4 3
2 1 1

Output:
42
63
1
*/
