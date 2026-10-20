#include<bits/stdc++.h>
#define maxn 1050
#define endl '\n'
#define ll long long
using namespace std;

// dp[i] = 以 i 结尾的最大子段和：要么自己单开一段，要么接在 i - 1 后面
// 答案初值不能给 0（全负时最大子段和是负数），用 -1e18
int n;
int a[maxn];
ll dp[maxn];

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	
	ll ans = -1e18;
	for(int i = 1; i <= n; i ++)
	{
		dp[i] = max((ll)a[i], dp[i - 1] + a[i]);
		ans = max(ans, dp[i]);
	}
	cout << ans;
	return 0;
}

//-- 分治 O(n log n)（跨中点那两段的分界下标就是起止位置，要输出区间用它）--

// 用法：读入 a[1..n] 之后直接 cout << max_sub_dc(1, n);
ll max_sub_dc(int l, int r)
{
	if(l == r) return a[l];
	int mid = (l + r) >> 1;
	ll ans = max(max_sub_dc(l, mid), max_sub_dc(mid + 1, r));
	ll sum = 0, bestl = -1e18; //从中点往左的最大后缀和
	for(int i = mid; i >= l; i --) sum += a[i], bestl = max(bestl, sum);
	sum = 0;
	ll bestr = -1e18; //从中点右边第一格往右的最大前缀和
	for(int i = mid + 1; i <= r; i ++) sum += a[i], bestr = max(bestr, sum);
	return max(ans, bestl + bestr); //跨中点的最优
}
/*
Input:
7
2 -1 3 -5 4 3 -2

Output:
7   （4 + 3）
*/
