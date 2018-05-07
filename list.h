struct List {
    int count;
    int capacity;
    int elem_size;
    void *arr;
};

struct List* list_create(int elem_size);
int list_add(struct List* list, void* elem);
void list_free(struct List* list);
void *list_item(struct List *list, int index);
