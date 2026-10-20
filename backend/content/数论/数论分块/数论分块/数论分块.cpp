#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define endl '\n'
#define ll long long
using namespace std;

int T;
ll n, sum1, sum2; //sum1 = Σ⌊n/i⌋，sum2 = Σ i·⌊n/i⌋

// 用法：calc(n) 之后读 sum1 / sum2，两个和式共用一套区间，只分块一次
// 为什么 r = n/(n/l)：令 q = ⌊n/l⌋，则 ⌊n/i⌋ = q 等价于 i <= n/q，最大的 i 就是 ⌊n/q⌋
void calc(ll n)
{
	sum1 = sum2 = 0;
	for(ll l = 1, r; l <= n; l = r + 1)
	{
		ll q = n / l;
		r = n / q; //下一块从 r + 1 开始
		sum1 += (r - l + 1) * q;
		sum2 += (l + r) * (r - l + 1) / 2 * q; //块内 Σi 用等差数列，(l+r)*(r-l+1) 必为偶数，先 /2 再乘 q 防爆 ll
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> T;
	while(T --)
	{
		cin >> n;
		calc(n);
		cout << sum1 << ' ' << sum2 << endl;
	}
	return 0;
}
/*
Input:
4
10
1
1000000000
999983

Output:
27 87
1 1
20877697634 822467034112360628
13969705 822438735728
*/
