#include <iostream>
#include "bubblesort.hpp"

int main()
{
    std::vector<int> arr = {64, 34, 25, 12, 22, 11, 90};
    std::cout << "Unsorted array: ";
    for (int num : arr)
    {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    bubbleSort(arr);
    std::cout << "Sorted array: ";
    for (int num : arr)
    {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    return 0;
}