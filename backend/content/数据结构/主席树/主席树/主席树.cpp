#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 200005
#define maxm 200005
#define endl '\n'
#define ll long long
using namespace std;

/*
//-- 主席树（可持久化权值线段树）：静态区间第 k 小，值域 1e9 先离散化 --
// 插入 a[1..i] 得到版本 root[i]，区间 [l, r] 的权值线段树 = root[r] - root[l-1]（前缀版本相减），
// 在这棵「差树」上二分，左子树个数 >= k 就往左走，否则往右走并减去左子树的个数。
// 必须动态开点：每次插入只新建根到叶的一条链，其余儿子指向旧版本。
// 空间：每次插入新建 logV + 1 ≈ 19 个点，再算上空树 O(V)，n = 2e5 时实测 4137855 个点，
//       所以 maxn << 5 = 6400160 够用；开 maxn * 20 = 4e6 刚好爆掉，这是主席树最容易踩的空间坑。
*/

int n, m;
int a[maxn];
int vals[maxn], V; //vals[1..V] 是离散化并去重后的值，排名 pos 对应的原值是 vals[pos]

int ls[maxn << 5], rs[maxn << 5], cnt[maxn << 5], idx; //cnt 是这段值域里数的个数
int root[maxn]; //root[i] = 插完 a[1..i] 之后的版本（根节点编号）

int build(int l, int r) //建空树，每个点的 cnt 都是 0；想省这 O(V) 个点，可以让 root[0] = 0 把节点 0 当空节点
{
	int u = ++ idx;
	if(l == r) return u;
	int mid = (l + r) >> 1;
	ls[u] = build(l, mid);
	rs[u] = build(mid + 1, r);
	return u;
}

// 在 pre 版本的基础上插入 pos（权值 +1），返回新版本根节点：复制 pre 的信息再只改一条链
int update(int pre, int l, int r, int pos)
{
	int u = ++ idx;
	ls[u] = ls[pre]; rs[u] = rs[pre]; cnt[u] = cnt[pre] + 1;
	if(l == r) return u;
	int mid = (l + r) >> 1;
	if(pos <= mid) ls[u] = update(ls[pre], l, mid, pos);
	else rs[u] = update(rs[pre], mid + 1, r, pos);
	return u;
}

// 用法：pos = query(root[r], root[l - 1], 1, V, k); cout << vals[pos];  要求 1 <= k <= r - l + 1
int query(int u, int v, int l, int r, int k) //u = root[r]、v = root[l-1]
{
	if(l == r) return l;
	int mid = (l + r) >> 1;
	int left_cnt = cnt[ls[u]] - cnt[ls[v]]; //[l, r] 里落在左半值域的数的个数
	if(k <= left_cnt) return query(ls[u], ls[v], l, mid, k);
	return query(rs[u], rs[v], mid + 1, r, k - left_cnt); //右半边要减去左半边的个数
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) { cin >> a[i]; vals[i] = a[i]; }
	sort(vals + 1, vals + n + 1);
	V = unique(vals + 1, vals + n + 1) - vals - 1; //离散化：把值域压到 [1, V]
	root[0] = build(1, V);
	for(int i = 1; i <= n; i ++)
	{
		int pos = lower_bound(vals + 1, vals + V + 1, a[i]) - vals; //rank 从 1 开始
		root[i] = update(root[i - 1], 1, V, pos);
	}
	while(m --)
	{
		int l, r, k; cin >> l >> r >> k;
		cout << vals[query(root[r], root[l - 1], 1, V, k)] << endl;
	}
	return 0;
}
/*
Input:
5 3
3 1 4 1 5
1 5 3
2 4 1
2 4 3

Output:
3
1
4
*/
