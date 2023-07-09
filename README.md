# Parallel Quick Sort

This is a header only implementation of quicksort in parallel (.cpp is for testing). 

Sorting in parallel can be tricky because there are a lot of different moving parts. I had trouble wrapping my head around how it works. You can find a (relatively) simple explanation below. 

My implementation can be boiled down to the following:

- First, we check if the input size is small. If it is, we can just use `std::sort` because C++ sorting is pretty fast already. 


- If it isn't we will start by "partitioning" the array. 

- Start by choosing a pivot index, preferably randomly. 

- Start filtering the array by finding elements that are less than / greater than the pivot. We store these values in a boolean array. Note that we don't need to filter elements that are equal to the pivot, since we can determine the number of "equal to" elements using the offset from the previous filter.

- Scan the prefix sums (inclusive or exclusively) which will give us information on where we can "pack" our elements in our sorted array. 

- Pack the elements in B by checking the filters from earlier. Additionally, we can pack elements that are equal to the pivot using the two offsets that we retrieved earlier.

- Finally, we return the `std::pair<leftOffset, rightOffset>` which will later be used as pointers to split the array. This ends the partition portion.

- We just need to partition recursively (in parallel) until we hit the base case, where we can just use `std::sort`.

Here is what it boils down to once the partition function has been made:

<img width="867" alt="image" src="https://github.com/kkadhith/parallelquicksort/assets/24882134/9561d9f5-59a5-462b-99ff-1a144a480697">
