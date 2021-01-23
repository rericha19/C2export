// heap and hash stuff, dont want it in the main heap thing

int heap_parent_index(int index) {
    return (index - 1) / 2;
}

// initialises heap
STATE_SEARCH_HEAP* heap_init_heap() {
    STATE_SEARCH_HEAP* temp = (STATE_SEARCH_HEAP*) malloc(sizeof(STATE_SEARCH_HEAP));                   // freed by caller
    temp->length = 0;
    temp->real_size = HEAP_SIZE_INCREMENT;
    temp->heap_array = (STATE_SEARCH_STR**) malloc(HEAP_SIZE_INCREMENT * sizeof(STATE_SEARCH_STR*));    // freed by caller

    return temp;
}


// frees all dynamically allocated memory of the heap's contents and the heap
void heap_destroy(STATE_SEARCH_HEAP* heap) {
    for (unsigned int i = 0; i < heap->length; i++)
        build_state_search_str_destroy(heap->heap_array[i]);
    free(heap->heap_array);
    free(heap);
}


// returns 1 if heap is empty, else 0
int heap_is_empty(STATE_SEARCH_HEAP heap) {
    if (heap.length == 0)
        return 1;
    return 0;
}

// figures out what swap has to be done in heap when removing, 0 if none, 1 if element<->childL, 2 if element<->childR
int heap_comp_util(STATE_SEARCH_HEAP* heap, int index_parent) {
    int index_child1 = 2 * index_parent + 1;
    int index_child2 = 2 * index_parent + 2;

    int temp = 0;
    int length = heap->length;

    if (index_child1 < length && index_child2 < length) {
        unsigned int value1 = heap->heap_array[index_child1]->estimated + heap->heap_array[index_child1]->elapsed;
        unsigned int value2 = heap->heap_array[index_child2]->estimated + heap->heap_array[index_child2]->elapsed;
        temp = (value1 < value2) ? 1 : 2;
    }

    if (index_child1 < length && index_child2 >= length)
        temp = 1;

    if (index_child1 >= length && index_child2 >= length)
        return 0;

    if (temp == 1) {
        unsigned int value1 = heap->heap_array[index_child1]->estimated + heap->heap_array[index_child1]->elapsed;
        unsigned int valueP = heap->heap_array[index_parent]->estimated + heap->heap_array[index_parent]->elapsed;
        if (value1 < valueP)
            return 1;
        else
            return 0;
    }

    if (temp == 2) {
        unsigned int value2 = heap->heap_array[index_child2]->estimated + heap->heap_array[index_child2]->elapsed;
        unsigned int valueP = heap->heap_array[index_parent]->estimated + heap->heap_array[index_parent]->elapsed;
        if (value2 < valueP)
            return 2;
        else
            return 0;
    }

    return 0;
}

/*
// pops from the heap, does necessary swaps to maintain the heap
STATE_SEARCH_STR* heap_pop(STATE_SEARCH_HEAP* heap) {

    if (heap->length == 0) {
        heap->heap_array = NULL;
        return NULL;
    }

    (heap->length)--;
    STATE_SEARCH_STR* result = heap->heap_array[0];

    if (heap->length != 0) {
        heap->heap_array[0] = heap->heap_array[heap->length];
    }
    else {
        return result;
    }

    for (int n = 0, temp = heap_comp_util(heap, n); temp != 0; temp = heap_comp_util(heap, n)) {
        STATE_SEARCH_STR* help = heap->heap_array[n];
        heap->heap_array[n] = heap->heap_array[2 * n + temp];
        heap->heap_array[2 * n + temp] = help;
        n = 2 * n + temp;
    }

    return result;
}*/

STATE_SEARCH_STR* heap_pop(STATE_SEARCH_HEAP *heap) {
    heap->length -= 1;
    STATE_SEARCH_STR* result = heap->heap_array[0];

    heap->heap_array[0] = heap->heap_array[heap->length];
    return result;
}


// adds state to the heap, makes necessary swap to mainatain the heap
void heap_add(STATE_SEARCH_HEAP* heap, STATE_SEARCH_STR* input) {
    int index_k = heap->length;

    heap->length += 1;
    if (heap->length >= heap->real_size) {
        heap->real_size += HEAP_SIZE_INCREMENT;
        heap->heap_array = (STATE_SEARCH_STR**) realloc(heap->heap_array, heap->real_size * sizeof(STATE_SEARCH_STR*));
        // when built with VS causes a crash, or at least it crashes right after this
    }

    heap->heap_array[index_k] = input;

    // heap thing sorting thing upkeep thing
    /*
    STATE_SEARCH_STR *temp;
    while(1)  {
        if (!index_k) break;

        int index_p = heap_parent_index(index_k);
        unsigned int value_k = heap->heap_array[index_k]->estimated + heap->heap_array[index_k]->elapsed;
        unsigned int value_p = heap->heap_array[index_p]->estimated + heap->heap_array[index_p]->elapsed;

        if (value_k >= value_p)
            break;

        temp = heap->heap_array[index_k];
        heap->heap_array[index_k] = heap->heap_array[index_p];
        heap->heap_array[index_p] = temp;

        index_k = index_p;
    }*/
}



/*
    hash implementation section
*/


// initialises the table
HASH_TABLE *hash_init_table(int (*hash_function)(HASH_TABLE*, unsigned short int *), int key_length) {
    HASH_TABLE *table = (HASH_TABLE *) malloc(sizeof(HASH_TABLE));              // freed by caller
    table->table_length = HASH_TABLE_SIZE;
    table->items = (HASH_ITEM**) calloc(HASH_TABLE_SIZE, sizeof(HASH_ITEM*));   // freed by caller
    table->key_length = key_length;
    table->item_count = 0;
    table->hash_function = hash_function;

    return table;
}


void hash_destroy_table(HASH_TABLE *table) {
    HASH_ITEM* curr, *next;
    for (int i = 0; i < table->table_length; i++) {
        for (curr = table->items[i]; curr != NULL; curr = next) {
            next = curr->next;
            free(curr);
        }
    }

    free(table->items);
    free(table);
}


int hash_compare_keys(unsigned short int* array1, unsigned short int* array2, int arrays_length) {
    for (int i = 0; i < arrays_length; i++)
        if (array1[i] != array2[i])
            return 0;
    return 1;
}


int hash_func_chek(HASH_TABLE* table, unsigned short int* entry_chunk_array) {
    long long int hash_temp = 0;
    int key_length = table->key_length;
    for (int i = 0; i < key_length; i++) {
        hash_temp *= i + 1;
        hash_temp += entry_chunk_array[i];
        hash_temp = (hash_temp & 0xFFFFFFFF) | (hash_temp >> 32);
    }

    hash_temp %= table->table_length;
    if (hash_temp < 0)
        hash_temp += table->table_length;

    return (int) hash_temp;
}



int hash_func(HASH_TABLE* table, unsigned short int* entry_chunk_array) {

    int key_length = table->key_length;

    int hash_temp = 0;
    for (int i = 0; i < key_length; i++) {
        hash_temp ^= ((int) entry_chunk_array[i] << (4 * i % 27));
    }

    hash_temp %= table->table_length;
    if (hash_temp < 0)
        hash_temp += table->table_length;

    return hash_temp;
}


HASH_ITEM* hash_find(HASH_TABLE* table, unsigned short int* entry_chunk_array)
// search function, returns valid pointer to the item or NULL if it does not exist
{
	HASH_ITEM* curr;
	int hash_value = table->hash_function(table, entry_chunk_array);

	if (table->items != NULL)
        for (curr = (table->items)[hash_value]; curr != NULL; curr = curr->next)
            if (hash_compare_keys(curr->entry_chunk_array, entry_chunk_array, table->key_length))
                return curr;

	return NULL;
}


// initialises hash item
HASH_ITEM* hash_make_item(unsigned short int* entry_chunk_array) {
	HASH_ITEM* item = NULL;
	item = (HASH_ITEM*) malloc(sizeof(HASH_ITEM));              // freed when table is destroyed
	item->entry_chunk_array = entry_chunk_array;
	item->next = NULL;

	return item;
}


// attempts to add an item into the specified list of the table, if such an item already exists returns 0, else returns 1 and adds
int hash_try_to_add_item(HASH_TABLE* table, HASH_ITEM* item,int list_index) {
	HASH_ITEM *curr, *last = NULL;

	for (curr = (table->items)[list_index];
         curr != NULL && !hash_compare_keys(curr->entry_chunk_array, item->entry_chunk_array, table->key_length);
         curr = curr->next)
		last = curr;

    if (curr != NULL) return 0;

	item->next = NULL;
	if (last == NULL)
        (table->items)[list_index] = item;
	else {
        last->next = item;
        //printf("hash table collision sucker\n");
	}

    return 1;
}


// puts the item in the specified list (used in resize)
void hash_place_item(HASH_TABLE *table, HASH_ITEM* item, int list_index)
{
    item->next = (table->items)[list_index];
    (table->items)[list_index] = item;
}


// attempts to add an item with this key, 'fails' if such an item already exists
int hash_add(HASH_TABLE *table, unsigned short int* entry_chunk_array) {
	int hash_val = table->hash_function(table, entry_chunk_array);

	// if adding it was not successful it gets rid of the item and returns, else goes on
    HASH_ITEM *curr = hash_make_item(entry_chunk_array);
    if (!hash_try_to_add_item(table, curr, hash_val)) {
        free(curr);
        return -1;
    }

    /*// resize thing, i think it causes trouble, i just made the table static size
    table->item_count += 1;
    if (((double) table->item_count / table->table_length) >= HASH_FULLNESS_RATIO) {
        HASH_ITEM** things = (HASH_ITEM**) malloc((table->item_count + 1) * sizeof(HASH_ITEM *));       // unused
        int index = 0;
        if (things != NULL)
        {
            for (int i = 0; i < table->table_length; i++)
                for (curr = (table->items)[i]; curr != NULL; curr = curr->next)
                    things[index++] = curr;
            free(table->items);
            table->items = (HASH_ITEM**) calloc(HASH_RESIZE_MULT * table->table_length, sizeof(HASH_ITEM*)); // unused
            table->table_length = table->table_length * HASH_RESIZE_MULT;
            for (int i = 0; i < index; i++)
            {
                things[i]->next = NULL;
                hash_place_item(table, things[i], table->hash_function(things[i]->entry_chunk_array, table->key_length, table->table_length));
            }
        }
        free(things);
    }
    //*/
	return hash_val;
}
