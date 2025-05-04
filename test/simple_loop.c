#include <stdio.h>
#include <time.h>

int main() {
    const long long N = 10000000000LL; // Adjust as needed (e.g., 1e9)
    long long sum = 0;

    // Start measuring time
    clock_t start = clock();

    for (long long i = 0; i <= N; i++) {
        sum += 1;
    }

    // Stop measuring time
    clock_t end = clock();

    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Sum = %lld\n", sum);
    printf("Time taken = %.4f seconds\n", time_taken);

    return 0;
}
