#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1050
#define maxm 105
#define endl '\n'
#define ll long long
using namespace std;

//-- 棋盘状压：n×n 棋盘放 k 个国王，任意两个国王不能相邻（上下左右 + 斜对角），求方案数 --

int n, k;
int state[maxn], num[maxn], tot; //state[]：一行内部的合法状态；num[]：该状态放几个国王；tot：合法状态个数
vector<int> compat[maxn]; //compat[i]：与 state[i] 上下不冲突（同列、斜对角都不撞）的上一行状态下标
ll dp[maxn][maxm], ndp[maxn][maxm]; //dp[i][c]：当前行摆 state[i]、全场共摆 c 个国王的方案数

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> k;
	for(int S = 0; S < (1 << n); S ++) //第一步：先把单行合法状态预处理出来，后面只在合法状态之间枚举
	{
		if(S & (S << 1)) continue; //同一行左右相邻 → 该状态非法
		state[tot] = S;
		num[tot] = __builtin_popcount(S); //顺便预处理每个状态放了几个国王
		tot ++;
	}
	for(int i = 0; i < tot; i ++)
		for(int j = 0; j < tot; j ++)
		{
			int a = state[i], b = state[j];
			if(a & b) continue; //同列冲突（上、下）
			if(a & (b << 1)) continue; //斜对角冲突（把上一行膨胀一格再比）
			if(a & (b >> 1)) continue; //另一个方向的斜对角
			compat[i].push_back(j); //第二步：只把相容的状态对存下来，DP 里不用再判断
		}
	
	dp[0][0] = 1; //下标 0 一定是空状态 0，当作虚拟的第 0 行
	for(int i = 1; i <= n; i ++)
	{
		memset(ndp, 0, sizeof ndp);
		for(int s = 0; s < tot; s ++)
			for(int l : compat[s]) //只枚举“相邻两行不冲突”的状态对，写成全部状态两两枚举就是 O(4^n)
				for(int c = num[s]; c <= k; c ++)
					ndp[s][c] += dp[l][c - num[s]];
		memcpy(dp, ndp, sizeof dp);
	}
	ll ans = 0;
	for(int s = 0; s < tot; s ++) ans += dp[s][k]; //最后一行可以是任意合法状态，把恰好 k 个的方案累起来
	cout << ans;
	return 0;
}

//-- TSP 最短哈密顿回路：dp[state][i] = 走过点集 state、最后停在 i 的最小花费，dist 是 n×n 邻接矩阵 --
// 用法：把 n×n 邻接矩阵读进 vector<vector<int>> w（w[i][j] 是 i 到 j 的边权）后，直接 cout << tsp_min_cycle(w);
ll tsp_min_cycle(const vector<vector<int>> &w)
{
	int n = w.size();
	if(n <= 1) return 0; //只有一个点，回路长度为 0
	const ll INFLL = 0x3f3f3f3f3f3f3f3f;
	int full = 1 << n;
	vector<vector<ll>> f(full, vector<ll>(n, INFLL));
	f[1][0] = 0; //从 0 号点出发
	for(int S = 1; S < full; S ++)
	{
		if(!(S & 1)) continue; //回路必须含起点 0
		for(int i = 0; i < n; i ++)
		{
			if(!(S >> i & 1) || f[S][i] >= INFLL) continue; //不可达状态直接跳过，免得 INF 加边权溢出
			for(int j = 0; j < n; j ++)
			{
				if(S >> j & 1) continue;
				int nS = S | (1 << j);
				f[nS][j] = min(f[nS][j], f[S][i] + w[i][j]);
			}
		}
	}
	ll ans = INFLL;
	for(int i = 1; i < n; i ++)
		if(f[full - 1][i] < INFLL) ans = min(ans, f[full - 1][i] + w[i][0]); //走满 n 个点后还要从 i 回到起点
	return ans;
}
/*
Input:
3 2

Output:
16
*/
