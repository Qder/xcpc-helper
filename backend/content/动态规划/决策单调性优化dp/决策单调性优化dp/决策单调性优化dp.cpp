#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 300005
#define endl '\n'
#define ll long long
using namespace std;

const ll llinf = 0x3f3f3f3f3f3f3f3f;

//-- 决策单调性（四边形不等式）：把序列切成恰好 k 段，最小化「每段和的平方」之和 --

int n, k;
ll a[maxn], s[maxn]; //题目保证 a[i] 是正数，所以 s[] 严格递增
ll pre[maxn], cur[maxn]; //滚动数组：pre = dp[t-1][*]，cur = dp[t][*]

// dp[t][i] = min_{j ∈ [t-1, i-1]} (dp[t-1][j] + (s[i] - s[j])^2)
// 代价 w(j, i) = (s[i] - s[j])^2 满足四边形不等式 w(a,c) + w(b,d) <= w(a,d) + w(b,c)（a<=b<=c<=d，s 递增可展开证明）
// ⇒ 最优决策点随 i 单调不减 ⇒ 分治优化：算 mid 时只在 [optl, optr] 里枚举，再把决策区间原样劈给左右两半
// 每层 O(n log n)，总复杂度 O(k * n log n)
void solve(int l, int r, int optl, int optr)
{
	if(l > r) return;
	int mid = (l + r) >> 1;
	int hi = min(optr, mid - 1); //决策点 j 必须 < mid，否则最后一段是空的
	int best = optl;
	cur[mid] = llinf;
	for(int j = optl; j <= hi; j ++)
	{
		if(pre[j] >= llinf) continue; //前 t-1 段都放不下 j 个数，这个决策不可达（也防止 llinf 参与加法后溢出）
		ll v = pre[j] + (s[mid] - s[j]) * (s[mid] - s[j]);
		if(v < cur[mid]) cur[mid] = v, best = j;
	}
	solve(l, mid - 1, optl, best); //左半边的决策点不会超过 best
	solve(mid + 1, r, best, optr); //右半边的决策点不会小于 best
}

//-- 二分队列：另一种决策单调性写法（把每个候选决策记成「从哪个 i 开始它最优」的区间）--
// 用法：cout << binary_queue_dp(n, k);  // 与分治版二选一，复用 pre/cur 两个数组
ll val(int j, int i)
{
	return pre[j] + (s[i] - s[j]) * (s[i] - s[j]);
}

ll binary_queue_dp(int N, int K)
{
	if(K > N || K <= 0) return -1; //每段至少一个数
	for(int i = 0; i <= N; i ++) pre[i] = (i == 0 ? 0ll : llinf);
	for(int t = 1; t <= K; t ++)
	{
		for(int i = 0; i <= N; i ++) cur[i] = llinf;
		deque<pair<int, int>> q; //(决策 j, 从 i = l 开始 j 最优)，按 l 递增
		q.push_back({t - 1, t});
		for(int i = t; i <= N; i ++)
		{
			while(q.size() >= 2 && q[1].second <= i) q.pop_front(); //队首的适用区间已经过去了
			int j = q.front().first;
			cur[i] = pre[j] + (s[i] - s[j]) * (s[i] - s[j]);
			if(i == N) break;
			while(!q.empty() && val(i, q.back().second) <= val(q.back().first, q.back().second)) q.pop_back(); //i 在队尾区间起点就更优，整段取代
			if(q.empty()) q.push_back({i, i + 1});
			else if(val(i, N) <= val(q.back().first, N)) //i 在 [l, N] 里第一次反超的位置二分出来
			{
				int lo = q.back().second, hi = N;
				while(lo < hi)
				{
					int mid = (lo + hi) >> 1;
					if(val(i, mid) <= val(q.back().first, mid)) hi = mid;
					else lo = mid + 1;
				}
				q.push_back({i, lo});
			}
		}
		for(int i = 0; i <= N; i ++) pre[i] = cur[i];
	}
	return pre[N];
}

//-- 朴素 O(k * n^2)：分层 dp，决策点暴力枚举 --
// 用法：cout << plain_dp(n, k);  // n 小的时候拿来对照分治版 / 二分队列版
ll plain_dp(int N, int K)
{
	if(K > N || K <= 0) return -1;
	for(int i = 0; i <= N; i ++) pre[i] = (i == 0 ? 0ll : llinf);
	for(int t = 1; t <= K; t ++)
	{
		for(int i = 0; i <= N; i ++) cur[i] = llinf;
		for(int i = t; i <= N; i ++)
			for(int j = t - 1; j < i; j ++)
				cur[i] = min(cur[i], pre[j] + (s[i] - s[j]) * (s[i] - s[j]));
		for(int i = 0; i <= N; i ++) pre[i] = cur[i];
	}
	return pre[N];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> k;
	for(int i = 1; i <= n; i ++) cin >> a[i], s[i] = s[i - 1] + a[i];
	if(k > n) { cout << -1; return 0; } //段数比数字还多，无解
	for(int i = 0; i <= n; i ++) pre[i] = llinf;
	pre[0] = 0;
	for(int t = 1; t <= k; t ++)
	{
		for(int i = 0; i <= n; i ++) cur[i] = llinf;
		solve(t, n, t - 1, n); //第 t 层：i ∈ [t, n]，决策点 j ∈ [t-1, i-1]
		for(int i = 0; i <= n; i ++) pre[i] = cur[i];
	}
	cout << pre[n];
	return 0;
}
/*
Input:
4 2
2 3 1 4

Output:
50
*/
