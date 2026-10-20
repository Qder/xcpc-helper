#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100005
#define maxm 20
#define endl '\n'
#define ll long long
using namespace std;

/*
//-- ST 表：静态区间最大值，预处理 O(n log n)，每次询问 O(1)，不支持修改 --
// 核心是可重复贡献：查询 [l, r] 时取 k = floor(log2(r - l + 1))，
// 用两段长度 2^k、允许重叠的区间拼出答案，重叠不影响最值。
*/

int n, m;
int a[maxn];
int Log[maxn]; //Log[i] = floor(log2(i))，预处理一次，询问时 O(1) 取 k
int st_max[maxn][maxm]; //maxm 在这里当 LOG 用：2^17 = 131072 > maxn，20 列绝对够
int st_min[maxn][maxm]; //区间最小值版
int st_gcd[maxn][maxm]; //区间 GCD 版

// 自己写欧几里得：c++14 没有 std::gcd，负数先取绝对值保证结果非负
int gcd(int a, int b)
{
	if(a < 0) a = -a;
	if(b < 0) b = -b;
	while(b) { int t = a % b; a = b; b = t; }
	return a;
}

// 用法：读入 a[1..n] 之后调用一次 build()，之后查询随便调
//      （如果只用最大值，把 st_min / st_gcd 那两行删掉即可）
void build()
{
	Log[1] = 0; //n = 1 时也必须先定好 Log[1]
	for(int i = 2; i <= n; i ++) Log[i] = Log[i >> 1] + 1;
	for(int i = 1; i <= n; i ++)
		st_max[i][0] = st_min[i][0] = st_gcd[i][0] = a[i]; //长度 2^0 = 1 的区间就是自己
	for(int j = 1; (1 << j) <= n; j ++) //长度 2^j 超过 n 的列不用算，算了也没用
		for(int i = 1; i + (1 << j) - 1 <= n; i ++)
		{
			st_max[i][j] = max(st_max[i][j - 1], st_max[i + (1 << (j - 1))][j - 1]); //[i, i+2^j-1] 拆成等长两半
			st_min[i][j] = min(st_min[i][j - 1], st_min[i + (1 << (j - 1))][j - 1]);
			st_gcd[i][j] = gcd(st_gcd[i][j - 1], st_gcd[i + (1 << (j - 1))][j - 1]);
		}
}

// 用法：cin >> l >> r; cout << query_max(l, r);  要求 1 <= l <= r <= n
int query_max(int l, int r)
{
	int k = Log[r - l + 1];
	return max(st_max[l][k], st_max[r - (1 << k) + 1][k]); //左边一段 + 右边一段，允许重叠
}

// 用法：cout << query_min(l, r);  把 max 换成 min 就行，min 同样可重复贡献
int query_min(int l, int r)
{
	int k = Log[r - l + 1];
	return min(st_min[l][k], st_min[r - (1 << k) + 1][k]);
}

// 用法：cout << query_gcd(l, r);  GCD 也满足可重复贡献：gcd(gcd(S), gcd(T)) = gcd(S ∪ T)，
//       重叠部分重复算不会改变结果，所以两段重叠照样对
int query_gcd(int l, int r)
{
	int k = Log[r - l + 1];
	return gcd(st_gcd[l][k], st_gcd[r - (1 << k) + 1][k]);
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	build();
	while(m --)
	{
		int l, r; cin >> l >> r;
		cout << query_max(l, r) << endl;
	}
	return 0;
}
/*
Input:
5 3
3 1 4 1 5
1 5
2 4
3 3

Output:
5
4
4
*/
