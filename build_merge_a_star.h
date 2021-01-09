// heap stuff, dont want it in the main heap thing

int heap_parent_index(int index) {
    return (index - 1) / 2;
}


A_STAR_HEAP* heap_init_heap() {
    A_STAR_HEAP* temp = (A_STAR_HEAP*) malloc(sizeof(A_STAR_HEAP));
    temp->length = 0;
    temp->heap_array = NULL;

    return temp;
}

void heap_destroy(A_STAR_HEAP* heap) {
    for (unsigned int i = 0; i < heap->length; i++)
        build_a_star_str_destroy(heap->heap_array[i]);
    free(heap);
}


int heap_is_empty(A_STAR_HEAP heap) {
    if (heap.length == 0)
        return 1;
    return 0;
}

// figures out what swap has to be done in heap when removing, 0 if none, 1 if element<->childL, 2 if element<->childR
int heap_comp_util(A_STAR_HEAP* heap, int index_parent) {
    int index_child1 = 2 * index_parent + 1;
    int index_child2 = 2 * index_parent + 2;

    int temp = 0;
    int length = heap->length;

    if (index_child1 < length && index_child2 < length) {
        unsigned int value1 = heap->heap_array[index_child1]->elapsed + heap->heap_array[index_child1]->estimated;
        unsigned int value2 = heap->heap_array[index_child2]->elapsed + heap->heap_array[index_child2]->estimated;
        temp = (value1 < value2) ? 1 : 2;
    }

    if (index_child1 < length && index_child2 >= length)
        temp = 1;

    if (index_child1 >= length && index_child2 >= length)
        return 0;

    if (temp == 1) {
        unsigned int value1 = heap->heap_array[index_child1]->elapsed + heap->heap_array[index_child1]->estimated;
        unsigned int valueP = heap->heap_array[index_parent]->elapsed + heap->heap_array[index_parent]->estimated;
        if (value1 < valueP)
            return 1;
        else
            return 0;
    }

    if (temp == 2) {
        unsigned int value2 = heap->heap_array[index_child2]->elapsed + heap->heap_array[index_child2]->estimated;
        unsigned int valueP = heap->heap_array[index_parent]->elapsed + heap->heap_array[index_parent]->estimated;
        if (value2 < valueP)
            return 2;
        else
            return 0;
    }

    return 0;
}


// pops from the heap, does necessary swaps to maintain the heap
A_STAR_STR* heap_pop(A_STAR_HEAP* heap) {

    if (heap->length == 0) {
        heap->heap_array = NULL;
        return NULL;
    }

    (heap->length)--;
    A_STAR_STR* result = heap->heap_array[0];

    if (heap->length != 0) {
        heap->heap_array[0] = heap->heap_array[heap->length];
        // TODO bad, make it realloc only every couple something
        heap->heap_array = (A_STAR_STR**) realloc(heap->heap_array, heap->length * sizeof(A_STAR_STR));
    }
    else {
        free(heap->heap_array);
        heap->heap_array = NULL;
        return result;
    }

    for (int n = 0, temp = heap_comp_util(heap, n); temp != 0; temp = heap_comp_util(heap, n)) {
        A_STAR_STR* help = heap->heap_array[n];
        heap->heap_array[n] = heap->heap_array[2 * n + temp];
        heap->heap_array[2 * n + temp] = help;
        n = 2 * n + temp;
    }

    return result;
}


// adds state to the heap, makes necessary swap to mainatain the heap
void heap_add(A_STAR_HEAP* heap, A_STAR_STR* input) {
    A_STAR_STR *temp;
    int index_k = heap->length;

    // TODO bad, realloc is expensive, make it only change the size every couple something idk
    heap->heap_array = (A_STAR_STR**) realloc(heap->heap_array, (index_k + 1) * sizeof(A_STAR_STR));
    heap->heap_array[heap->length] = input;
    heap->length += 1;

    while(1)  {
        if (!index_k) break;

        int index_p = heap_parent_index(index_k);
        unsigned int value_k = heap->heap_array[index_k]->elapsed + heap->heap_array[index_k]->estimated;
        unsigned int value_p = heap->heap_array[index_p]->elapsed + heap->heap_array[index_p]->estimated;

        if (value_k < value_p) {
            temp = heap->heap_array[index_p];
            heap->heap_array[index_k] = heap->heap_array[index_p];
            heap->heap_array[index_p] = temp;
            index_k = index_p;
        }
        else break;
    }
}
