#include <iostream>
#include <vector>

void bubbleSort(std::vector<int> &arr)
{
    int n = arr.size();
    for (int i = 0; i < n - 1; ++i)
    {
        for (int j = 0; j < n - i - 1; ++j)
        {
            if (arr[j] > arr[j + 1])
            {
                std::swap(arr[j], arr[j + 1]);
#ifdef PRINT_VECTOR
                std::cout << "Round " << i + 1 << ": ";
                for (int num : arr)
                    std::cout << num << " ";
                std::cout << std::endl;
#endif
            }
        }
    }
}