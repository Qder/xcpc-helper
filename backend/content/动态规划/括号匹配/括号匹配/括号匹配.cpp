#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 305
#define endl '\n'
#define ll long long
using namespace std;

// dp[l][r] = 让 s[l..r] 合法最少要补几个括号
// 初始化 dp[i][i] = 1：单个括号配不上对，至少要补 1 个（漏了这一行结果基本全错）
// 这里不要 memset(dp, 0x3f, sizeof dp)：len = 2 时要用到 dp[l + 1][r - 1] = dp[r][l]（空区间），它必须是 0
int n;
char s[maxn];
ll dp[maxn][maxn];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> (s + 1);
	n = strlen(s + 1);
	
	for(int i = 1; i <= n; i ++) dp[i][i] = 1;
	for(int len = 2; len <= n; len ++)
		for(int l = 1; l + len - 1 <= n; l ++)
		{
			int r = l + len - 1;
			dp[l][r] = inf;
			if((s[l] == '(' && s[r] == ')') || (s[l] == '[' && s[r] == ']'))
				dp[l][r] = dp[l + 1][r - 1]; //首尾本来就能配对，先试把它俩套起来
			for(int k = l; k < r; k ++)
				dp[l][r] = min(dp[l][r], dp[l][k] + dp[k + 1][r]); //不配对就枚举分割点
		}
	cout << dp[1][n];
	return 0;
}
/*
Input:
([)]

Output:
2
*/
