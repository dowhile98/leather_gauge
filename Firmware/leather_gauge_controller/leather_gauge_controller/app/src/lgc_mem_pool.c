/**
 * @file tcs_xxx_bsp_mem_pool.c
 * @brief Memory pool management implementation for BSP (ThreadX or generic RTOS).
 *
 * Provides thread-safe memory allocation, deallocation, and resizing routines for use with RTOS.
 *
 * @date Jul 9, 2025
 * @author jeffr
 */

#include "os_port.h"
#include "error.h"
#include "os_port.h"
#include "error.h"


/******************************************************************************/
/* Global Variables */
/******************************************************************************/
static OsMutex mutex; /**< Mutex for thread-safe memory operations */

#if defined(USE_THREADX)
static TX_BYTE_POOL *app_byte_pool; /**< Pointer to ThreadX byte pool */
#endif

static uint8_t tcs_bsp_mem_init = 0;

/******************************************************************************/
/**
 * @brief Initialize the memory pool and mutex for thread-safe allocation.
 * @param[in] pointer Pointer to RTOS memory pool (ThreadX only)
 * @return NO_ERROR on success, error code otherwise
 */
#if defined(USE_THREADX)
uint8_t osPoolInit(void *pointer)
{
#else
uint8_t osPoolInit(void)
{
#endif

	if(tcs_bsp_mem_init == 1)
	{
		return NO_ERROR;
	}
    if (osCreateMutex(&mutex) != TRUE) {
        return TX_MUTEX_ERROR;
    }
#if defined(USE_THREADX)
    app_byte_pool = (TX_BYTE_POOL *)pointer;
#endif
    tcs_bsp_mem_init = 1;
    return NO_ERROR;
}


/**
 * @brief Allocate a memory block from the pool (thread-safe).
 * @param[in] size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void *osAllocMem(size_t size)
{
    void *p;
    /* Enter critical section */
    osAcquireMutex(&mutex);
    /* Allocate a memory block */
#if defined(USE_THREADX)
    if (tx_byte_allocate(app_byte_pool, (void **)&p, size, TX_NO_WAIT) != TX_SUCCESS) {
        p = NULL;
    }
#else
    /* Add other RTOS or generic allocation here if needed */
#endif
    /* Leave critical section */
    osReleaseMutex(&mutex);
    /* Return a pointer to the newly allocated memory block */
    return p;
}


/**
 * @brief Allocate and zero-initialize a memory block.
 * @param[in] num Number of elements to allocate
 * @param[in] size Size of each element in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void *osCallocMem(size_t num, size_t size)
{
    size_t total;
    void *ptr = NULL;
    /* Check for safe multiplication to avoid overflow */
    if (size != 0 && num > (SIZE_MAX / size)) {
        return NULL;
    }
    total = num * size;
    ptr = osAllocMem(total);
    /* If allocation succeeded, zero-initialize the memory */
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}



/**
 * @brief Resize a previously allocated memory block.
 * @param[in] ptr Pointer to the current memory block
 * @param[in] new_size New size for the memory block
 * @return Pointer to resized memory block, or NULL on failure
 *
 * If ptr is NULL, behaves like osAllocMem. If new_size is 0, frees the memory.
 * Note: Only safe if new_size <= old_size; otherwise, may cause buffer overrun.
 */
void *osReallocMem(void *ptr, size_t new_size)
{
    void *new_ptr;
    /* Case 1: ptr is NULL, behaves like malloc */
    if (ptr == NULL) {
        return osAllocMem(new_size);
    }
    /* Case 2: new_size is 0, behaves like free */
    if (new_size == 0) {
        osFreeMem(ptr);
        return NULL;
    }
    /* Case 3: resize */
    new_ptr = osAllocMem(new_size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, new_size); // WARNING: Only safe if new_size <= old_size
        osFreeMem(ptr);
    }
    return new_ptr;
}

/**
 * @brief Release a previously allocated memory block.
 * @param[in] p Previously allocated memory block to be freed
 */
void osFreeMem(void *p)
{
    /* Make sure the pointer is valid */
    if (p != NULL)
    {
        /* Enter critical section */
        osAcquireMutex(&mutex);
        /* Free memory block */
#if defined(USE_THREADX)
        if (tx_byte_release(p) == TX_SUCCESS) {
            p = NULL;
        }
#else
        /* Add other RTOS or generic free here if needed */
#endif
        /* Leave critical section */
        osReleaseMutex(&mutex);
    }
}
