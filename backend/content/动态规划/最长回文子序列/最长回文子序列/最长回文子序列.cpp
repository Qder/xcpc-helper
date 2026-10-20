#include<bits/stdc++.h>
#define maxn 1050
#define endl '\n'
#define ll long long
using namespace std;

const ll mod = 1e9 + 7;

// dp[i][j] = s[i..j] 的最长回文子序列长度（子序列可以跳字符，不是子串）
// i 从后往前枚举，保证 dp[i + 1][j]、dp[i + 1][j - 1] 都已经算好
// 这里不要 memset：j = i + 1 时要用到 dp[i + 1][i]（空区间）= 0
int n;
char s[maxn];
ll dp[maxn][maxn];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> (s + 1);
	n = strlen(s + 1);
	
	for(int i = n; i >= 1; i --)
	{
		dp[i][i] = 1; //单字符自己就是长度 1 的回文
		for(int j = i + 1; j <= n; j ++)
			if(s[i] == s[j]) dp[i][j] = dp[i + 1][j - 1] + 2; //两端能一起用
			else dp[i][j] = max(dp[i + 1][j], dp[i][j - 1]); //不然丢掉一头
	}
	cout << dp[1][n];
	return 0;
}

//-- 回文子序列的个数（按位置计数，不同位置的相同串算多个，对 1e9 + 7 取模）--

// 用法：读入 s[1..n] 之后直接 cout << count_pal(n);
ll dpp[maxn][maxn];

ll count_pal(int n)
{
	for(int i = 1; i <= n; i ++) dpp[i][i] = 1;
	for(int len = 2; len <= n; len ++)
		for(int l = 1; l + len - 1 <= n; l ++)
		{
			int r = l + len - 1;
			if(s[l] == s[r]) dpp[l][r] = (dpp[l + 1][r] + dpp[l][r - 1] + 1) % mod; //两端相同：左右并集，再加上 s[l..r] 自己这个新的
			else dpp[l][r] = (dpp[l + 1][r] + dpp[l][r - 1] - dpp[l + 1][r - 1] + mod) % mod; //两端不同：容斥减掉中间重复算的
		}
	return dpp[1][n];
}
/*
Input:
character

Output:
5   （carac）
*/
