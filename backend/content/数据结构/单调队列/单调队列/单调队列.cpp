#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1000050
#define endl '\n'
#define ll long long
using namespace std;

int n, k, a[maxn];

// 用法：vector<int> mn = sliding_min(k);  // 每个长度 k 窗口的最小值，共 n - k + 1 个，n < k 时为空
vector<int> sliding_min(int len)
{
	vector<int> res;
	deque<int> q; //存下标，队首到队尾单调递增，所以队首就是窗口最小值
	for(int i = 1; i <= n; i ++)
	{
		while(!q.empty() && a[q.back()] >= a[i]) q.pop_back(); //新元素更小，队尾那些永远轮不到当最小值
		q.push_back(i);
		while(!q.empty() && q.front() <= i - len) q.pop_front(); //窗口是 [i - len + 1, i]，下标 <= i - len 已经滑出去了
		if(i >= len) res.push_back(a[q.front()]);
	}
	return res;
}

// 用法：vector<int> mx = sliding_max(k);  // 每个长度 k 窗口的最大值
vector<int> sliding_max(int len)
{
	vector<int> res;
	deque<int> q; //存下标，队首到队尾单调递减
	for(int i = 1; i <= n; i ++)
	{
		while(!q.empty() && a[q.back()] <= a[i]) q.pop_back();
		q.push_back(i);
		while(!q.empty() && q.front() <= i - len) q.pop_front();
		if(i >= len) res.push_back(a[q.front()]);
	}
	return res;
}

//-- 单调队列优化 dp：dp[i] = max(dp[j] + w[i])，j 在 [i - k, i - 1] --
// 用法：w 传 1-based（大小 N + 1），返回 dp[N]；dp[0] = 0 是唯一合法起点，
//       把窗口外的决策点弹掉之后，队首就是窗口内 dp 最大的下标
ll monotone_dp(int N, int K, vector<ll> &w)
{
	vector<ll> dp(N + 1, -1e18); //-1e18 表示这个位置走不到；题目保证答案非负时也可以换 -1
	dp[0] = 0;
	deque<int> q;
	q.push_back(0); //起点 0 先进窗口
	for(int i = 1; i <= N; i ++)
	{
		while(!q.empty() && q.front() < i - K) q.pop_front(); //j >= i - K 才是合法决策
		if(!q.empty()) dp[i] = dp[q.front()] + w[i];
		while(!q.empty() && dp[q.back()] <= dp[i]) q.pop_back(); //按下标顺序入队，按 dp 值维护单调递减
		q.push_back(i);
	}
	return dp[N];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> k;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	vector<int> mn = sliding_min(k), mx = sliding_max(k);
	if(!mn.empty()) //n < k 时窗口一个都凑不齐，两行都不输出
	{
		for(int i = 0; i + 1 < (int)mn.size(); i ++) cout << mn[i] << " ";
		cout << mn.back() << endl;
		for(int i = 0; i + 1 < (int)mx.size(); i ++) cout << mx[i] << " ";
		cout << mx.back() << endl;
	}
	return 0;
}
/*
Input:
8 3
1 3 -1 -3 5 3 6 7

Output:
-1 -3 -3 -3 3 3
3 3 5 5 6 7
*/
