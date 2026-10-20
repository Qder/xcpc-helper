#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 300005
#define endl '\n'
#define ll long long
using namespace std;

int n, m;
ll a[maxn], s[maxn]; //s 是前缀和

//-- 前缀和 + 单调队列：长度不超过 m 的最大子段和，O(n) --
// dp[i] = s[i] - min_{j ∈ [i-m, i-1]} s[j]，决策点 j 是一个滑动窗口 → 单调队列存窗口内最小的 s[j]
// 先 push 再 query：子段长度至少为 1，j = i-1 必须在窗口里
ll max_subarray(int N, int M)
{
	deque<int> q; //存前缀和下标，队内 s[] 单调递增，队首是窗口内最小的 s[j]
	ll ans = -(1LL << 60); //答案可能是负数（全为负时只能取单个元素），不能用 0 初始化
	for(int i = 1; i <= N; i ++)
	{
		while(!q.empty() && q.front() < i - M) q.pop_front(); //j < i-m 的决策点滑出窗口
		while(!q.empty() && s[q.back()] >= s[i - 1]) q.pop_back(); //队尾比 i-1 大又比它早，永远用不上
		q.push_back(i - 1);
		ans = max(ans, s[i] - s[q.front()]);
	}
	return ans;
}

// 用法：读入 w[1..N] 后调 window_max(N, K, w, res)，res[i] = max(w[i-K+1..i])（i >= K）；要区间最小值把下面第二个 <= 改成 >=
void window_max(int N, int K, ll w[], ll res[])
{
	deque<int> q;
	for(int i = 1; i <= N; i ++)
	{
		while(!q.empty() && q.front() <= i - K) q.pop_front();
		while(!q.empty() && w[q.back()] <= w[i]) q.pop_back();
		q.push_back(i);
		if(i >= K) res[i] = w[q.front()];
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> a[i], s[i] = s[i - 1] + a[i];
	cout << max_subarray(n, m);
	return 0;
}
/*
Input:
5 3
3 -1 2 -5 4

Output:
4
*/
