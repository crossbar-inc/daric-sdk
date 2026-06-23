/**
 ******************************************************************************
 * @file    threads_dependency.c
 * @author  OS Team
 * @brief   The Thread Execution Management Framework provides a robust solution 
 *          for managing thread creation and initialization dependencies 
 *          in ThreadX RTOS-based systems. 
 *          It ensures that threads are created and initialized in the correct order, 
 *          with proper handling of inter-thread dependencies.
******************************************************************************
* @attention
*
* © Copyright CrossBar, Inc. 2024.
*
* All rights reserved.
*
* This software is the proprietary property of CrossBar, Inc. and is protected
* by copyright laws. Any unauthorized reproduction, distribution, or
* modification is strictly prohibited.
*
******************************************************************************
*/

/* Includes --------------------------------------------------------------------------*/
#include <stdint.h>
#include "tx_api.h"
#include "user_threads_attrdef.h"
#include "tx_log.h"

#undef LOG_TAG
#define LOG_TAG "THRD_DEPCY"
extern void touch_thread_entry(ULONG thread_input);
extern void guiThreadEntry(ULONG thread_input);
extern void bluetooth_entry(ULONG arg);
// extern void nfc_thread_entry(ULONG arg);
extern void thread_usb_service_entry(ULONG thread_input);

#define DEPENDENCY_ITEMS(...) __VA_ARGS__
#define START_THREAD_ITEM(id, entry, parm, ...) \
    T_UserThreadEnum dep_##id[] = {__VA_ARGS__};
    #include "threads_dependency.h"
#undef START_THREAD_ITEM

#define START_THREAD_ITEM(id, entry, parm, ...) \
    { \
        .threadId       = id, \
        .dependencies   = dep_##id, \
        .depCount       = sizeof(dep_##id)/sizeof(T_UserThreadEnum), \
        .entryFunc      = entry, \
        .param          = parm, \
        .status         = THREAD_STATUS_NOT_CREATED \
    },
T_ThreadsDepType gStartThreadList[] = {
    #include "threads_dependency.h"
};
#undef START_THREAD_ITEM

TX_EVENT_FLAGS_GROUP threadInitEvent;

/**
 * @brief Thread wrapper function that handles initialization sequencing
 * @param parameter Pointer to thread configuration cast to ULONG
 *
 * This wrapper ensures proper thread initialization sequence and handles
 * dependency management before calling the actual thread function.
 */
static void entryFuncWrapper(ULONG parameter)
{
    T_ThreadsDepType *config = (T_ThreadsDepType *)parameter;

    /* Update thread status */
    config->status = THREAD_STATUS_INITIALIZED;

    /* Signal completion of initialization */
    if (config->threadId != USER_THREAD_GUI)
    {
        tx_event_flags_set(&threadInitEvent, 1UL, TX_OR);
    }

    /* Start actual thread function */
    if (config->entryFunc)
    {
        config->entryFunc(parameter);
    }
}

/**
 * @brief Find thread config data from threadId
 * @param threadId The thread ID to start checking from
 * @return uint32_t T_ThreadsDepType point if successful, NULL otherwise
 *
 * Find thread config data from threadId.
 * T_ThreadsDepType point if successful, NULL otherwise.
 */
static T_ThreadsDepType *findThreadConfig(T_UserThreadEnum threadId)
{
    uint32_t totalCnt = sizeof(gStartThreadList) / sizeof(T_ThreadsDepType);

    for (int i = 0; i < totalCnt; i++)
    {
        if (gStartThreadList[i].threadId == threadId)
        {
            return &gStartThreadList[i];
        }
    }

    return NULL;
}

/**
 * @brief Check if the thread IDs of the dependencies have all been defined.
 * @param None
 * @return uint32_t 1 if successful, 0 otherwise
 *
 * Check if the thread IDs of the dependencies have all been defined.
 */
static uint32_t checkDependThreadIdValid(void)
{
    T_ThreadsDepType *val = NULL;
    uint32_t totalCnt = sizeof(gStartThreadList) / sizeof(T_ThreadsDepType);

    for (int i = 0; i < totalCnt; i++)
    {
        if (gStartThreadList[i].depCount > 0)
        {
            for (int j = 0; j < gStartThreadList[i].depCount; j++)
            {
                val = findThreadConfig(gStartThreadList[i].dependencies[j]);
                if (val == NULL)
                {
                    LOGE_COLOR(COL_RED, "There are dependent thread IDs that are not defined.");
                    return 0;
                }
            }
        }
    }

    return 1;
}

/**
 * @brief Checks if all dependencies for a thread are initialized
 * @param config Pointer to thread configuration structure
 * @return uint32_t 1 if all dependencies are satisfied, 0 otherwise
 *
 * This function verifies that all threads that the specified thread depends on
 * have completed their initialization phase.
 */
static uint32_t checkDependencies(T_ThreadsDepType *config)
{
    T_ThreadsDepType *depCfg;

    if (!config->dependencies || config->depCount == 0)
    {
        return 1;
    }

    for (uint32_t i = 0; i < config->depCount; i++)
    {
        depCfg = findThreadConfig(config->dependencies[i]);
        if (depCfg == NULL)
        {
            return 0;
        }
        if (depCfg->status == THREAD_STATUS_NOT_CREATED)
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Creates and starts a thread based on its configuration
 * @param config Pointer to thread configuration structure
 * @return uint32_t TX_SUCCESS if successful, error code otherwise
 *
 * Handles the actual thread creation process after verifying that all
 * dependencies are satisfied.
 */
static uint32_t startThread(T_ThreadsDepType *config)
{

    if (config->status != THREAD_STATUS_NOT_CREATED)
    {
        return TX_SUCCESS;
    }

    /* Checks if all dependencies for a thread are initialized */
    if (!checkDependencies(config))
    {
        return TX_NOT_AVAILABLE;
    }

    uint32_t result = tx_thread_create(&config->thread,
                                       gUsrThreads_cfg_table[config->threadId].name,
                                       entryFuncWrapper,
                                       (ULONG)config,
                                       gUsrThreads_cfg_table[config->threadId].pStack,
                                       gUsrThreads_cfg_table[config->threadId].stackSize,
                                       gUsrThreads_cfg_table[config->threadId].priority,
                                       gUsrThreads_cfg_table[config->threadId].preemptThod,
                                       gUsrThreads_cfg_table[config->threadId].timeSlice,
                                       TX_AUTO_START);

    if (result == TX_SUCCESS)
    {
        config->status = THREAD_STATUS_CREATED;
        LOGV("Create thread (%s)", gUsrThreads_cfg_table[config->threadId].name);
    }

    uint32_t rstFlag = 0;
    if (tx_event_flags_get(&threadInitEvent, 1UL,
                           TX_AND_CLEAR, &rstFlag, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        return TX_NO_EVENTS;
    }

    return result;
}

/**
 * @brief Detects circular dependencies in thread configuration
 * @param threadId The thread ID to start checking from
 * @param path Array to store the dependency path
 * @param pathLen Pointer to the current path length
 * @param visited Array to track visited threads
 * @return uint32_t 1 if circular dependency found, 0 otherwise
 *
 * This function uses a depth-first search approach to detect circular dependencies
 * in the thread configuration. When a circular dependency is found, it stores
 * the complete path of the dependency cycle.
 *
 * The path array should be pre-allocated with size equal to total number
 *       of threads in the system.
 *
 * Example cycle output:
 * Thread A -> Thread B -> Thread C -> Thread A
 */
static uint32_t detectCircularDependency(T_UserThreadEnum threadId,
                                         T_UserThreadEnum *path,
                                         uint32_t *pathLen,
                                         uint8_t *visited)
{
    T_ThreadsDepType *config = findThreadConfig(threadId);
    if (config == NULL)
    {
        return 0;
    }

    /* Add current thread to path */
    path[(*pathLen)++] = threadId;

    /* Mark current thread as visited */
    visited[threadId] = 1;

    /* Check each dependency */
    for (uint32_t i = 0; i < config->depCount; i++)
    {
        T_UserThreadEnum depId = config->dependencies[i];

        /* If we find a thread already in our path, we have a cycle */
        if (visited[depId])
        {
            /* Add the repeated thread to complete the cycle */
            path[(*pathLen)++] = depId;
            return 1;
        }

        /* Recursively check dependencies */
        if (detectCircularDependency(depId, path, pathLen, visited))
        {
            return 1;
        }
    }

    // Remove current thread from path when backtracking
    (*pathLen)--;
    visited[threadId] = 0;

    return 0;
}

/**
 * @brief Prints the dependency cycle path
 * @param path Array containing the dependency cycle
 * @param pathLen Length of the path array
 *
 * This function formats and prints the circular dependency path in a readable format,
 * showing the complete cycle of thread dependencies that forms the circular reference.
 */
static void printDependencyCycle(T_UserThreadEnum *path, uint32_t pathLen)
{
    LOGE_COLOR(COL_RED, "Circular Dependency Detected:");
    LOGE_COLOR(COL_RED, "|-%-*s ---", 18, gUsrThreads_cfg_table[path[0]].name);
    for (uint32_t i = 1; i < pathLen - 1; i++)
    {
        LOGE_COLOR(COL_RED, "|-%-*s    |", 18, gUsrThreads_cfg_table[path[i]].name);
    }
    LOGE_COLOR(COL_RED, "|-%-*s <-|", 18, gUsrThreads_cfg_table[path[pathLen - 1]].name);
}

/**
 * @brief Checks all threads for circular dependencies
 * @param None
 * @return uint32_t Number of circular dependencies found
 *
 * This function performs a comprehensive check of all threads in the system
 * for circular dependencies. It initializes the necessary tracking arrays
 * and calls the recursive detection function for each thread.
 *
 * This function should be called before starting any threads to ensure
 * the thread dependency configuration is valid.
 */
static uint32_t checkAllThreadsForCircularDependencies(void)
{
    uint32_t totalThreads = sizeof(gStartThreadList) / sizeof(T_ThreadsDepType);
    T_UserThreadEnum *path = NULL;
    uint8_t *visited = NULL;
    uint32_t circularDepsFound = 0;

    path = (T_UserThreadEnum *)malloc((totalThreads + 1) * sizeof(T_UserThreadEnum));
    visited = (uint8_t *)malloc(USER_THREAD_MAXNUM * sizeof(uint8_t));

    if (!path || !visited)
    {
        LOGE("Memory allocation failed for dependency checking");
        return 0;
    }

    memset(path, 0, (totalThreads + 1) * sizeof(T_UserThreadEnum));

    /* Check each thread as a starting point */
    for (uint32_t i = 0; i < totalThreads; i++)
    {
        uint32_t pathLen = 0;
        memset(visited, 0, USER_THREAD_MAXNUM * sizeof(uint8_t));

        if (detectCircularDependency(gStartThreadList[i].threadId,
                                     path, &pathLen, visited))
        {
            printDependencyCycle(path, pathLen);
            circularDepsFound++;
            break;
        }
    }

    free(path);
    free(visited);

    return circularDepsFound;
}

/**
 * @brief Initializes the thread management framework
 * @param None
 * @return uint32_t TX_SUCCESS if successful, error code otherwise
 *
 * Must be called before any threads are created. Sets up the event flags
 * and initializes thread status tracking.
 */
static uint32_t threadManagerInit(void)
{
    tx_event_flags_create(&threadInitEvent, "Thread Init Events");
    return TX_SUCCESS;
}

/**
 * @brief Starts all configured threads in the correct order
 * @param None
 * @return uint32_t TX_SUCCESS if successful, error code otherwise
 *
 * Iteratively creates threads while respecting their dependencies.
 * Continues until all threads are created and started.
 */
uint32_t startAllThreads(void)
{
    uint32_t totalCnt = sizeof(gStartThreadList) / sizeof(T_ThreadsDepType);
    uint32_t cnt = 0;

    /* Check if the thread IDs of the dependencies have all been defined */
    if (!checkDependThreadIdValid())
    {
        return TX_NOT_AVAILABLE;
    }

    /* Checks all threads for circular dependencies */
    if (checkAllThreadsForCircularDependencies() > 0)
    {
        return TX_NOT_AVAILABLE;
    }

    threadManagerInit();
    while (cnt < totalCnt)
    {
        for (uint32_t i = 0; i < totalCnt; i++)
        {
            if (gStartThreadList[i].status == THREAD_STATUS_NOT_CREATED)
            {
                if (startThread(&gStartThreadList[i]) == TX_SUCCESS)
                {
                    cnt++;
                }
            }
        }
        tx_thread_sleep(1);
    }

    return TX_SUCCESS;
}