#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define maxm 1050
#define endl '\n'
#define ll long long
using namespace std;

/*
// 不剪枝的写法（对照用，别抄）：j 一路枚举到 m、k 一路枚举到 j，退化成 O(n*m^2)
// 和下面的区别只有两个 for 的上界少了 min(...) 的 sz 限制
	knap_init(u);
	for(int v : g[u])
	{
		knap_dfs(v);
		for(int j = m; j >= w[u]; j --)
			for(int k = 0; k <= j - w[u]; k ++)
				if(dp[u][j - k] >= 0 && dp[v][k] >= 0)
					dp[u][j] = max(dp[u][j], dp[u][j - k] + dp[v][k]);
		vsz[u] += vsz[v];
		for(int j = vsz[u] + 1; j <= m; j ++) dp[u][j] = dp[u][vsz[u]]; //补尾巴这句也不能省
	}
*/

int n, m;
int w[maxn], val[maxn], fa[maxn]; //w 是体积，val 是价值，fa 是依赖（0 表示没有依赖）
int vsz[maxn]; //vsz 是子树的体积和（不是点数，别和树形 DP 里的 sz 搞混）
vector<int> g[maxn]; //有根树：g[u] 只放儿子；森林建虚根 0 连成一棵
ll dp[maxn][maxm]; //dp[u][j]：子树 u 内、选了 u，总容量不超过 j 的最大价值，-1 表示不可达

void knap_init(int u) //只初始化自己这一行，别在 DFS 前 memset 整个二维表
{
	for(int j = 0; j <= m; j ++) dp[u][j] = -1;
	for(int j = w[u]; j <= m; j ++) dp[u][j] = val[u]; //"不超过 j"：容量够就取 u，所以整段都赋 val[u]
	vsz[u] = w[u];
}

void knap_merge(int u, int v) //把儿子 v 整棵子树当成一个物品组并进 u
{
	//j 倒序：dp[u][j - k] 必须还是"上一组"的结果；k 是分给 v 这组的容量，k = 0 就是不要这组
	//j <= min(m, vsz[u] + vsz[v])、k <= min(j - w[u], vsz[v]) 就是 sz 剪枝：
	//超出两边子树总体积的容量枚举了也没用；去掉它们就是 O(n*m^2)，n = m = 1000 时近 1e9 次内层枚举，必 TLE
	for(int j = min(m, vsz[u] + vsz[v]); j >= w[u]; j --)
		for(int k = 0; k <= min(j - w[u], vsz[v]); k ++)
			if(dp[u][j - k] >= 0 && dp[v][k] >= 0)
				dp[u][j] = max(dp[u][j], dp[u][j - k] + dp[v][k]);
	vsz[u] += vsz[v];
	//"不超过 j"每并完一组都要补尾巴：j 超过已合并总体积时取满整棵子树。
	//不补的话这些位置留的是上一轮的小值，"不要 v 这组"这个分支就丢了，答案会偏小（还会不单调）
	for(int j = vsz[u] + 1; j <= m; j ++) dp[u][j] = dp[u][vsz[u]];
}

void knap_dfs(int u) //后序：先算完所有儿子，再一组一组并进 u
{
	knap_init(u);
	for(int v : g[u])
	{
		knap_dfs(v);
		knap_merge(u, v);
	}
}

//-- 递归转迭代（n = 2e5 的链状树会爆递归栈时用）：把 main 里的 knap_dfs(0) 换成 knap_iter(0) --
int ord[maxn], ocnt;
void knap_iter(int rt)
{
	queue<int> q; q.push(rt); ocnt = 0;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		ord[++ ocnt] = u;
		for(int v : g[u]) q.push(v); //g 已经是有根树的儿子表，不用再判父亲
	}
	for(int i = ocnt; i >= 1; i --) //逆序 = 后序：轮到 u 时，它所有儿子的 dp 都已经算好
	{
		int u = ord[i];
		knap_init(u);
		for(int v : g[u]) knap_merge(u, v);
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++)
	{
		cin >> w[i] >> val[i] >> fa[i];
		g[fa[i]].push_back(i); //依赖 0 的直接挂到虚根 0 底下
	}
	w[0] = val[0] = 0; //虚根 0 体积、价值都是 0，但照样"必选"，它只负责把森林连成一棵
	knap_dfs(0);
	cout << dp[0][m]; //"不超过"语义：答案就是 dp[0][m]
	// 要改成"恰好 j"：knap_init 里只留 dp[u][w[u]] = val[u]、其余 -1（不可达），并且去掉 knap_merge 末尾补尾巴那两行
	// 之后 dp[u][j] 就是"u 必选且容量恰好用掉 j"，答案取 dp[0][m]（允许装不满则写 max(dp[0][j], j <= m)）
	return 0;
}
/*
Input:
4 5
2 3 0
3 4 1
1 5 0
2 9 3

Output:
17
*/
