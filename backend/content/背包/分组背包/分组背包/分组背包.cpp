#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 110
#define maxm 5050
#define endl '\n'
#define ll long long
using namespace std;

int n, m;
int s[maxn], v[maxn][maxn], w[maxn][maxn];
ll dp[maxm];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++)
	{
		cin >> s[i];
		for(int j = 1; j <= s[i]; j ++)
			cin >> v[i][j] >> w[i][j];
	}
	
	for(int i = 1; i <= n; i ++)
		for(int j = m; j >= 0; j --) //容量反向，保证每组最多选 1 件
			for(int k = 1; k <= s[i]; k ++)
				if(j >= v[i][k]) dp[j] = max(dp[j], dp[j - v[i][k]] + w[i][k]);
	
	cout << dp[m];
	return 0;
}
/*
Input:
3 5
2
1 3
2 5
1
2 4
1
4 5

Output:
9
*/
