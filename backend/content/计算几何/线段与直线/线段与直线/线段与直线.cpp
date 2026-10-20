#include<bits/stdc++.h>
#define maxn 2005
#define endl '\n'
#define ll long long
using namespace std;

struct Point
{
	ll x, y;
};

int n;
Point a[maxn], b[maxn];

// (b-a) x (c-a)：>0 逆时针，<0 顺时针，=0 共线；坐标 1e9 时用 __int128 防溢出。
__int128 cross(const Point &a, const Point &b, const Point &c)
{
	return (__int128)(b.x - a.x) * (c.y - a.y) - (__int128)(b.y - a.y) * (c.x - a.x);
}

int sgn(__int128 v)
{
	return (v > 0) - (v < 0);
}

ll dist2(const Point &a, const Point &b)
{
	ll dx = a.x - b.x, dy = a.y - b.y;
	return dx * dx + dy * dy;
}

// p 在闭线段 ab 上：先共线，再落在横纵坐标的闭区间里。
bool on_seg(const Point &a, const Point &b, const Point &p)
{
	return cross(a, b, p) == 0 &&
	       min(a.x, b.x) <= p.x && p.x <= max(a.x, b.x) &&
	       min(a.y, b.y) <= p.y && p.y <= max(a.y, b.y);
}

// 跨立实验 + 共线特判；端点接触、共线重叠、退化成点都算相交。
bool seg_cross(const Point &a, const Point &b, const Point &c, const Point &d)
{
	__int128 c1 = cross(a, b, c), c2 = cross(a, b, d);
	__int128 c3 = cross(c, d, a), c4 = cross(c, d, b);
	if(sgn(c1) * sgn(c2) < 0 && sgn(c3) * sgn(c4) < 0) return true; //严格跨立
	return (c1 == 0 && on_seg(a, b, c)) || (c2 == 0 && on_seg(a, b, d)) ||
	       (c3 == 0 && on_seg(c, d, a)) || (c4 == 0 && on_seg(c, d, b));
}

// 用法：point_line_dist(p, a, b) 求点到直线 AB 的距离；line_intersect(a, b, c, d) 求两直线交点（平行返回 {nan, nan}）。
double point_line_dist(const Point &p, const Point &a, const Point &b)
{
	__int128 cr = cross(a, b, p);
	if(cr < 0) cr = -cr;
	long double len = sqrtl((long double)dist2(a, b));
	if(len == 0) return (double)sqrtl((long double)dist2(a, p)); //退化直线按点处理
	return (double)((long double)cr / len);
}

pair<double, double> line_intersect(const Point &a, const Point &b, const Point &c, const Point &d)
{
	__int128 dx1 = b.x - a.x, dy1 = b.y - a.y;
	__int128 dx2 = d.x - c.x, dy2 = d.y - c.y;
	__int128 den = dx1 * dy2 - dy1 * dx2;
	if(den == 0) return {NAN, NAN}; //平行 / 重合 / 有直线退化
	__int128 num = (__int128)(c.x - a.x) * dy2 - (__int128)(c.y - a.y) * dx2;
	long double t = (long double)num / (long double)den;
	return {(double)(a.x + t * (b.x - a.x)), (double)(a.y + t * (b.y - a.y))};
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i].x >> a[i].y >> b[i].x >> b[i].y;
	ll ans = 0;
	for(int i = 1; i <= n; i ++)
		for(int j = i + 1; j <= n; j ++)
			if(seg_cross(a[i], b[i], a[j], b[j])) ans ++;
	cout << ans << endl;
	return 0;
}
/*
Input:
5
0 0 2 0
1 0 3 0
2 0 2 2
2 2 2 2
5 5 6 6

Output:
4
*/
