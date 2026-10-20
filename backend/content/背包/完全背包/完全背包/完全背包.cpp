#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define maxm 5050
#define endl '\n'
#define ll long long
using namespace std;

/*
// 朴素二维版本
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
			if(j >= v[i]) dp[i][j] = max(dp[i][j], dp[i][j - v[i]] + w[i]);
			//和 01 背包的区别：用第 i 行（当前物品）转移，等于这件物品可以反复选
		}
	cout << dp[n][m];
	return 0;
}
*/

//-- 一维优化（容量正向枚举）--

int n, m;
int v[maxn], w[maxn];
ll dp[maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> v[i] >> w[i];
	for(int i = 1; i <= n; i ++)
		for(int j = v[i]; j <= m; j ++) //正向枚举，物品可以反复选
			dp[j] = max(dp[j], dp[j - v[i]] + w[i]);
	cout << dp[m];
	return 0;
}
/*
Input:
2 10
3 4
5 6

Output:
12
*/
