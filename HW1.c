#include <stdio.h>
#include <stdlib.h>

// Assignment 3: Pointers and Functions
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Assignment 6: String Manipulation with Pointers
int str_length(char *str) {
    int len = 0;
    while (*str++) len++;
    return len;
}

int main() {
    // Assignment 1: Basics of Pointers
    printf("Assignment 1: Basics of Pointers\n");
    int x = 10;
    int *ptr = &x;
    printf("Address of x: %p\n", &x);
    printf("Address via ptr: %p\n", ptr);
    *ptr = 20;
    printf("New value of x: %d\n\n", x);

    // Assignment 2: Pointer Arithmetic
    printf("Assignment 2: Pointer Arithmetic\n");
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = arr;
    printf("Original array: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", *(p + i));
    }
    printf("\n");
    // Modify using pointer arithmetic
    for (int i = 0; i < 5; i++) {
        *(p + i) += 10;
    }
    printf("Modified array via pointer: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", *(p + i));
    }
    printf("\n");
    printf("Modified array via array name: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n\n");

    // Assignment 3: Pointers and Functions
    printf("Assignment 3: Pointers and Functions\n");
    int a = 1, b = 2;
    printf("Before swap: a=%d, b=%d\n", a, b);
    swap(&a, &b);
    printf("After swap: a=%d, b=%d\n\n", a, b);

    // Assignment 4: Pointers to Pointers
    printf("Assignment 4: Pointers to Pointers\n");
    int y = 5;
    int *p_y = &y;
    int **pp_y = &p_y;
    printf("Value via pointer: %d\n", *p_y);
    printf("Value via double pointer: %d\n\n", **pp_y);

    // Assignment 5: Dynamic Memory Allocation with Pointers
    printf("Assignment 5: Dynamic Memory Allocation with Pointers\n");
    int *dyn = (int*)malloc(sizeof(int));
    if (dyn == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }
    *dyn = 42;
    printf("Dynamically allocated integer: %d\n", *dyn);
    int *arr_dyn = (int*)malloc(5 * sizeof(int));
    if (arr_dyn == NULL) {
        printf("Memory allocation failed\n");
        free(dyn);
        return 1;
    }
    for (int i = 0; i < 5; i++) {
        *(arr_dyn + i) = i * 10;
    }
    printf("Dynamically allocated array: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", *(arr_dyn + i));
    }
    printf("\n");
    free(dyn);
    free(arr_dyn);
    printf("Memory freed\n\n");

    // Assignment 6: String Manipulation with Pointers
    printf("Assignment 6: String Manipulation with Pointers\n");
    char *str = "Hello World";
    printf("String: ");
    for (char *p = str; *p != '\0'; p++) {
        printf("%c", *p);
    }
    printf("\n");
    printf("Length of string: %d\n\n", str_length(str));

    // Assignment 7: Array of Pointers
    printf("Assignment 7: Array of Pointers\n");
    char *strings[3] = {"Hello", "World", "C"};
    printf("Original strings:\n");
    for (int i = 0; i < 3; i++) {
        char *p = strings[i];
        while (*p) {
            printf("%c", *p++);
        }
        printf("\n");
    }
    // Modify one string
    strings[1] = "Universe";
    printf("Modified strings:\n");
    for (int i = 0; i < 3; i++) {
        char *p = strings[i];
        while (*p) {
            printf("%c", *p++);
        }
        printf("\n");
    }

    return 0;
}
