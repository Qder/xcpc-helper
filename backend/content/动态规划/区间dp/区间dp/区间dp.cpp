#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 305
#define endl '\n'
#define ll long long
using namespace std;

// dp[l][r] = 把第 l..r 堆石子合并成一堆的最小代价，初值 dp[i][i] = 0（全局数组本来就是 0）
// 区间 dp 三件套：外层枚举长度 len，次层枚举左端点 l（r = l + len - 1），最内层枚举断点 k
// 代价用前缀和 O(1) 算：合并 [l, r] 最后一步的代价就是 sum[r] - sum[l - 1]
int n;
int a[maxn], sum[maxn];
ll dp[maxn][maxn];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i], sum[i] = sum[i - 1] + a[i];
	
	for(int len = 2; len <= n; len ++)
		for(int l = 1; l + len - 1 <= n; l ++)
		{
			int r = l + len - 1;
			dp[l][r] = inf; //求最小值先置成 inf；len >= 2 时 k 至少有 l 这一个取值，不会把 inf 留下来
			for(int k = l; k < r; k ++)
				dp[l][r] = min(dp[l][r], dp[l][k] + dp[k + 1][r] + sum[r] - sum[l - 1]);
		}
	cout << dp[1][n];
	return 0;
}

//-- 环形石子合并（断环成链）--

// 用法：读入 a[1..n] 之后直接 cout << stone_merge_ring(n);
// 把数组复制一倍，长度还是枚举 2..n，左端点枚举 1..2n，答案是 min(dp[i][i + n - 1])
ll dpr[2 * maxn][2 * maxn];
int sum2[2 * maxn];

ll stone_merge_ring(int n)
{
	int N = n << 1;
	for(int i = 1; i <= N; i ++) sum2[i] = sum2[i - 1] + a[(i - 1) % n + 1]; //复制一倍
	for(int len = 2; len <= n; len ++)
		for(int l = 1; l + len - 1 <= N; l ++)
		{
			int r = l + len - 1;
			dpr[l][r] = inf;
			for(int k = l; k < r; k ++)
				dpr[l][r] = min(dpr[l][r], dpr[l][k] + dpr[k + 1][r] + sum2[r] - sum2[l - 1]);
		}
	ll ans = inf;
	for(int i = 1; i <= n; i ++) ans = min(ans, dpr[i][i + n - 1]);
	return ans;
}
/*
Input:
4
1 3 5 2

Output:
22
*/
