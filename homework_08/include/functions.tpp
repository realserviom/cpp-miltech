template <typename T>
int countFilledElements(const T arr[], int size, T marker) {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (arr[i] != marker) count++;
    }
    return count;
}