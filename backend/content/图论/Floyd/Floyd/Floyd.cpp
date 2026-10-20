#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 505
#define endl '\n'
#define ll long long
using namespace std;

const ll llinf = 0x3f3f3f3f3f3f3f3f;

int n, m;
ll dis[maxn][maxn]; //Floyd 要 O(n^2) 空间，maxn 只开到 505，题目 n 一般 <= 500

// 用法：dis 先全填 llinf、dis[i][i] 置 0，读边时 dis[u][v] = min(dis[u][v], w)（重边取最小），再调 floyd()
// k 必须是最外层：它是 DP 的阶段（这一轮只允许拿 1..k 当中转点），写反了就成了错的贪心
void floyd()
{
	for(int k = 1; k <= n; k ++)
		for(int i = 1; i <= n; i ++)
		{
			if(dis[i][k] >= llinf) continue; //不可达就别当中转，否则 llinf + 负权会变小
			for(int j = 1; j <= n; j ++)
			{
				if(dis[k][j] >= llinf) continue; //两个都要判：i 可达、k->j 也得可达
				if(dis[i][k] + dis[k][j] < dis[i][j]) dis[i][j] = dis[i][k] + dis[k][j];
			}
		}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= n; j ++)
			dis[i][j] = (i == j ? 0 : llinf);
	for(int i = 1; i <= m; i ++)
	{
		int u, v, w; cin >> u >> v >> w;
		dis[u][v] = min(dis[u][v], (ll)w); //重边取最小，自环不用管（保证无负环）
	}
	floyd();
	for(int i = 1; i <= n; i ++)
	{
		for(int j = 1; j <= n; j ++)
		{
			if(i == j) cout << 0;
			else if(dis[i][j] >= llinf) cout << "INF";
			else cout << dis[i][j];
			cout << " \n"[j == n];
		}
	}
	return 0;
}

//-- Floyd 求无向图最小环：环 = 边 (i,k) + 边 (k,j) + 一条只经过 1..k-1 的 i->j 最短路 --
ll mat[maxn][maxn]; //原始边权另存一份：dis 会被 Floyd 改掉，最小环要的是「直接边的权」

// 用法：无向边读进 mat[u][v] = mat[v][u] = min(mat[u][v], w)，再复制一份到 dis，然后调 min_cycle()，
//       返回最小环的长度，llinf 表示无环（n < 3 或没有三元以上的环）
ll min_cycle()
{
	ll ans = llinf;
	for(int k = 1; k <= n; k ++)
	{
		//此时 dis[i][j] 只用过 1..k-1 当中间点，再加上 i-k、k-j 两条直接边就是一个过 k 的环
		for(int i = 1; i < k; i ++)
			for(int j = i + 1; j < k; j ++)
				if(dis[i][j] < llinf && mat[i][k] < llinf && mat[k][j] < llinf)
					ans = min(ans, dis[i][j] + mat[i][k] + mat[k][j]);
		for(int i = 1; i <= n; i ++) //正常的 Floyd 松弛，别和上面的判环写在一个循环里
		{
			if(dis[i][k] >= llinf) continue;
			for(int j = 1; j <= n; j ++)
				if(dis[k][j] < llinf && dis[i][k] + dis[k][j] < dis[i][j])
					dis[i][j] = dis[i][k] + dis[k][j];
		}
	}
	return ans;
}
/*
Input:
4 6
1 2 3
1 3 8
2 3 -2
3 4 1
4 1 2
2 4 5

Output:
0 3 1 2
1 0 -2 -1
3 6 0 1
2 5 3 0
*/
