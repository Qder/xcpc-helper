#include<bits/stdc++.h>
#define maxn 5050
#define maxm 10050
#define endl '\n'
#define ll long long
using namespace std;

int n, m;
int head[maxn], nxt[maxm], to[maxm], w[maxm], tot;
ll dis[maxn];
int cnt[maxn]; //cnt[v] = 到 v 的最短路经过的边数，超过 n 条就说明绕出了负环
bool inq[maxn];

void add_edge(int u, int v, int ww)
{
	to[++ tot] = v;
	nxt[tot] = head[u];
	head[u] = tot;
	w[tot] = ww;
}

// 约束 x_v - x_u <= w 就是最短路里的三角不等式 dis[v] <= dis[u] + w，所以连 u -> v、权 w
// 超级源点 0 向所有点连权 0 的边：每个点都有初值，负环也一定从源点可达（否则判不出来）
// 用法：返回 false 表示无解（有负环）；true 时 dis[1..n] 就是一组可行解
bool spfa()
{
	memset(dis, 0x3f, sizeof(dis)); //0x3f 每个字节都是 0x3f，ll 的 INF 也这么写
	queue<int> q;
	dis[0] = 0; q.push(0); inq[0] = true;
	while(!q.empty())
	{
		int u = q.front(); q.pop(); inq[u] = false;
		for(int e = head[u]; e; e = nxt[e])
		{
			int v = to[e];
			if(dis[v] > dis[u] + w[e])
			{
				dis[v] = dis[u] + w[e];
				cnt[v] = cnt[u] + 1;
				if(cnt[v] > n) return false; //n + 1 个点，简单路径最多 n 条边，再松弛就是负环
				if(!inq[v]) { q.push(v); inq[v] = true; }
			}
		}
	}
	return true;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= m; i ++)
	{
		int u, v, ww; cin >> u >> v >> ww;
		add_edge(u, v, ww); //x_v - x_u <= ww
	}
	for(int i = 1; i <= n; i ++) add_edge(0, i, 0);
	if(!spfa()) { cout << "No" << endl; return 0; }
	cout << "Yes" << endl;
	for(int i = 1; i <= n; i ++) cout << dis[i] << " \n"[i == n];
	return 0;
}
/*
Input:
3 3
1 2 3
2 3 -2
3 1 3

Output:
Yes
0 0 -2
*/
