#include<bits/stdc++.h>
#define maxn 1000005
#define endl '\n'
using namespace std;

//-- 后缀数组：倍增 + 计数排序，O(n log n) --
// 选倍增而不是 SA-IS：常数小、代码短、n <= 1e6 完全够用，出错也容易调；
// SA-IS 是 O(n) 但实现长得多，模板里性价比不高。

int n;
char s[maxn]; //1-based，s[1..n]
int sa[maxn];   //sa[i]：字典序第 i 小的后缀的起始位置
int rk[maxn];   //rk[i]：后缀 i 的排名（与 sa 互逆）
int ork[maxn];  //上一轮的 rk，用来比较两个二元组
int id2[maxn];  //按第二关键字排好序的后缀编号
int cnt[maxn];
int height[maxn]; //height[i] = LCP(sa[i-1], sa[i])，height[1] = 0

// 用法：build_sa(s, n) 之后 sa[1..n] 就是答案（1-based）；字符按 ASCII 比较。
//       需要 LCP 时再调 build_height(s, n)，height[2..n] 有效。
//       多测要清空 cnt 与各数组，或直接重新调用（内部会覆盖用到的部分）。
void build_sa(const char *s, int n)
{
	int m = 256; //字符集大小，按 unsigned char 处理
	for(int i = 0; i <= m; i ++) cnt[i] = 0;
	for(int i = 1; i <= n; i ++) cnt[rk[i] = (unsigned char)s[i]] ++;
	for(int i = 1; i <= m; i ++) cnt[i] += cnt[i - 1];
	for(int i = n; i >= 1; i --) sa[cnt[rk[i]] --] = i;

	for(int w = 1; w < n; w <<= 1)
	{
		// 按第二关键字排序：i + w 越界（后半段为空）的排最前，其余按上一轮 sa 的顺序平移
		int p = 0;
		for(int i = n - w + 1; i <= n; i ++) id2[++ p] = i;
		for(int i = 1; i <= n; i ++)
			if(sa[i] > w) id2[++ p] = sa[i] - w;

		// 第一关键字计数排序（从后往前扫，保证第二关键字的相对顺序不被打乱）
		for(int i = 0; i <= m; i ++) cnt[i] = 0;
		for(int i = 1; i <= n; i ++) cnt[rk[i]] ++;
		for(int i = 1; i <= m; i ++) cnt[i] += cnt[i - 1];
		for(int i = n; i >= 1; i --) sa[cnt[rk[id2[i]]] --] = id2[i];

		// 重算排名：二元组 (rk[i], rk[i + w]) 不同就排名 +1
		for(int i = 1; i <= n; i ++) ork[i] = rk[i];
		p = 0;
		for(int i = 1; i <= n; i ++)
		{
			int a = sa[i], b = sa[i - 1];
			bool diff = (i == 1) || ork[a] != ork[b]
				|| (a + w <= n ? ork[a + w] : 0) != (b + w <= n ? ork[b + w] : 0);
			if(diff) p ++;
			rk[a] = p;
		}
		if(p == n) break; //每个后缀排名都不同，已经排好
		m = p;
	}
}

// Kasai：height[rk[i]] >= height[rk[i-1]] - 1，所以 k 只减不增，总复杂度 O(n)
void build_height(const char *s, int n)
{
	for(int i = 1, k = 0; i <= n; i ++)
	{
		if(rk[i] == 1) { height[1] = k = 0; continue; }
		if(k) k --;
		int j = sa[rk[i] - 1]; //排名紧挨着的前一个后缀
		while(i + k <= n && j + k <= n && s[i + k] == s[j + k]) k ++;
		height[rk[i]] = k;
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	string str;
	cin >> str;
	n = (int)str.size();
	for(int i = 1; i <= n; i ++) s[i] = str[i - 1];
	build_sa(s, n);
	for(int i = 1; i <= n; i ++) cout << sa[i] << " \n"[i == n];
	return 0;
}
/*
Input:
ababa

Output:
5 3 1 4 2
*/
