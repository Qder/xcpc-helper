#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 200050
#define endl '\n'
#define ll long long
using namespace std;

struct Edge { int v, w; }; //带权边，边权是正整数

int n, k;
ll ans;
vector<Edge> g[maxn];
bool del[maxn]; //del[u]：u 已经当过重心（等于被删掉），统计时不能再跨过它
int par[maxn], sz[maxn], msz[maxn]; //msz[u]：当前块内 u 的所有儿子子树里最大的那个
int order[maxn], ocnt;

// 求当前块（不含已删除点）的 sz、msz：BFS 定序 + 逆序回代，不递归，长链块也不会爆栈
void get_info(int rt)
{
	ocnt = 0;
	queue<int> q; q.push(rt); par[rt] = 0;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		order[++ ocnt] = u;
		for(Edge e : g[u]) if(e.v != par[u] && !del[e.v])
		{
			par[e.v] = u;
			q.push(e.v);
		}
	}
	for(int i = 1; i <= ocnt; i ++) sz[order[i]] = 1, msz[order[i]] = 0;
	for(int i = ocnt; i >= 2; i --)
	{
		int u = order[i];
		sz[par[u]] += sz[u];
		if(sz[u] > msz[par[u]]) msz[par[u]] = sz[u];
	}
}

// 找重心：删掉它之后最大连通块最小；total - sz[u] 是 u 头顶那一块
int get_centroid(int rt) //要先调用 get_info(rt) 把 sz 算好
{
	int total = sz[rt], best = inf, root = rt;
	for(int i = 1; i <= ocnt; i ++)
	{
		int u = order[i];
		int mx = max(msz[u], total - sz[u]);
		if(mx < best) best = mx, root = u;
	}
	return root;
}

// 收集 st 所在子树里每个点到重心的距离（手写栈，链状块递归会爆栈）
void collect_dist(int st, int from, int d0, vector<int> &vec)
{
	stack<pair<int, pair<int, int> > > stk; //(节点, (父亲, 到重心的距离))
	stk.push(make_pair(st, make_pair(from, d0)));
	while(!stk.empty())
	{
		int u = stk.top().first, p = stk.top().second.first, d = stk.top().second.second;
		stk.pop();
		if(d > k) continue; //边权为正，再往下只会更大；k 比所有路径都大时相当于不剪枝
		vec.push_back(d);
		for(Edge e : g[u]) if(e.v != p && !del[e.v]) stk.push(make_pair(e.v, make_pair(u, d + e.w)));
	} //距离用 int 存，边权能到 1e9 的时候要换 ll
}

// 双指针统计有序数组里 v[i] + v[j] == k 的无序对 (i < j) 个数
// 同一个 i 不会和自己配对，所以 u = u 永远不会被算进去
ll count_pairs_k(vector<int> &v)
{
	ll res = 0;
	int i = 0, j = (int)v.size() - 1;
	while(i < j)
	{
		ll s = (ll)v[i] + v[j];
		if(s == k)
		{
			if(v[i] == v[j]) { ll c = j - i + 1; res += c * (c - 1) / 2; break; } //中间全是同一个值，直接组合数
			ll ci = 1, cj = 1;
			while(i + 1 < j && v[i + 1] == v[i]) i ++, ci ++;
			while(j - 1 > i && v[j - 1] == v[j]) j --, cj ++;
			res += ci * cj; i ++; j --;
		}
		else if(s < k) i ++;
		else j --;
	}
	return res;
}

// 统计经过重心的路径：先把每个子树到重心的距离收集起来（只收集 ≤ k 的）
// 先加"所有点一起配对"的数量，再减去"同一子树内部配对"的数量——后者两点路径不过重心，留给下层递归算，
// 这样每个无序点对恰好被它的 LCA 那一层的重心统计一次，(u,v) 和 (v,u)、u = u 都不会重复
void calc(int root)
{
	vector<int> all;
	all.push_back(0); //重心自身距离为 0，单端点就是重心的路径靠它配上
	for(Edge e : g[root]) if(!del[e.v])
	{
		vector<int> sub;
		collect_dist(e.v, root, e.w, sub);
		sort(sub.begin(), sub.end());
		ans -= count_pairs_k(sub);
		all.insert(all.end(), sub.begin(), sub.end());
	}
	sort(all.begin(), all.end()); //整体只排一次：每个子树单独排会退化成 O(n² log n)（菊花图实测很慢）
	ans += count_pairs_k(all);
}

// 点分治主过程：重心分解的递归深度本身只有 O(log n)，但收集/求大小的递归深度是块大小，
// 所以这里全部改成手写栈 + BFS 定序的写法
void decompose(int rt)
{
	vector<int> stk; stk.push_back(rt);
	while(!stk.empty())
	{
		int u = stk.back(); stk.pop_back();
		get_info(u);
		int root = get_centroid(u);
		calc(root); //先统计经过 root 的路径，再删掉 root
		del[root] = true;
		for(Edge e : g[root]) if(!del[e.v]) stk.push_back(e.v);
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> k;
	for(int i = 1; i < n; i ++)
	{
		int u, v, w; cin >> u >> v >> w;
		g[u].push_back({v, w}); g[v].push_back({u, w});
	}
	decompose(1); //k = 0 时答案为 0：u = u 这种"点对"不算
	cout << ans << endl;
	return 0;
}
/*
Input:
5 3
1 2 1
2 3 1
3 4 1
3 5 1

Output:
2
*/
