#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1000005
#define endl '\n'
#define ll long long
using namespace std;

char s[maxn], p[maxn];
int nxt[maxn]; //nxt[i] = p[0..i] 的最长相等真前后缀长度（border 长度）
int ans[maxn], cnt; //ans[1..cnt] = 所有出现位置（1-based，升序）

// 用法：get_next(p, m)；这里统一用 border 长度定义（nxt[0] = 0，失配跳 nxt[j - 1]），
// 另一种写法是 next[0] = -1、next[i] = 失配后跳到的下标，整体比 border 少 1，两种定义不要混用
void get_next(const char *p, int m)
{
	nxt[0] = 0; //p[0] 没有非空真前后缀，必须是 0（写成 -1 就是另一套定义了）
	for(int i = 1, j = 0; i < m; i ++)
	{
		while(j > 0 && p[i] != p[j]) j = nxt[j - 1];
		if(p[i] == p[j]) j ++;
		nxt[i] = j;
	}
}

// 用法：kmp(s, n, p, m) 之后结果在 ans[1..cnt]；函数内部自己建 nxt，文本指针不回溯，O(n + m)
void kmp(const char *s, int n, const char *p, int m)
{
	get_next(p, m);
	cnt = 0;
	for(int i = 0, j = 0; i < n; i ++)
	{
		while(j > 0 && s[i] != p[j]) j = nxt[j - 1];
		if(s[i] == p[j]) j ++;
		if(j == m)
		{
			ans[++ cnt] = i - m + 2; //0-based 起点 i-m+1 转 1-based 再加 1
			j = nxt[j - 1]; //找到一个后不能清零，跳 border 继续找可重叠的下一个
		}
	}
}

// 用法：cout << min_period(p, strlen(p));  |p| 能被周期整除时返回最小循环节长度，否则返回 |p|
int min_period(const char *p, int m)
{
	get_next(p, m);
	int t = m - nxt[m - 1]; //最小候选周期 = m - 最长 border
	return m % t == 0 ? t : m; //不能整除说明只是循环节候选，答案退回 m
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> s >> p;
	int n = strlen(s), m = strlen(p);
	kmp(s, n, p, m);
	cout << cnt << endl;
	for(int i = 1; i <= cnt; i ++) cout << ans[i] << endl;
	return 0;
}
/*
Input:
abababac
aba

Output:
3
1
3
5
*/
