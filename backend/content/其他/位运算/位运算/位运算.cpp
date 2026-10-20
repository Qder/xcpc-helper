#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define endl '\n'
#define ll long long
using namespace std;

// 用法：popcount_ll(x) 数 x 的二进制里 1 的个数，O(1)（GCC/Clang 直接翻成 popcnt 指令）
int popcount_ll(ll x)
{
	return __builtin_popcountll((unsigned ll)x);
}

// 兼容版：没有 __builtin_popcountll 的编译器（MSVC）换成这个，Brian Kernighan，循环次数就是 1 的个数
int popcount_kernighan(ll x)
{
	int cnt = 0;
	while(x) { x &= (x - 1); cnt ++; } //x & (x - 1) 每次消掉最低位的 1
	return cnt;
}

// 用法：lowbit(x) 取最低位的 1（树状数组跳下标就靠它）；x = 0 时自然得 0，不用特判，
// 也正因为 lowbit(0) = 0，树状数组下标必须从 1 开始，否则 add 循环会原地打转
ll lowbit(ll x)
{
	return x & (-x); //-x 是补码取反加 1，只有最低位的 1 和它下面那串 0 保留下来
}

// 用法：top_bit(x) 返回最高位 1 的位置（0-based），x = 0 返回 -1
// 为什么 x = 0 是 -1 而不是 0：位置 0 已经表示「只有最低位是 1」，也就是 x = 1；
// 而 0 一个 1 位都没有，只能拿一个不存在的下标 -1 表示「没有最高位」，否则 x = 0 和 x = 1 会撞在同一个答案上
int top_bit(ll x)
{
	return x ? 63 - __builtin_clzll((unsigned ll)x) : -1; //clzll(0) 是未定义行为，必须先把 0 挡掉
}

// 用法：gray(x) = x ^ (x >> 1)，相邻两个格雷码只差一位；构造「相邻异或和最小」的排列时直接 for 输出 gray(0..n-1)
ll gray(ll x)
{
	return x ^ (x >> 1);
}

int sub_cnt; //枚举到几个子集，等于 2^popcount(mask)，可以拿来验算

// 用法：enumerate_subsets(mask) 遍历 mask 的所有子集（含 mask 自身和空集 0），把循环体那行换成自己的处理
// 注意步长是 s = (s - 1) & mask 而不是 s --；对所有 mask 都跑一遍总复杂度是 O(3^n)，不是 O(4^n)
void enumerate_subsets(int mask)
{
	sub_cnt = 0;
	for(int s = mask; ; s = (s - 1) & mask)
	{
		sub_cnt ++; //这里处理子集 s
		if(s == 0) break;
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int T; cin >> T;
	while(T --)
	{
		ll x; cin >> x;
		cout << popcount_ll(x) << ' ' << lowbit(x) << ' ' << top_bit(x) << endl;
	}
	return 0;
}
/*
Input:
5
0
1
6
123456789
1000000000000000000

Output:
0 0 -1
1 1 0
2 2 2
16 1 26
24 262144 59
*/
