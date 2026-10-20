#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 200050
#define endl '\n'
#define ll long long
using namespace std;

struct Edge { int v, w; }; //带权边

int n;
vector<Edge> g[maxn]; //带权树
vector<int> gu[maxn]; //无权树，只给下面两个备用函数用

ll diam_down[maxn], diameter; //diam_down[u]：从 u 往子树走的最长路

// 树形 DP 求直径 O(n)：边权可以为负，所以只能用这个（两次 DFS/BFS 有负权时会选错端点）
// diam_down 初值取 0，等于把"只含一个点的路径"当长度 0，所以全负的树答案自动是 0
void diam_dfs(int u, int fa)
{
	for(Edge e : g[u]) if(e.v != fa)
	{
		diam_dfs(e.v, u);
		diameter = max(diameter, diam_down[u] + diam_down[e.v] + e.w); //先拿两条不同儿子的链更新答案
		diam_down[u] = max(diam_down[u], diam_down[e.v] + e.w); //再更新 u 向下的最长链，顺序反了同一个儿子会被用两次
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
	diam_dfs(1, 0);
	cout << diameter;
	return 0;
}
//-- 两次 BFS 求无权树直径（每条边权当 1，有负权就废了）--

int bdist[maxn];

int bfs_farthest(int s) //返回离 s 最远的点，最远距离留在 bdist 里
{
	for(int i = 1; i <= n; i ++) bdist[i] = -1;
	queue<int> q; q.push(s); bdist[s] = 0;
	int far = s;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		for(int v : gu[u])
		{
			if(bdist[v] != -1) continue;
			bdist[v] = bdist[u] + 1;
			q.push(v);
			if(bdist[v] > bdist[far]) far = v;
		}
	}
	return far;
}
// 用法：读无权边存进 gu；int p = bfs_farthest(1), q = bfs_farthest(p); 直径 = bdist[q];

//-- 两次 DFS 记录直径路径（只对非负权成立，输出路径上的点）--

int fa[maxn];

pair<int, int> dfs_far(int u, int p, int d) //返回 (最远点, 距离)，顺路记父指针
{
	fa[u] = p;
	pair<int, int> res = make_pair(u, d);
	for(int v : gu[u]) if(v != p)
	{
		pair<int, int> ch = dfs_far(v, u, d + 1);
		if(ch.second > res.second) res = ch;
	}
	return res;
}
// 用法：int a = dfs_far(1, 0, 0).first; int b = dfs_far(a, 0, 0).first;
//       从 b 沿 fa 走到 0 就是直径点序列（反过来是 a→b），长度 = 第二次 DFS 的返回值
// 递归深度就是链长：本机实测链长 2e4 就爆栈（0xC00000FD），长链要改成 BFS 定序 order[]、逆序当后序回代
/*
Input:
5
1 2 -3
2 3 4
3 4 -1
2 5 2

Output:
6
*/
