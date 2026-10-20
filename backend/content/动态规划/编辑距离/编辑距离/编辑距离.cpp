#include<bits/stdc++.h>
#define maxn 1050
#define endl '\n'
#define ll long long
using namespace std;

// dp[i][j] = 把 a[1..i] 变成 b[1..j] 的最少操作数（增、删、改）
// 边界：a 前 i 个全删 / 空串插入 j 个；字符相同就不用操作，否则 删、增、改 取最小再加 1
int n, m;
char a[maxn], b[maxn];
ll dp[maxn][maxn];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> (a + 1) >> (b + 1);
	n = strlen(a + 1), m = strlen(b + 1);
	
	for(int i = 0; i <= n; i ++) dp[i][0] = i;
	for(int j = 0; j <= m; j ++) dp[0][j] = j;
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= m; j ++)
			if(a[i] == b[j]) dp[i][j] = dp[i - 1][j - 1];
			else dp[i][j] = 1 + min(dp[i - 1][j], min(dp[i][j - 1], dp[i - 1][j - 1])); //删 a[i] / 插入 b[j] / 改 a[i]
	cout << dp[n][m];
	return 0;
}
/*
Input:
horse
ros

Output:
3
*/
