#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 200050
#define endl '\n'
#define ll long long
using namespace std;

const ll NEG = -(1LL << 60); //pre/suf 的哨兵，比任何真实候选都小

struct Edge { int v, w; }; //带权边

int n;
vector<Edge> g[maxn];
ll down1[maxn], down2[maxn], up[maxn], ecc[maxn]; //down1/down2 是向下的最长/次长路，两者必须来自不同儿子

void dfs1(int u, int fa)
{
	for(Edge e : g[u]) if(e.v != fa)
	{
		dfs1(e.v, u);
		ll d = down1[e.v] + e.w;
		if(d > down1[u]) down2[u] = down1[u], down1[u] = d;
		else if(d > down2[u]) down2[u] = d; //等长也要收进次长，否则最长路被排除后没替补
	}
}
void dfs2(int u, int fa)
{
	ecc[u] = max(down1[u], up[u]); //最远点只可能来自两个方向：子树里（down1）或父亲那边上来（up）
	for(Edge e : g[u]) if(e.v != fa)
	{
		ll d = down1[e.v] + e.w;
		ll used = (d == down1[u]) ? down2[u] : down1[u]; //u 的最长路正好穿过 v，就必须换次长
		up[e.v] = max(up[u], used) + e.w;
		dfs2(e.v, u);
	}
}

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
	dfs2(1, 0);
	for(int i = 1; i <= n; i ++) cout << ecc[i] << endl;
	return 0;
}
// 递归深度就是链长：本机实测链长 2e4 就爆栈（0xC00000FD），长链要改成 BFS 定序 order[]、逆序求 down、正序求 up
//-- 前缀/后缀优化版：要排除的贡献不是"最值"、没法用次大值顶替时用 --

void ecc_pref_dfs2(int u, int fa)
{
	ecc[u] = max(down1[u], up[u]);
	vector<int> ch; vector<ll> cand, ew;
	for(Edge e : g[u]) if(e.v != fa) //pre/suf 只能对儿子建
	{
		ch.push_back(e.v);
		cand.push_back(down1[e.v] + e.w);
		ew.push_back(e.w);
	}
	int d = (int)ch.size();
	// 原笔记把父亲 fa 那一支也建进了 pre/suf，等于把 down1[u] 自己又当成一次候选（fa 的最长路往往就穿过 u），
	// 树深 ≥ 2 就会算错（随机树复现：300 棵里错 156 棵）；只对儿子建才是对的
	vector<ll> pre(d + 1, NEG), suf(d + 1, NEG);
	for(int i = 0; i < d; i ++) pre[i + 1] = max(pre[i], cand[i]);
	for(int i = d - 1; i >= 0; i --) suf[i] = max(suf[i + 1], cand[i]);
	for(int i = 0; i < d; i ++)
	{
		ll best = max(up[u], max(pre[i], suf[i + 1])); //挖掉第 i 个儿子后的最优候选
		up[ch[i]] = best + ew[i];
		ecc_pref_dfs2(ch[i], u);
	}
}
// 用法：把 main 里的 dfs2(1, 0) 换成 ecc_pref_dfs2(1, 0)，dfs1 照跑，结果和经典写法完全一样
//       边权非负时 up[u] ≥ 0，所以哨兵 -∞ 和"次大值默认 0"等价；有负权这两种写法都不能用
/*
Input:
4
1 2 5
2 3 1
3 4 2

Output:
8
5
6
8
*/
