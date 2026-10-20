#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define maxm 5050
#define endl '\n'
#define ll long long
using namespace std;

/*
// 朴素二维版本：dp[i][j] = 只考虑前 i 个物品、容量 j 的最大价值
int n, m;
int v[maxn], w[maxn], dp[maxn][maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> v[i] >> w[i];
	for(int i = 1; i <= n; i ++)
		for(int j = 0; j <= m; j ++)
		{
			dp[i][j] = dp[i - 1][j];
			if(j >= v[i]) dp[i][j] = max(dp[i][j], dp[i - 1][j - v[i]] + w[i]);
		}
	cout << dp[n][m];
	return 0;
}
*/

//-- 滚动数组优化（最常用）--

int n, m;
int v[maxn], w[maxn]; //v 是体积，w 是价值
ll dp[maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> v[i] >> w[i];
	for(int i = 1; i <= n; i ++)
		for(int j = m; j >= v[i]; j --) //容量反向枚举，保证每件物品只被选 1 次
			dp[j] = max(dp[j], dp[j - v[i]] + w[i]);
	cout << dp[m];
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
