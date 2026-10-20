#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
using namespace std;

int n, q, m;
int a[maxn], b[maxn]; //a[1..n] 是原数组，b[0..m-1] 是去重排序后的值域（b 用 0-based，名次好算）

// 用法：rank_of(x) 返回 x 在去重排序后的名次（1-based）；x 没在原数组里出现过就返回 0
int rank_of(int x)
{
	int *it = lower_bound(b, b + m, x); //b 已经 sort + unique，二分到的位置就是 x 该占的名次
	if(it == b + m || *it != x) return 0; //lower_bound 只保证「第一个 >= x」，不保证等于 x，必须再比一次值
	return it - b + 1; //是 0-based 下标，转成 1-based 名次要 +1；相等的元素都落在这个位置，编号自然相同
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) { cin >> a[i]; b[i - 1] = a[i]; }
	sort(b, b + n);
	m = unique(b, b + n) - b; //unique 只把重复元素挪到后面并返回第一个废弃位置，减一下才是真正的值域大小 m <= n
	cin >> q;
	while(q --)
	{
		int x; cin >> x;
		cout << rank_of(x) << endl;
	}
	return 0;
}
/*
Input:
5
100 10 100 -5 50
3
10
200
-5

Output:
2
0
1
*/
