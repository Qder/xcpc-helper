#include<bits/stdc++.h>
#define maxn 1050
#define endl '\n'
using namespace std;

int n, m, k;
vector<int> g[maxn]; //只存左边到右边的边
int match_r[maxn]; //match_r[v] = 右边点 v 现在配的左点，0 表示没配
int vis[maxn], timer; //vis 存时间戳：第 timer 轮 dfs 访问过 v 就跳过，省掉每轮 memset
int col[maxn];

// 用法：dfs(u) 从左边点 u 出发找增广路，找到就沿路取反并返回 true
// 递归深度 = 增广路长度，路径很长（十万级）时照样会爆栈，那种数据换 Hopcroft-Karp 或开大栈
bool dfs(int u)
{
	for(int v : g[u])
	{
		if(vis[v] == timer) continue; //这一轮已经试过 v，再试还是同一条路
		vis[v] = timer;
		if(!match_r[v] || dfs(match_r[v])) //v 空着，或者 v 的原配能腾出别的点
		{
			match_r[v] = u;
			return true;
		}
	}
	return false;
}

// 用法：hungarian() 返回最大匹配数；匹配边就是 (match_r[v], v)，按 u 升序输出时扫一遍左边点即可
int hungarian()
{
	int res = 0;
	for(int u = 1; u <= n; u ++)
	{
		timer ++;
		if(dfs(u)) res ++;
	}
	return res;
}

//-- 二分图判定（染色法）--
// 用法：g 要建成无向图（每条边正反各 push 一次），col[] 会填上 0/1，返回整张图能不能二染色。
// 注意判的是"有没有奇环"，和上面左->右的匹配图不是一套存法。
bool is_bipartite(int N)
{
	for(int i = 1; i <= N; i ++) col[i] = -1;
	for(int s = 1; s <= N; s ++)
	{
		if(col[s] != -1) continue;
		col[s] = 0;
		queue<int> q; q.push(s);
		while(!q.empty())
		{
			int u = q.front(); q.pop();
			for(int v : g[u])
			{
				if(col[v] == -1) { col[v] = col[u] ^ 1; q.push(v); }
				else if(col[v] == col[u]) return false; //相邻同色，说明有奇环
			}
		}
	}
	return true;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m >> k;
	for(int i = 1; i <= k; i ++)
	{
		int u, v; cin >> u >> v;
		g[u].push_back(v);
	}
	int ans = hungarian();
	cout << ans << endl;
	for(int u = 1; u <= n; u ++) //按 u 升序扫，输出天然有序
		for(int v : g[u])
			if(match_r[v] == u) { cout << u << " " << v << endl; break; } //重边只输出一次
	return 0;
}
/*
Input:
5 4 6
1 1
1 2
2 1
3 2
3 3
5 3

Output:
3
1 2
2 1
3 3
*/
