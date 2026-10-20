#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1005
#define endl '\n'
#define ll long long
using namespace std;

int n;
int a[maxn][maxn];
ll memo[maxn][maxn];
bool vis[maxn][maxn];

/*
//-- 朴素递归：同一个 (i, j) 会被左上、右上各算一次，n = 1000 时是指数级，只用来对照 --
ll dfs_naive(int i, int j)
{
	if(i == n) return a[n][j];
	return a[i][j] + max(dfs_naive(i + 1, j), dfs_naive(i + 1, j + 1));
}
*/

// 用法：dfs(1, 1) 返回从塔顶走到底的最大路径和
// memo 初值不能用 -1 判空：路径和本身可以是 -1，同一个 -1 既表示「没算过」又表示「答案是 -1」，
// 负数的格子会被反复重算，最坏退化成朴素递归。这里用 vis 单独记「算没算过」，memo 里放多负都不影响
ll dfs(int i, int j)
{
	if(i == n) return a[n][j]; //到底了，没有分支
	if(vis[i][j]) return memo[i][j];
	vis[i][j] = true;
	return memo[i][j] = a[i][j] + max(dfs(i + 1, j), dfs(i + 1, j + 1));
}

// 用法：memo 全填 -1 后调 memo_fib(n, memo)，返回第 n 项斐波那契（n <= 92 时 ll 不溢出）
// 记忆化 = 加缓存的递归：算过的状态直接查表，没算过的先算完存表再返回
ll memo_fib(int n, ll f[])
{
	if(n <= 1) return n;
	if(f[n] != -1) return f[n];
	return f[n] = memo_fib(n - 1, f) + memo_fib(n - 2, f);
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= i; j ++)
			cin >> a[i][j];
	cout << dfs(1, 1) << endl; //n <= 1000，递归深度 1000，栈完全够
	return 0;
}
/*
Input:
4
-2
1 -3
4 -1 2
-5 3 0 -4

Output:
6
*/
