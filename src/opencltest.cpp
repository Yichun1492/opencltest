#include <iostream>
#include <string>
#include <CL/cl.h>
#include <algorithm>
#include <array>
void init_cl()
{
    cl_uint plats = 0;
    std::cout << "Begin get OpenCL platform." << std::endl;
    clGetPlatformIDs(0, NULL, &plats);
    std::cout << "OpenCL platforms: " << plats << std::endl;
}
 
template<class ForwardIterator>
inline size_t argmin(ForwardIterator first, ForwardIterator last)
{
    return std::distance(first, std::min_element(first, last));
}

template<class ForwardIterator>
inline size_t argmax(ForwardIterator first, ForwardIterator last)
{
    return std::distance(first, std::max_element(first, last));
}

int main(int argc, char **argv)
{
    std::array<int, 7> numbers{2, 4, 8, 0, 6, -1, 3};
    size_t minIndex =  argmin(numbers.begin(), numbers.end());
    std::cout << minIndex << '\n';
    std::vector<float> prices = {12.5, 8.9, 100, 24.5, 30.0};
    size_t maxIndex = argmax(prices.begin(), prices.end());
    std::cout << maxIndex << '\n';
    std::string str = "cl";
    init_cl();
    return 0;
}