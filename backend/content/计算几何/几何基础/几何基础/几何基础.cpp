#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
#define ll long long
using namespace std;

struct Point
{
	ll x, y;
	bool operator < (const Point &o) const //按 x 再 y 排序，Andrew 的第一步
	{
		if(x != o.x) return x < o.x;
		return y < o.y;
	}
	bool operator == (const Point &o) const { return x == o.x && y == o.y; }
};

int n, m, h;
Point p[maxn], q[maxn], hull[maxn]; //q 是去重后的点，hull 是凸包（1-based）

// (b-a) x (c-a)：>0 逆时针，<0 顺时针，=0 共线。
// 整数叉积不掉精度；坐标 1e9 要用 __int128 或改用 long double：
// (b-a) 的分量到 2e9，两个乘积各到 4e18、相减到 8e18，ll 已经贴着上限，这里一律用 __int128。
__int128 cross(const Point &a, const Point &b, const Point &c)
{
	return (__int128)(b.x - a.x) * (c.y - a.y) - (__int128)(b.y - a.y) * (c.x - a.x);
}

// 鞋带公式算面积两倍。逐项最多 1e18，1e5 项累加能到 1e23，必须用 __int128 存中间和；
// 真正的面积两倍不超过 8e18，最后转回 ll 输出。
ll area2(Point *a, int k)
{
	__int128 s = 0;
	for(int i = 1; i <= k; i ++)
	{
		int j = (i == k ? 1 : i + 1);
		s += (__int128)a[i].x * a[j].y - (__int128)a[j].x * a[i].y;
	}
	if(s < 0) s = -s;
	return (ll)s;
}

// Andrew 单调链：排序去重后先扫下凸壳、再扫上凸壳，O(n log n)。
// cross <= 0 就弹栈：共线点不留，hull 里只有严格拐角（题目要保留边上所有点就改成 < 0）。
// 返回 h，hull[1..h] 是逆时针；起点是字典序最小的点，main 里再转到「y 最小、y 同取 x 最小」。
int convex_hull()
{
	sort(p + 1, p + n + 1);
	m = 0;
	for(int i = 1; i <= n; i ++)
		if(i == 1 || !(p[i] == p[i - 1])) q[++ m] = p[i]; //坐标相同的点只留一个
	if(m == 1) { hull[1] = q[1]; return 1; }
	h = 0;
	for(int i = 1; i <= m; i ++)
	{
		while(h >= 2 && cross(hull[h - 1], hull[h], q[i]) <= 0) h --;
		hull[++ h] = q[i];
	}
	for(int i = m - 1, t = h + 1; i >= 1; i --)
	{
		while(h >= t && cross(hull[h - 1], hull[h], q[i]) <= 0) h --;
		hull[++ h] = q[i];
	}
	h --; //上凸壳最后把起点又加了一遍，去掉
	return h;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> p[i].x >> p[i].y;
	h = convex_hull();
	int st = 1; //最左下：先比 y，再比 x
	for(int i = 2; i <= h; i ++)
		if(hull[i].y < hull[st].y || (hull[i].y == hull[st].y && hull[i].x < hull[st].x)) st = i;
	cout << h << endl;
	for(int i = 0; i < h; i ++)
	{
		int id = st + i;
		if(id > h) id -= h;
		cout << hull[id].x << " " << hull[id].y << endl;
	}
	cout << area2(hull, h) << endl;
	return 0;
}
/*
Input:
6
0 0
2 0
4 0
4 3
2 5
0 3

Output:
5
0 0
4 0
4 3
2 5
0 3
32
*/
