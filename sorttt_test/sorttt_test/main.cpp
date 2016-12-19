


#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#define MAX_NR 1000000


void fillRandom(std::vector<int>& field, int size, int max)
{
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(1,max);
	for (int i = 0;i < size;i++)
	{
		field.push_back(distribution(generator));

	}


}


void Merge(std::vector<int>& field, int l, int m, int r, int max)
{
	int n1 = m - l + 1;
	int n2 = r - m;
	int* L = new int[n1+1];
	int* R = new int[n2+1];
	memcpy(L, field.data()+l, (n1)*sizeof(int));
	memcpy(R, field.data() + m, (n2)*sizeof(int));
	L[n1] = max;
	R[n2] = max;
	int i = 0;
	int j = 0;

	for (int k = l;k < r;k++)
	{
		if (L[i] <= R[j])
		{
			field[k] = L[i];
			i++;
		}
		else
		{
			field[k] = R[j];
			j++;
		}
	}

	delete[] L;
	delete[] R;

}

void mergeSort(std::vector<int>& field, int l, int r, int max)
{
	if (l < r)
	{
		int m = (l + r) / 2;
		mergeSort(field, l, m, max);
		mergeSort(field, m + 1, r, max);
		Merge(field, l, m, r, max);

	}
}













int main()
{
	
	std::vector<int> field;
	for (int i = 1;i < 50;i++)
	{
		
		int size = pow(2, i);
		fillRandom(field, size, MAX_NR-1);
		std::cout << "Starting Sorting of Array Size " << size << std::endl;
		auto start_time = std::chrono::high_resolution_clock::now();
		mergeSort(field, 0, size, MAX_NR);
		auto end_time = std::chrono::high_resolution_clock::now();
		std::cout << "Sorting Finished in " << std::chrono::duration_cast<std::chrono::seconds> (end_time - start_time).count() << ":" << std::chrono::duration_cast<std::chrono::milliseconds> (end_time - start_time).count() << std::endl;
		std::cout << "Current size should be " << field.size()*sizeof(int) << std::endl;
		
		std::cout << "Press Enter" << std::endl;
		std::cin.get();

		field.clear();
	}



	return 0;
}



