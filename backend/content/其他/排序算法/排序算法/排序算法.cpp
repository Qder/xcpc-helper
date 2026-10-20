#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
#define ll long long
using namespace std;

int n;
int a[maxn], tmp[maxn], b[maxn];

// 用法：merge_sort(1, n) 原地排好 a[1..n] 并返回逆序对个数；相等时先取左边，保证稳定、不重不漏
ll merge_sort(int l, int r)
{
	if(l >= r) return 0;
	int mid = (l + r) >> 1;
	ll res = merge_sort(l, mid) + merge_sort(mid + 1, r);
	int i = l, j = mid + 1, k = l;
	while(i <= mid && j <= r)
	{
		if(a[i] <= a[j]) tmp[k ++] = a[i ++];
		else
		{
			tmp[k ++] = a[j ++];
			res += mid - i + 1; //a[i..mid] 这 mid-i+1 个数都大于 a[j]，一次记满，不用逐个比
		}
	}
	while(i <= mid) tmp[k ++] = a[i ++];
	while(j <= r) tmp[k ++] = a[j ++];
	for(int p = l; p <= r; p ++) a[p] = tmp[p];
	return res;
}

int median3(int x, int y, int z) //三个数排一下取中间值
{
	if(x > y) swap(x, y);
	if(y > z) swap(y, z);
	if(x > y) swap(x, y);
	return y;
}

// 用法：quick_sort(1, n) 原地排 a[1..n]；三数取中是为了躲开「已有序」这种最坏数据
void quick_sort(int l, int r)
{
	if(l >= r) return;
	int mid = (l + r) >> 1;
	int pivot = median3(a[l], a[mid], a[r]);
	int i = l - 1, j = r + 1;
	while(true) //Hoare 划分：i 找 >= pivot 的，j 找 <= pivot 的，撞上就结束
	{
		do i ++; while(a[i] < pivot);
		do j --; while(a[j] > pivot);
		if(i >= j) break;
		swap(a[i], a[j]);
	}
	quick_sort(l, j); //pivot 取的是区间内的值，j 一定落在 [l, r-1]，不会无限递归
	quick_sort(j + 1, r);
}

void sift_down(int i, int len) //大根堆下沉，1-based：父亲 i 的儿子是 2i、2i+1
{
	while(true)
	{
		int big = i, L = i * 2, R = i * 2 + 1;
		if(L <= len && b[L] > b[big]) big = L;
		if(R <= len && b[R] > b[big]) big = R;
		if(big == i) break;
		swap(b[i], b[big]);
		i = big;
	}
}

// 用法：heap_sort_demo() 把 a[1..n] 拷到 b 再堆排，演示 O(1) 额外空间的原地排序（结果留在 b 里）
void heap_sort_demo()
{
	for(int i = 1; i <= n; i ++) b[i] = a[i];
	for(int i = n / 2; i >= 1; i --) sift_down(i, n); //从最后一个非叶子结点往前下沉，建堆是 O(n)
	for(int i = n; i > 1; i --)
	{
		swap(b[1], b[i]); //堆顶最大值换到末尾，之后只管前 i-1 个
		sift_down(1, i - 1);
	}
}

// 比赛里直接 std::sort 就行：内省排序 = 快排 + 堆排兜底 + 小区间插入，最坏也是 O(n log n)
int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	ll inv = merge_sort(1, n);
	for(int i = 1; i <= n; i ++)
	{
		if(i > 1) cout << ' ';
		cout << a[i];
	}
	cout << endl; //n = 0 时这里正好输出一个空行
	cout << inv << endl;
	return 0;
}
/*
Input:
5
4 2 5 1 3

Output:
1 2 3 4 5
6
*/
