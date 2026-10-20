#include<bits/stdc++.h>
#define maxn 1050
#define maxm 1000005
#define endl '\n'
using namespace std;

int n, S, sum;
int a[maxn];
bitset<maxm> bs; //bs[j] = 1 表示能凑出和 j；位宽按 sum 上限 1e6 开，多留一点防越界

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) { cin >> a[i]; sum += a[i]; }
	cin >> S;
	bs[0] = 1; //一个都不选，和 0 一定可达
	// 左移 a[i] 位相当于把每个可达和都加上 a[i]，或回自身则是保留「不选这个数」的情况，
	// 合起来正好是 01 背包的存在性转移；一次位运算顶 64 次循环，总复杂度 O(n * sum / 64)
	for(int i = 1; i <= n; i ++) bs |= bs << a[i];
	cout << ((S <= sum && bs[S]) ? "Yes" : "No") << endl; //S 超过总和时肯定凑不出，显式判掉更清楚
	// 统计范围就是 [0, sum]：所有 a[i] 都是正数，任何转移都打不出 sum 以上的位，
	// 所以 bs.count() 数的就是 0..sum 里所有不同和（含 0），不需要再减高位
	cout << bs.count() << endl;
	return 0;
}
/*
Input:
4
1 2 4 8
10

Output:
Yes
16
*/
