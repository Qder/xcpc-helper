#include<bits/stdc++.h>
#define maxn 1050
#define maxm 1050
#define endl '\n'
#define ll long long
using namespace std;

//-- 二维前缀和（活跃版，O(1) 子矩阵和）+ 二维差分（附函数，O(1) 子矩阵加）--
// 构造：S[i][j] = S[i-1][j] + S[i][j-1] - S[i-1][j-1] + a[i][j]
// 查询：sum(x1,y1,x2,y2) = S[x2][y2] - S[x1-1][y2] - S[x2][y1-1] + S[x1-1][y1-1]（左上角被减两次，要加回来）
// 数组都要开 [maxn + 2][maxm + 2]：查询会用到 S[x1-1]、S[0][*]，差分会在 x2+1 = n+1、y2+1 = m+1 处打标记

int n, m, q, dirty; //dirty = 还有没有没并进 a 的差分修改
ll a[maxn + 2][maxm + 2], S[maxn + 2][maxm + 2], d[maxn + 2][maxm + 2];

void build_sum()
{
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= m; j ++)
			S[i][j] = S[i - 1][j] + S[i][j - 1] - S[i - 1][j - 1] + a[i][j];
}

ll query_sum(int x1, int y1, int x2, int y2)
{
	return S[x2][y2] - S[x1 - 1][y2] - S[x2][y1 - 1] + S[x1 - 1][y1 - 1];
}

//-- 二维差分：子矩阵 (x1,y1)-(x2,y2) 每个元素加 k，只改四个角，O(1) --
// 用法：update(x1, y1, x2, y2, k); 全部改完后 restore() 还原（d 原地做一次二维前缀和即得增量）
// 符号规律：主对角线 +k，副对角线 -k；右边界打在 y2+1 列，下边界打在 x2+1 行，表示从这里开始抵消
void update(int x1, int y1, int x2, int y2, ll k)
{
	d[x1][y1] += k;
	d[x1][y2 + 1] -= k;
	d[x2 + 1][y1] -= k;
	d[x2 + 1][y2 + 1] += k;
}

//还原顺序：i、j 都从小到大原地做前缀和，此时 d[i][j] 就是该位置累计被加了多少
//（顺序不能反：算 d[i][j] 要用到已经算好的 d[i-1][j] 和 d[i][j-1]）
void restore()
{
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= m; j ++)
			d[i][j] += d[i - 1][j] + d[i][j - 1] - d[i - 1][j - 1];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m >> q;
	for(int i = 1; i <= n; i ++)
		for(int j = 1; j <= m; j ++)
			cin >> a[i][j];
	build_sum();
	
	for(int i = 1; i <= q; i ++)
	{
		int op; cin >> op;
		if(op == 1)
		{
			int x1, y1, x2, y2; cin >> x1 >> y1 >> x2 >> y2;
			if(dirty) //查询前要先把攒着的差分修改并进 a，否则 S 还是旧的
			{
				restore();
				for(int x = 1; x <= n; x ++)
					for(int y = 1; y <= m; y ++)
					{
						a[x][y] += d[x][y];
						d[x][y] = 0; //清零，下一轮差分从新矩阵重新记
					}
				build_sum();
				dirty = 0;
			}
			cout << query_sum(x1, y1, x2, y2) << endl;
		}
		else
		{
			int x1, y1, x2, y2; ll k; cin >> x1 >> y1 >> x2 >> y2 >> k; //k 可以是负数
			update(x1, y1, x2, y2, k);
			dirty = 1;
		}
	}
	return 0;
}
/*
Input:
2 3 4
1 2 3
4 5 6
1 1 1 2 3
2 1 1 2 2 -1
1 1 1 2 3
1 2 2 2 3

Output:
21
17
10
*/
