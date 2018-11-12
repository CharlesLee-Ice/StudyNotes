## TimSort ##

`TimeSort`是一款实际应用比较好的排序算法，结合了二分插入排序和归并排序，广泛应用于python,jdk的源码中，该排序在最优情况下复杂度 O(n), 一般情况和最差情况复杂度为 O(nlog(n))

#### 当数组个数小于32时，采取二分插入排序

	void binaryInsertSort(int array[], int length)
	{
	    for (int i = 1; i < length; i++) {
	        int tmp = array[i];
	        int left = 0;
	        int right = i - 1;
	        while (left < right) {
	            int mid = (left + right) >> 1;
	            if (array[mid] > tmp) {
	                right = mid;
	            } else {
	                left = mid + 1;
	            }
	        }
	    
	        int n = i - left; // 需要移动的个数
	        if (n > 0) {
	            memmove(array + left + 1, array + left, n * sizeof(array[0]));
	            array[left] = tmp;
	        }
	    }
	}

#### 数组个数大于32时，采取归并排序，具体归并步骤：

* 从数组中取不小于32个数，保证取出来的这些数按从小到大已经排序好了，
	1. 依次遍历数组，如果一直增加，就一直取，
	2. 遍历时，如果发现有连续减小的数，则颠倒之，
	3. 连续递增的数小于32，则取满32个数，进行二分插入排序
* 将这一组Run（index和len）压栈
* 处理栈中Run（原则：保证栈下面的长度比栈上面长度大，因为首先归并栈上面的两个Run，越归并到最后，Run的长度肯定越大，所以尽量保证栈底Run的长度最大）
	
	1. 如果栈中Run个数为2，且`Run[1].len >= Run[0].len`则这两个Run归并成一个Run 
	2. 如果栈中Run个数大于2, 且`Run[n-1].len <= Run[n].len + Run[n+1].len`
	
		    private void mergeCollapse() {
		        while (stackSize > 1) {
		            int n = stackSize - 2;
		            if (n > 0 && runLen[n-1] <= runLen[n] + runLen[n+1]) {
		                if (runLen[n - 1] < runLen[n + 1])
		                    n--;
		                mergeAt(n);
		            } else if (runLen[n] <= runLen[n + 1]) {
		                mergeAt(n);
		            } else {
		                break; // Invariant is established
		            }
		        }
		    }
 * 继续取数组不小于32个数压栈，重复上述操作，直到数组取完后，进行最后一次栈内Run归并排序，直到栈内只剩下最后一个Run结束。

## 参考 ##
[Timsort](https://hackernoon.com/timsort-the-fastest-sorting-algorithm-youve-never-heard-of-36b28417f399)
