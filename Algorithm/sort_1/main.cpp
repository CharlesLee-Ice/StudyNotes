//
//  main.cpp
//  SimpleTest
//
//

#include <iostream>

using namespace std;


void swap(int array[], int left, int right)
{
    int temp = array[left];
    array[left] = array[right];
    array[right] = temp;
}

/***
 * 冒泡排序
 * 时间复杂度 O(n^2)
 */
void bubbleSort(int array[], int length)
{
    for (int i = 0; i < length; i++) {
        for (int j = i + 1; j < length; j++) {
            if (array[i] > array[j]) {
                swap(array, i, j);
            }
        }
    }
}

/***
 * 选择排序
 * 时间复杂度 O(n^2)
 */
void selectSort(int array[], int length)
{
    for (int i = 0; i < length; i++) {
        int min = array[i];
        int minIndex = i;
        for (int j = i + 1; j < length; j++) {
            if (array[j] < min) {
                min = j;
                minIndex = j;
            }
        }
        
        if (minIndex != i) {
            swap(array, i, minIndex);
        }
    }
}

/***
 * 快速排序
 * 时间复杂度最坏情况：一个序列5,4,3,2,1，要排为1,2,3,4,5，每次二分都分成 一个数 和 剩余数（复杂度O(n)），达不到平均二分的效果O(log(n))
 * 每个split函数内部复杂度O(n),所以最坏情况复杂度O(n^2), 最好情况 O(nlog(n))
 */
int split(int array[], int left, int right)
{
    int temp = array[left];
    
    while (left < right) {
        while (left < right && array[right] >= temp) {
            right--;
        }
        if (left < right) {
            array[left] = array[right];
        }
        while (left < right && array[left] <= temp) {
            left++;
        }
        if (left < right) {
            array[right] = array[left];
        }
    }
    
    array[right] = temp;// 这里left 和 right 已经没区别了，上述while循环 出来 left == right
    
    return right;
}

void quickSort(int array[], int left, int right)
{
    if (left < right) {
        int mid = split(array, left, right);
        quickSort(array, left, mid - 1);
        quickSort(array, mid + 1, right);
    }
}

/***
 * 插入排序
 * 时间复杂度：O(n^2)
 */
void insertSort(int array[], int length)
{
    for (int i = 1; i < length; i++) {
        int insert = array[i];
        int j = i;
        while (j > 0 && insert < array[j - 1]) {
            array[j] = array[j - 1];
            j--;
        }
        array[j] = insert;
    }
}

/***
 * 希尔排序
 * 最坏的时间复杂度 O(n^2)
 */
void shellSort(int array[], int length)
{
    for (int gap = length/2; gap > 0; gap = gap/2)
    {
        for (int i = gap; i < length; i++) {
            int j = i;
            int temp = array[i];
            while ((j - gap) >= 0 && array[j - gap] > temp) {
                array[j] = array[j - gap];
                j = j - gap;
            }
            array[j] = temp;
        }
    }
}

/***
 * 堆排序
 * 平均复杂度为 O(nlog(n))
 * adjustHeap函数作用：保证该非叶子节点的数比左右孩子数都大，时间复杂度 O(log(n)), heapSort函数复杂度 O(n),所以总共复杂度 O(nlog(n))
 * 一般升序采用大顶堆，降序采用小顶堆
 */
void adjustHeap(int array[], int i, int length)
{
    int temp = array[i];
    
    int k = 2*i + 1;
    for (; k < length; k = 2*k + 1) {
        if ((k + 1) < length && array[k] < array[k+1]) {
            k++;
        }
        
        if (array[k] > temp) {
            array[i] = array[k];
            i = k;
        } else {
            break;
        }
    }
    
    array[i] = temp;
}

void heapSort(int array[], int length)
{
    for (int i = length/2 - 1; i >= 0; i--) {
        adjustHeap(array, i, length);
    }
    
    for (int j = length - 1; j > 0; j--) {
        swap(array, 0, j);
        
        adjustHeap(array, 0, j);
    }
}

int main(int argc, const char * argv[]) {
    
    int a[5]={5, 6, 3, 7, 8};
    
//    insertSort(a, 12);
//    heapSort(a, 5);
//    shellSort(a, 5);
//    selectSort(a, 5);
    bubbleSort(a, 5);
    
    for(int i = 0; i < 5; i++)
    {
        printf("%d ",a[i]);
    }
    
    printf("main %d\n", 1);
    
    return 0;
}
