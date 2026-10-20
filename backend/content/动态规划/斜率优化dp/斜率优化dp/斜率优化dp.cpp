#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 300005
#define endl '\n'
#define ll long long
using namespace std;

/*
//-- 朴素 O(n^2)：dp[i] = min_{j<i} (dp[j] + (s[i] - s[j])^2) --
ll dp[maxn];
ll plain_dp(int N)
{
	for(int i = 1; i <= N; i ++)
		for(int j = 0; j < i; j ++)
			dp[i] = min(dp[i], dp[j] + (s[i] - s[j]) * (s[i] - s[j]));
	return dp[N];
}
*/

//-- 斜率优化 O(n)：把序列切成若干连续段，最小化「每段和的平方」之和 --

int n;
ll a[maxn], s[maxn]; //题目保证 a[i] 是正数，所以 s[] 严格递增
ll dp[maxn];

// 推导：dp[i] = min_{j<i} (dp[j] + (s[i] - s[j])^2)
//             = s[i]^2 + min_{j<i} (dp[j] + s[j]^2 - 2*s[i]*s[j])
// 把决策 j 看成点 (x_j, y_j) = (s[j], dp[j] + s[j]^2)，就是求 min(y_j - k*x_j)，k = 2*s[i]
// 即用斜率 k 的直线去切点集、找最小截距 → 维护下凸壳；a[i] > 0 使 k 单调递增，于是单调队列 O(n)
// 判别：下凸壳上相邻斜率递增，slope(次尾, 尾) >= slope(尾, 新点) 时尾点必不在壳上，弹掉
// 注意：代价、叉积都是 O((Σa)^2) 级别，Σa 超过 3e9 会爆 ll，那种数据要换 __int128
int q[maxn]; //存凸壳上的决策点下标
ll sx[maxn], sy[maxn];

// 叉积代替除法判斜率，避免浮点误差；sx 严格递增，所以两边同乘分母不变号
bool bad(int a, int b, int c)
{
	return (sy[b] - sy[a]) * (sx[c] - sx[b]) >= (sy[c] - sy[b]) * (sx[b] - sx[a]);
}

ll slope_dp(int N)
{
	int head = 1, tail = 0;
	sx[0] = s[0], sy[0] = dp[0] + s[0] * s[0];
	q[++ tail] = 0;
	for(int i = 1; i <= N; i ++)
	{
		ll k = 2 * s[i];
		while(head < tail && sy[q[head + 1]] - sy[q[head]] <= k * (sx[q[head + 1]] - sx[q[head]])) head ++; //队首到次队首的斜率 <= k，队首不优了
		int j = q[head];
		dp[i] = s[i] * s[i] + sy[j] - k * sx[j]; //取队首这个切点
		sx[i] = s[i], sy[i] = dp[i] + s[i] * s[i];
		while(head < tail && bad(q[tail - 1], q[tail], i)) tail --; //新点让队尾变成上凸，弹掉
		q[++ tail] = i;
	}
	return dp[N];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i], s[i] = s[i - 1] + a[i];
	cout << slope_dp(n);
	return 0;
}
/*
Input:
4
2 3 1 4

Output:
30
*/
