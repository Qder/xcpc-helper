#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn (1 << 18)
#define endl '\n'
#define ll long long
using namespace std;

int n;
ll f[maxn]; //f[S] 先装原数组 a[S]，做完变换就是子集和

//-- 子集和（Zeta 变换）：f[S] = Σ_{T⊆S} a[T]，O(2^n * n) --
// 位循环必须在外层：第 i 轮里 f[S ^ (1<<i)] 存的还是上一轮的值（它的第 i 位是 0，本轮不会被改）
// 高位前缀和的视角：子集和就是「n 维、每维长度 2」的前缀和，逐维推进，不要容斥
void subset_sum(int bit)
{
	for(int i = 0; i < bit; i ++)
		for(int S = 0; S < (1 << bit); S ++)
			if(S >> i & 1) f[S] += f[S ^ (1 << i)];
}

// 用法：原数组（重新读或拷一份）放回 f[] 后调 superset_sum(n)，f[S] 就是 g[S]；它是子集和的"补集视角"，逆变换即莫比乌斯反演/高维差分——把这里的 += 换成 -= 再跑一遍
void superset_sum(int bit)
{
	for(int i = 0; i < bit; i ++)
		for(int S = 0; S < (1 << bit); S ++)
			if(!(S >> i & 1)) f[S] += f[S | (1 << i)];
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int S = 0; S < (1 << n); S ++) cin >> f[S];
	subset_sum(n);
	for(int S = 0; S < (1 << n); S ++) cout << f[S] << endl; //mask 从 0 到 2^n-1，每行一个
	return 0;
}
/*
Input:
3
1 2 3 4 5 6 7 8

Output:
1
3
4
10
6
14
16
36
*/
