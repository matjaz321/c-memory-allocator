#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

typedef char ALIGN[16];

/*
 * We need to store size and whether block is free for every block of allocated memory
 * So we need to have memory blocks stored contiguously so we need to store them in the linked list.
 * We need to use union with stub so the memory will be aligned to 16 bytes.
 */
union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    /* force the header to be aligned to 16 bytes */
    ALIGN stub;
};
typedef union header header_t;


typedef union header header_t;
header_t *head = NULL, *tail = NULL; // to keep track of the list
pthread_mutex_t  global_malloc_lock;


// Traverse the linked list and see if there already exists block of memory that is marked as free and can allocate the given size.
header_t *get_free_block(size_t size)
{
    header_t *curr = head;
    while (curr) {
        if (curr->s.is_free && curr->s.size >= size) {
            return curr;
        }
        curr = (header_t *) curr->s.next;
    }

    return NULL;
}

void *malloc(size_t size)
{
    size_t total_size;
    void *block;
    header_t *header;
    // if requested size is 0 then just return;
    if (!size) {
        return NULL;
    }
    // Acquire the lock
    pthread_mutex_lock(&global_malloc_lock);
    // get free free memory block
    header = get_free_block(size);
    if (header) {
        // Mark block as non free.
        header->s.is_free = 0;
        // Release the lock.
        pthread_mutex_unlock(&global_malloc_lock);
        // Return pointer to that block.
        return (void*)(header + 1); // We increase by one byte because we don't want header to be seen by outside.
    }

    // When we dont find that block, extend the heap that fits the requested size as well as header.
    total_size = sizeof(header_t) + size; // Remember total size is header + the requested size
    block = sbrk(total_size);
    if (block == (void*) -1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    // Fill in the header mark it as non free.
    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;

    // Update the next pointer, so head and tail reflect the new state of the linked list.
    if (!head) {
        head = header;
    }

    if (tail) {
        tail->s.next = (union header *) (struct header_t *) header;
    }
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock); // Release the lock
    return (void *)(header +1); // We increase by one byte because we don't want header to be seen by outside.
}

int main() {
     void *pointer = malloc(sizeof("test"));
    printf("%p", &pointer);
}