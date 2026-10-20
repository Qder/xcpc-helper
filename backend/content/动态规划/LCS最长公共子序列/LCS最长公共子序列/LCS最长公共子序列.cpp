#include<bits/stdc++.h>
#define maxn 1050
#define endl '\n'
#define ll long long
using namespace std;

// 两个串都按 1-based 存进 a[1..n]、b[1..m]，长度用 strlen(a + 1)，空串也能处理
// dp[cur][j] = a[1..i] 与 b[1..j] 的 LCS，第 i 行只依赖第 i - 1 行和本行左边一格，所以两行滚动
int n, m;
char a[maxn], b[maxn];
ll dp[2][maxn];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> (a + 1) >> (b + 1);
	n = strlen(a + 1), m = strlen(b + 1);
	
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= m; j ++)
		{
			int cur = i & 1, pre = cur ^ 1;
			if(a[i] == b[j]) dp[cur][j] = dp[pre][j - 1] + 1;
			else dp[cur][j] = max(dp[pre][j], dp[cur][j - 1]);
		}
	cout << dp[n & 1][m];
	return 0;
}

//-- 二维完整版 O(nm)（不滚动，好写；n、m 大就用上面那版）--

// 用法：读入 a、b 之后直接 cout << lcs2(n, m);
// 全局数组本来就是 0，第 0 行第 0 列不用再初始化
ll dp2[maxn][maxn];

ll lcs2(int n, int m)
{
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= m; j ++)
			if(a[i] == b[j]) dp2[i][j] = dp2[i - 1][j - 1] + 1;
			else dp2[i][j] = max(dp2[i - 1][j], dp2[i][j - 1]);
	return dp2[n][m];
}
/*
Input:
abcbdab
bdcaba

Output:
4   （bcba）
*/
