#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define maxm 5050
#define endl '\n'
#define ll long long
using namespace std;

int n, m;
int v[maxn], w[maxn];
ll dp[maxn][maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> v[i] >> w[i];
	
	//倒着做 DP：dp[i][j] = 从第 i 件到第 n 件里选、容量 j 的最大价值
	for(int i = n; i >= 1; i --)
		for(int j = 0; j <= m; j ++)
		{
			dp[i][j] = dp[i + 1][j];
			if(j >= v[i]) dp[i][j] = max(dp[i][j], dp[i + 1][j - v[i]] + w[i]);
		}
	
	//正着回溯：能选就选，这样得到的方案字典序最小
	int tmpv = m;
	for(int i = 1; i <= n; i ++)
		if(tmpv >= v[i] && dp[i][tmpv] == dp[i + 1][tmpv - v[i]] + w[i])
		{
			cout << i << ' ';
			tmpv -= v[i];
		}
	return 0;
}
/*
Input:
3 5
1 3
2 5
4 6

Output:
1 3
*/
/*
要输出字典序最小的方案，就得让编号小的物品"优先被选中"。
所以 DP 反过来做（编号大的先更新），回溯时正着走。
另外 tmpv >= v[i] 这个判断不能少：容量已经不够时 tmpv - v[i] 是负下标。
（价值为 0 的物品会被顺手选上，最优值不变，但不是严格意义的最小字典序。）
*/
