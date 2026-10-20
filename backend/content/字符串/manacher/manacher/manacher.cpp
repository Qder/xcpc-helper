#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1000005
#define endl '\n'
#define ll long long
using namespace std;

char s[maxn];
int t[2 * maxn + 5]; //变换串：# a # b # ...，两端各放一个哨兵
int p[2 * maxn + 5]; //p[i] = 以 i 为中心向两边成功扩展的次数，数值上就等于该中心在原串里的最长回文长度

// 用法：manacher(s, n)；结果在 p[1..2n]，偶数下标是字符中心（奇回文），奇数下标是 # 中心（偶回文）
// p[i] 这里记的是"扩展步数"，长度直接就是 p[i]；
// 如果把 p[i] 记成"含中心的半径"（初值 1），长度才是 p[i] - 1，两套定义差 1，不要混用
void manacher(const char *s, int n)
{
	t[0] = -2; //左哨兵和任何字符都不等，扩展时就不用判越界
	for(int i = 0; i <= 2 * n; i ++)
		if(i % 2 == 0) t[i + 1] = -1; //分隔符用 -1，避免和输入里的小写字母冲突
		else t[i + 1] = (unsigned char)s[i / 2];
	t[2 * n + 2] = -3; //右哨兵
	int id = 0, mr = 0; //mr = 目前探到的最右回文边界（开区间），id = 这个回文的中心
	p[0] = 0;
	for(int i = 1; i <= 2 * n; i ++)
	{
		if(i < mr) p[i] = min(p[2 * id - i], mr - i); //镜像点 2*id-i 的半径只能继承 mr 以内的部分
		else p[i] = 0;
		while(t[i - p[i] - 1] == t[i + p[i] + 1]) p[i] ++; //从继承的半径继续暴力扩，靠哨兵收边界
		if(i + p[i] > mr) { mr = i + p[i]; id = i; } //只有右边界真的右移才更新 id，否则复杂度会退化
	}
}

// 用法：cout << longest_pal_prefix(s, strlen(s));  返回最长回文前缀长度（会重算一遍 p[]）
int longest_pal_prefix(const char *s, int n)
{
	manacher(s, n);
	int res = 0;
	for(int i = 1; i <= 2 * n; i ++)
		if(i - p[i] <= 1) res = max(res, p[i]); //回文左端顶到第一个分隔符，说明它从原串开头开始
	return res;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> s;
	int n = strlen(s);
	manacher(s, n);
	int mx = 0;
	ll cnt = 0;
	for(int i = 1; i <= 2 * n; i ++)
	{
		mx = max(mx, p[i]); //p[i] 已经是原串回文长度，不用再减 1
		cnt += (p[i] + 1) / 2; //每个中心贡献 (p[i]+1)/2 个回文区间：p 偶贡献 p/2，p 奇贡献 (p+1)/2
	}
	cout << mx << endl << cnt << endl;
	return 0;
}
/*
Input:
abacaba

Output:
7
12
*/
