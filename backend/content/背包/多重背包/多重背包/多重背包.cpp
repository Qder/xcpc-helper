#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 10050
#define maxm 100050
#define endl '\n'
#define ll long long
using namespace std;

/*
// 朴素 O(N*M*S)：直接枚举第 i 种物品取 k 件
int n, m;
int v[maxn], w[maxn], s[maxn];
ll dp[maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> v[i] >> w[i] >> s[i];
	for(int i = 1; i <= n; i ++)
		for(int j = m; j >= v[i]; j --) //容量还是要反向
			for(int k = 1; k <= s[i] && k * v[i] <= j; k ++)
				dp[j] = max(dp[j], dp[j - k * v[i]] + k * w[i]);
	cout << dp[m];
	return 0;
}
*/

//-- 二进制优化 O(N*M*logS)（推荐）--

int n, m, tot;
int v[maxn], w[maxn], s[maxn];
ll dp[maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++)
	{
		int vol, wor, cnt; cin >> vol >> wor >> cnt;
		for(int k = 1; k <= cnt; k <<= 1) //把 cnt 件拆成 1,2,4,... 件一包的 01 物品
		{
			v[++ tot] = vol * k;
			w[tot] = wor * k;
			cnt -= k;
		}
		if(cnt) v[++ tot] = vol * cnt, w[tot] = wor * cnt; //剩下的单独包一包
	}
	for(int i = 1; i <= tot; i ++)
		for(int j = m; j >= v[i]; j --)
			dp[j] = max(dp[j], dp[j - v[i]] + w[i]);
	cout << dp[m];
	return 0;
}
//-- 单调队列优化 O(N*M)：v[i]、M 很大时用 --

// 用法：读入存进 v[1..n], w[1..n], s[1..n] 之后，直接 cout << multi_monotone(n, m);
//       （注意别先做二进制拆分，那样会把 v/w 覆盖掉）
ll multi_monotone(int N, int M)
{
	vector<ll> d(M + 1, 0);
	for(int i = 1; i <= N; i ++)
	{
		int vi = v[i], wi = w[i], si = s[i];
		vector<ll> nd = d;
		// 按余数 r = j % vi 分组，每组内是滑窗最大值（窗口大小 si）
		// 注意 r <= M 这个上界：vi > M 时 (M - r) / vi 会被截断成 0 而不是负数，
		// 少了这个判断，下面 cur = r + p * vi 就会越界
		for(int r = 0; r < vi && r <= M; r ++)
		{
			deque<int> q; //存已取件数 p
			int maxp = (M - r) / vi;
			for(int p = 0; p <= maxp; p ++)
			{
				int cur = r + p * vi;
				ll val = d[cur] - (ll)p * wi;
				while(!q.empty() && d[r + q.back() * vi] - (ll)q.back() * wi <= val) q.pop_back();
				q.push_back(p);
				while(!q.empty() && q.front() < p - si) q.pop_front();
				nd[cur] = max(nd[cur], d[r + q.front() * vi] + (ll)(p - q.front()) * wi);
			}
		}
		d.swap(nd);
	}
	return d[M];
}
/*
Input:
2 10
3 4 2
5 6 1

Output:
10
*/
