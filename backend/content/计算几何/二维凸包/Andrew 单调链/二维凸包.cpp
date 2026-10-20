#include<bits/stdc++.h>
#define endl '\n'
using namespace std;

//-- 二维凸包：Andrew 单调链，O(n log n) --
// 坐标用 long double：叉积是两个坐标差之积再相减，|坐标| <= 1e6 时叉积 <= 8e12，
// double 的 53 位尾数（约 9e15）刚好够；|坐标| 到 1e9 时叉积可达 8e18，
// 必须 long double 或 __int128，否则共线判断会出错。

struct Point { long double x, y; };

bool cmp_point(const Point &a, const Point &b)
{
	return a.x != b.x ? a.x < b.x : a.y < b.y; //先按 x，再按 y
}

// 叉积 > 0：o -> a -> b 左转；= 0：三点共线；< 0：右转
long double cross(const Point &o, const Point &a, const Point &b)
{
	return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

long double dist(const Point &a, const Point &b)
{
	long double dx = a.x - b.x, dy = a.y - b.y;
	return sqrt(dx * dx + dy * dy);
}

bool same_point(const Point &a, const Point &b) { return a.x == b.x && a.y == b.y; }

// 用法：传入所有点，返回凸包周长（逆时针，重复点先去重，共线点被丢掉）。
// 点数 <= 1 返回 0；只有两个不同点时返回两点距离的 2 倍（退化成一个来回的线段）。
// 若需要凸包顶点序列而不是周长，把最后的求和换成返回 h 即可。
long double convex_hull_perimeter(vector<Point> p)
{
	sort(p.begin(), p.end(), cmp_point);
	p.erase(unique(p.begin(), p.end(), same_point), p.end()); //重复点会让叉积判断退化
	int n = (int)p.size();
	if(n <= 1) return 0;
	vector<Point> h(2 * n); //凸包上最多 2n 个点（上下链各一遍）
	int k = 0;
	//下凸壳：保持左转，cross <= 0 说明右转或共线，弹栈
	for(int i = 0; i < n; i ++)
	{
		while(k >= 2 && cross(h[k - 2], h[k - 1], p[i]) <= 0) k --;
		h[k ++] = p[i];
	}
	//上凸壳：从右往左再扫一遍，t 是本轮下界，防止把下凸壳的点弹掉
	for(int i = n - 2, t = k + 1; i >= 0; i --)
	{
		while(k >= t && cross(h[k - 2], h[k - 1], p[i]) <= 0) k --;
		h[k ++] = p[i];
	}
	h.resize(k - 1); //首尾各存了一次起点，去掉最后一个
	long double res = 0;
	for(int i = 0; i < (int)h.size(); i ++) res += dist(h[i], h[(i + 1) % h.size()]);
	return res;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int n;
	cin >> n;
	vector<Point> p(n);
	for(int i = 0; i < n; i ++) cin >> p[i].x >> p[i].y;
	cout << fixed << setprecision(2) << (double)convex_hull_perimeter(p) << endl;
	return 0;
}
/*
Input:
4
4 8
4 12
5 9.3
7 8

Output:
12.00
*/
