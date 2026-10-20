#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100050
#define maxm 200050
#define endl '\n'
#define ll long long
using namespace std;

int n, m;
int fa[maxn], sz[maxn];

struct Edge
{
	int u, v, w;
	bool operator<(const Edge &o) const { return w < o.w; } //改成 > 就是最大生成树
} e[maxm];

void init_dsu(int N)
{
	for(int i = 1; i <= N; i ++) fa[i] = i, sz[i] = 1;
}

int find_fa(int x)
{
	if(fa[x] == x) return x;
	return fa[x] = find_fa(fa[x]); //路径压缩
}

bool merge_dsu(int a, int b)
{
	int ra = find_fa(a), rb = find_fa(b);
	if(ra == rb) return false;
	if(sz[ra] < sz[rb]) swap(ra, rb); //按秩合并：小集合挂到大集合下面，树高稳在 O(log n)
	fa[rb] = ra;
	sz[ra] += sz[rb];
	return true;
}

// 用法：e[1..m] 读好之后直接调，返回 MST 总权值；图不连通返回 -1（n == 1 时返回 0，一条边都不选）
ll kruskal()
{
	sort(e + 1, e + m + 1);
	init_dsu(n);
	ll ans = 0; //边权可负，不能拿 0 当"还没选"的标记
	int cnt = 0;
	for(int i = 1; i <= m; i ++)
	{
		if(!merge_dsu(e[i].u, e[i].v)) continue; //两端已经连通，这条边成环，扔掉
		ans += e[i].w;
		if(++ cnt == n - 1) break; //选够 n - 1 条边就得到生成树
	}
	return cnt == n - 1 ? ans : -1; //连边数不够说明图不连通
}

//-- Prim（堆优化）版本：O(m log n)，用法：main 里把 kruskal() 换成 prim_heap() --
int phead[maxn], pnxt[maxm << 1], pto[maxm << 1], pw[maxm << 1], ptot;
int pdist[maxn];
bool pvis[maxn];

void padd(int u, int v, int w)
{
	pto[++ ptot] = v; pw[ptot] = w; pnxt[ptot] = phead[u]; phead[u] = ptot;
}

// 用法：e[1..m] 读好后调，返回 MST 总权值，不连通返回 -1；从 1 号点开始长树
ll prim_heap()
{
	priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > q; //(到已选集合的最小边权, 点)
	for(int i = 1; i <= m; i ++) padd(e[i].u, e[i].v, e[i].w), padd(e[i].v, e[i].u, e[i].w);
	for(int i = 1; i <= n; i ++) pdist[i] = inf;
	pdist[1] = 0;
	q.push(make_pair(0, 1));
	ll ans = 0;
	int cnt = 0;
	while(!q.empty())
	{
		int d = q.top().first, u = q.top().second; q.pop();
		if(pvis[u]) continue; //堆里的过期记录
		pvis[u] = true;
		ans += d;
		if(++ cnt == n) break;
		for(int i = phead[u]; i; i = pnxt[i])
		{
			int v = pto[i];
			if(!pvis[v] && pw[i] < pdist[v]) //Prim 松弛的是边权本身，不是 dist[u] + w
			{
				pdist[v] = pw[i];
				q.push(make_pair(pdist[v], v));
			}
		}
	}
	return cnt == n ? ans : -1;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= m; i ++) cin >> e[i].u >> e[i].v >> e[i].w;
	cout << kruskal() << endl;
	return 0;
}
/*
Input:
4 5
1 2 -1
2 3 2
3 4 3
1 4 10
2 4 4

Output:
4
*/
