#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100005
#define maxm 200005
#define endl '\n'
#define ll long long
using namespace std;

const ll llinf = 0x3f3f3f3f3f3f3f3f;

struct Edge { int v, w; };

int n, m;
vector<Edge> g[maxn]; //有向图，w 可以为负
ll dis[maxn];
int cnt[maxn], inq[maxn];

// 用法：spfa(s) 返回 false 表示存在「从 s 可达」的负环；返回 true 时 dis[1..n] 就是答案，llinf 表示不可达
// 只把 s 入队：负环只有能从 s 走到时才会让 s 的最短路不存在，若把所有点都当源点入队，
// 别的连通块里的负环也会被算进来，本来有答案的图就被误判成 NEGATIVE 了
// cnt[v] 记的是「入队次数」：每次被松弛成功才重新入队，等于它参与了第几轮改进，
// 入队 >= n 次说明某个点的距离被压了 n 轮还没停，只能是在负环上兜圈子（最短路至多 n-1 条边）
// 另一种写法是 cnt[v] = cnt[u] + 1（记当前最短路的边数），>= n 同样判负环，
// 区别：入队次数只看「改进次数」，边数写法直接对应鸽巢原理；重边多、自环多时入队次数更好写
bool spfa(int s)
{
	for(int i = 1; i <= n; i ++) { dis[i] = llinf; cnt[i] = 0; inq[i] = 0; }
	queue<int> q;
	dis[s] = 0; q.push(s); inq[s] = 1; cnt[s] = 1; //s 自己也算一次入队，负自环才判得出来
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		inq[u] = 0;
		for(Edge e : g[u])
		{
			if(dis[u] + e.w >= dis[e.v]) continue; //松弛不动就跳过
			dis[e.v] = dis[u] + e.w;
			if(inq[e.v]) continue; //已经在队里就不重复入队，只更新距离
			q.push(e.v); inq[e.v] = 1;
			if(++ cnt[e.v] >= n) return false;
		}
	}
	return true;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int s;
	cin >> n >> m >> s;
	for(int i = 1; i <= m; i ++)
	{
		int u, v, w; cin >> u >> v >> w;
		g[u].push_back({v, w});
	}
	if(!spfa(s)) cout << "NEGATIVE" << endl;
	else
	{
		for(int i = 1; i <= n; i ++)
		{
			if(dis[i] >= llinf) cout << "INF";
			else cout << dis[i];
			cout << " \n"[i == n];
		}
	}
	return 0;
}
/*
Input:
6 6 1
1 2 4
1 3 2
2 3 -3
3 4 2
4 2 1
2 5 5

Output:
0 4 1 3 9 INF
*/
