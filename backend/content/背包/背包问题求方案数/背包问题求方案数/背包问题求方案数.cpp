#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define maxm 5050
#define endl '\n'
#define ll long long
using namespace std;

// 求"恰好装满容量 m"的方案数（每件物品 0/1 次）
// 记 dp[j] 为装满 j 的方案数，加物品就是 dp[j] += dp[j - v[i]]
// dp[0] = 1 表示容量 0 有 1 种方案：什么都不选
int n, m;
int v[maxn];
ll dp[maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> v[i];
	
	dp[0] = 1;
	for(int i = 1; i <= n; i ++)
		for(int j = m; j >= v[i]; j --) //还是反向，保证每件只用一次
			dp[j] += dp[j - v[i]];
	
	cout << dp[m];
	return 0;
}
/*
Input:
4 5
1 2 3 4

Output:
2   （1+4、2+3）
*/
