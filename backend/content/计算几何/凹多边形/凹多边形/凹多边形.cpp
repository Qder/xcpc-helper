#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
#define ll long long
using namespace std;

struct Point
{
	ll x, y;
};

int n, q;
Point p[maxn];

// (b-a) x (c-a)。整数叉积没有精度问题，能用整数就别换 double；
// 坐标 1e9 时乘法到 4e18、相减到 8e18，ll 贴上限，统一用 __int128。
__int128 cross(const Point &a, const Point &b, const Point &c)
{
	return (__int128)(b.x - a.x) * (c.y - a.y) - (__int128)(b.y - a.y) * (c.x - a.x);
}

// 点 c 在不在线段 ab 上：先要求共线，再要求落在包围盒内（不要用除法算参数）。
bool on_seg(const Point &a, const Point &b, const Point &c)
{
	if(cross(a, b, c) != 0) return false;
	return min(a.x, b.x) <= c.x && c.x <= max(a.x, b.x) &&
	       min(a.y, b.y) <= c.y && c.y <= max(a.y, b.y);
}

// 鞋带公式，凹凸通用：把 O->p_i->p_{i+1} 的有向面积相加，凹角处的负贡献会自动抵消。
// 逐项 1e18、1e5 项累加能到 1e23，中间用 __int128；结果面积两倍不超过 8e18，转 ll 返回。
// 用法：ll s2 = polygon_area2(p, n); //p 是 1-based 顶点数组，返回 |面积| * 2
ll polygon_area2(Point *a, int k)
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

// CCW 输入下所有拐角 cross >= 0 才是凸（允许共线）；出现一个负号就是凹点。
// 用法：bool convex = is_convex(p, n); //顶点必须按 CCW 给出
bool is_convex(Point *a, int k)
{
	for(int i = 1; i <= k; i ++)
	{
		int pre = (i == 1 ? k : i - 1);
		int nxt = (i == k ? 1 : i + 1);
		if(cross(a[pre], a[i], a[nxt]) < 0) return false;
	}
	return true;
}

// 射线法，凹凸通用，O(n)：
// 1) 先判 ON：cross == 0 且 q 在边的包围盒里，纯整数精确，不依赖 EPS；
// 2) (a.y > y) != (b.y > y) 是半开区间，只把边的下端点算一次，射线正好穿过顶点时不会重复计数；
// 3) 交点 x 和 q.x 的比较交叉相乘成整数，D = b.y-a.y 的正负决定不等号方向，避免除法；
//    交点 x 恰好等于 q.x 就是点在边上，第 1 步已经返回 ON，所以这里用严格 > / <。
// 返回 0=OUT，1=IN，2=ON。
int point_in_polygon(const Point &c)
{
	bool inside = false;
	for(int i = 1; i <= n; i ++)
	{
		int j = (i == n ? 1 : i + 1);
		const Point &a = p[i], &b = p[j];
		if(on_seg(a, b, c)) return 2;
		bool above_a = a.y > c.y;
		bool above_b = b.y > c.y;
		if(above_a == above_b) continue; //水平边、或整条边在射线同侧，都不产生交点
		__int128 lhs = (__int128)(b.x - a.x) * (c.y - a.y);
		__int128 rhs = (__int128)(c.x - a.x) * (b.y - a.y);
		if(above_b ? lhs > rhs : lhs < rhs) inside = !inside; //交点严格在 c 右侧才计数
	}
	return inside ? 1 : 0;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> p[i].x >> p[i].y;
	cin >> q;
	while(q --)
	{
		Point c; cin >> c.x >> c.y;
		int res = point_in_polygon(c);
		if(res == 2) cout << "ON" << endl;
		else if(res == 1) cout << "IN" << endl;
		else cout << "OUT" << endl;
	}
	return 0;
}
/*
Input:
6
0 0
4 0
4 2
2 2
2 4
0 4
15
1 1
3 1
1 3
3 3
1 2
2 2
2 0
2 4
0 2
3 2
2 3
5 2
0 5
4 0
4 2

Output:
IN
IN
IN
OUT
IN
ON
ON
ON
ON
ON
ON
OUT
OUT
ON
ON
*/
