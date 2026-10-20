#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define maxm 5050
#define endl '\n'
#define ll long long
using namespace std;

/*
// 不要求装满（就是普通 01 背包）：dp 全部初始化为 0
ll dp[maxm];

	memset(dp, 0, sizeof(dp));
	for(int i = 1; i <= n; i ++)
		for(int j = m; j >= v[i]; j --)
			dp[j] = max(dp[j], dp[j - v[i]] + w[i]);
	cout << dp[m];
*/

//-- 恰好装满 --

// 区别只在初始化：只有 dp[0] 是"能装满"的，别的容量一开始都不可达
// 用 -1 标记不可达，转移时跳过不可达状态，最后 dp[m] == -1 就是装不满
int n, m;
int v[maxn], w[maxn];
ll dp[maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> v[i] >> w[i];
	
	for(int j = 0; j <= m; j ++) dp[j] = -1;
	dp[0] = 0;
	for(int i = 1; i <= n; i ++)
		for(int j = m; j >= v[i]; j --)
			if(dp[j - v[i]] != -1)
				dp[j] = max(dp[j], dp[j - v[i]] + w[i]);
	
	if(dp[m] == -1) cout << -1 << endl; //装不满
	else cout << dp[m] << endl;
	return 0;
}
/*
Input:
3 5
1 3
2 5
4 6

Output:
9
*/
