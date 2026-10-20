#include<bits/stdc++.h>
#define maxn 100050
#define maxm 100050
#define endl '\n'
using namespace std;

//-- 基础莫队：离线回答 m 个「区间不同数个数」，复杂度 O((n + m) * sqrt(n)) --

int n, m, block, distinct;
int a[maxn], cnt[maxn], ans[maxm]; //a 存离散化后的值（0 起），cnt[x] 是值 x 在当前区间里的出现次数
int val[maxn]; //离散化用的临时数组

struct Query
{
	int l, r, id;
} q[maxm];

void add(int pos) //把位置 pos 加进当前区间
{
	if(++ cnt[a[pos]] == 1) distinct ++; //这个值第一次出现才算多了一个不同数
}

void del(int pos)
{
	if(-- cnt[a[pos]] == 0) distinct --;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++)
	{
		cin >> a[i];
		val[i] = a[i];
	}
	//值域可能很大或带负数，先离散化成 0 ~ tot-1 再用 cnt 计数
	sort(val + 1, val + n + 1);
	int tot = (int)(unique(val + 1, val + n + 1) - val - 1);
	for(int i = 1; i <= n; i ++) a[i] = (int)(lower_bound(val + 1, val + tot + 1, a[i]) - val) - 1;
	
	//本文件块长取 n / sqrt(m)：左指针每个询问最多走 O(block)，共 O(m * block)；
	//右指针在每个左块内单调（奇偶排序）走 O(n)，共 O(n * n / block)；
	//两边平衡得 block = n / sqrt(m)，总复杂度 O(n * sqrt(m))，n、m 同阶时即 O((n + m) * sqrt(n))
	//（m = 0 时 sqrt(0) 会除零，所以用 max(1.0, ...) 兜底；m 很小的时候 block 会顶到 n，也是对的）
	block = max(1, (int)(n / max(1.0, sqrt((double)m))));
	for(int i = 1; i <= m; i ++)
	{
		cin >> q[i].l >> q[i].r;
		q[i].id = i; //离线排序会打乱顺序，答案要按原编号存回去
	}
	//奇偶排序（蛇形）：左端点块升序；同一块内奇数块右端点升序、偶数块降序，
	//这样右指针扫完一块后从右边接着扫回来，少走回头路
	sort(q + 1, q + m + 1, [](const Query &x, const Query &y)
	{
		int bx = (x.l - 1) / block, by = (y.l - 1) / block;
		if(bx != by) return bx < by;
		return (bx & 1) ? x.r > y.r : x.r < y.r;
	});
	
	int curL = 1, curR = 0; //空区间写成 [1, 0]
	for(int i = 1; i <= m; i ++)
	{
		int l = q[i].l, r = q[i].r;
		while(curL > l) add(-- curL); //先扩后缩：先加不会让区间变空
		while(curR < r) add(++ curR);
		while(curL < l) del(curL ++);
		while(curR > r) del(curR --);
		ans[q[i].id] = distinct;
	}
	for(int i = 1; i <= m; i ++) cout << ans[i] << endl;
	return 0;
}
/*
Input:
5 3
1 1 2 1 3
1 3
2 5
1 5

Output:
2
3
3
*/
