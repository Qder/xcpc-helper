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
// 坐标 1e9 时叉积能到 8e18，贴着 ll 上限，所以一律用 __int128。
__int128 cross(const Point &a, const Point &b, const Point &c)
{
	return (__int128)(b.x - a.x) * (c.y - a.y) - (__int128)(b.y - a.y) * (c.x - a.x);
}

ll dist2(const Point &a, const Point &b)
{
	ll dx = a.x - b.x, dy = a.y - b.y;
	return dx * dx + dy * dy;
}

// Andrew 单调链：排序去重后先扫下凸壳、再扫上凸壳，O(n log n)。
// cross <= 0 就弹栈：共线点不留，保证凸包上相邻三点都是严格拐角。
int convex_hull()
{
	sort(p + 1, p + n + 1);
	m = 0;
	for(int i = 1; i <= n; i ++)
		if(i == 1 || !(p[i] == p[i - 1])) q[++ m] = p[i]; //重复点只留一个
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

// 旋转卡壳：枚举每条边，用叉积比较对踵点到这条边的三角形面积，面积单峰所以 j 单调后移。
// j 在整轮里最多绕凸包一圈，总移动量 O(h)，每条边只做常数次距离比较。
ll rotating_calipers()
{
	if(h == 1) return 0; //全部点重合
	if(h == 2) return dist2(hull[1], hull[2]); //n=2 或全部共线，凸包退化成一条线段
	ll ans = 0;
	int j = 3;
	for(int i = 1; i <= h; i ++)
	{
		int ni = i % h + 1;
		while(cross(hull[i], hull[ni], hull[j % h + 1]) > cross(hull[i], hull[ni], hull[j]))
			j = j % h + 1;
		ans = max(ans, dist2(hull[i], hull[j]));
		ans = max(ans, dist2(hull[ni], hull[j]));
		int nj = j % h + 1; //面积平台有两个端点时，另一端也可能贡献直径
		ans = max(ans, dist2(hull[i], hull[nj]));
		ans = max(ans, dist2(hull[ni], hull[nj]));
	}
	return ans;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> p[i].x >> p[i].y;
	h = convex_hull();
	cout << rotating_calipers() << endl;
	return 0;
}
/*
Input:
9
0 0
4 0
4 3
0 3
2 0
2 2
0 0
4 3
1 1

Output:
25
*/
