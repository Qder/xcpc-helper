#include<bits/stdc++.h>
#define maxn 100050
#define endl '\n'
#define ll long long
using namespace std;

// 严格上升：lower_bound 找第一个 >= a[i] 的位置替换过去；要求非降就把 lower_bound 换成 upper_bound
// tail[k] = 所有长度为 k 的上升子序列里最小的结尾值，tail 单调不减所以能二分
int n;
int a[maxn], tail[maxn];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	
	int len = 0;
	for(int i = 1; i <= n; i ++)
	{
		int pos = lower_bound(tail + 1, tail + len + 1, a[i]) - tail; //1-based 的第一个 >= a[i] 的位置，找不到就是 len + 1
		if(pos > len) tail[++ len] = a[i];
		else tail[pos] = a[i];
	}
	cout << len; //答案就是 tail 的长度
	return 0;
}
/*
Input:
8
3 1 4 1 5 9 2 6

Output:
4   （例如 1 4 5 6）
*/
