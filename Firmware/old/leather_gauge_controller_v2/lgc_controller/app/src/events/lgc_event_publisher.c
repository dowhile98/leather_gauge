/**
 * @file    lgc_event_publisher.c
 * @brief   Event Publisher Implementation (Observer Pattern)
 * @author  Clean Architecture Refactor Team (TDD)
 * @date    2026-02-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Leather Gauge Controller Team
 */

/* ============================= Includes ============================= */
#include "lgc_event_publisher.h"
#include <string.h>

/* ============================= Private Functions ==================== */

/**
 * @brief Initialize publisher
 */
static Result_t publisher_init_impl(void *ctx)
{
    LGC_VALIDATE_PTR(ctx);

    LgcEventPublisher_t *pub = (LgcEventPublisher_t *)ctx;

    if (pub->is_initialized)
    {
        return ERR_BUSY;
    }

    /* Clear observer list */
    memset(pub->observers, 0, sizeof(pub->observers));
    pub->observer_count = 0;

    /* Create mutex */
    UINT tx_res = tx_mutex_create(&pub->mutex, "event_pub_mutex", TX_NO_INHERIT);
    if (tx_res != TX_SUCCESS)
    {
        return ERR_HARDWARE_FAULT;
    }

    pub->is_initialized = true;
    return ERR_OK;
}

/**
 * @brief Subscribe to events
 */
static Result_t publisher_subscribe_impl(
    void *ctx,
    LgcEventCallback_t callback,
    void *user_ctx,
    LgcEventType_t event_mask)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(callback);

    LgcEventPublisher_t *pub = (LgcEventPublisher_t *)ctx;

    if (!pub->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Lock observer list */
    tx_mutex_get(&pub->mutex, TX_WAIT_FOREVER);

    /* Check if already subscribed */
    for (uint8_t i = 0; i < pub->observer_count; i++)
    {
        if (pub->observers[i].callback == callback)
        {
            /* Already subscribed: Update event mask */
            pub->observers[i].event_mask = event_mask;
            pub->observers[i].is_active = true;
            tx_mutex_put(&pub->mutex);
            return ERR_OK;
        }
    }

    /* Check if list is full */
    if (pub->observer_count >= LGC_MAX_OBSERVERS)
    {
        tx_mutex_put(&pub->mutex);
        return ERR_FULL;
    }

    /* Add new observer */
    pub->observers[pub->observer_count].callback = callback;
    pub->observers[pub->observer_count].context = user_ctx;
    pub->observers[pub->observer_count].event_mask = event_mask;
    pub->observers[pub->observer_count].is_active = true;
    pub->observer_count++;

    tx_mutex_put(&pub->mutex);
    return ERR_OK;
}

/**
 * @brief Unsubscribe from events
 */
static Result_t publisher_unsubscribe_impl(
    void *ctx,
    LgcEventCallback_t callback)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(callback);

    LgcEventPublisher_t *pub = (LgcEventPublisher_t *)ctx;

    if (!pub->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Lock observer list */
    tx_mutex_get(&pub->mutex, TX_WAIT_FOREVER);

    /* Find and remove observer */
    for (uint8_t i = 0; i < pub->observer_count; i++)
    {
        if (pub->observers[i].callback == callback)
        {
            /* Mark as inactive (safer than removing from array) */
            pub->observers[i].is_active = false;
            tx_mutex_put(&pub->mutex);
            return ERR_OK;
        }
    }

    tx_mutex_put(&pub->mutex);
    return ERR_NOT_FOUND;
}

/**
 * @brief Publish event to all matching observers
 */
static Result_t publisher_publish_impl(void *ctx, const LgcEvent_t *event)
{
    LGC_VALIDATE_PTR(ctx);
    LGC_VALIDATE_PTR(event);

    LgcEventPublisher_t *pub = (LgcEventPublisher_t *)ctx;

    if (!pub->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Lock observer list */
    tx_mutex_get(&pub->mutex, TX_WAIT_FOREVER);

    /* Notify all matching observers */
    for (uint8_t i = 0; i < pub->observer_count; i++)
    {
        LgcObserver_t *obs = &pub->observers[i];

        /* Check if observer is active and subscribed to this event */
        if (obs->is_active && (obs->event_mask & event->type))
        {
            /* Call observer callback */
            obs->callback(event, obs->context);
        }
    }

    tx_mutex_put(&pub->mutex);
    return ERR_OK;
}

/**
 * @brief Deinitialize publisher
 */
static Result_t publisher_deinit_impl(void *ctx)
{
    LGC_VALIDATE_PTR(ctx);

    LgcEventPublisher_t *pub = (LgcEventPublisher_t *)ctx;

    if (!pub->is_initialized)
    {
        return ERR_NOT_INITIALIZED;
    }

    /* Delete mutex */
    tx_mutex_delete(&pub->mutex);

    /* Clear observer list */
    memset(pub->observers, 0, sizeof(pub->observers));
    pub->observer_count = 0;

    pub->is_initialized = false;
    return ERR_OK;
}

/* ============================= Public API =========================== */

Result_t LgcEventPublisher_Init(LgcEventPublisher_t *publisher)
{
    LGC_VALIDATE_PTR(publisher);

    /* Clear structure */
    memset(publisher, 0, sizeof(LgcEventPublisher_t));

    return publisher_init_impl(publisher);
}

ILgcEventPublisher_t *LgcEventPublisher_GetInterface(LgcEventPublisher_t *publisher)
{
    if (publisher == NULL)
    {
        return NULL;
    }

    /* Static V-Table (interface) */
    static ILgcEventPublisher_t iface = {
        .context = NULL, /* Will be set below */
        .init = publisher_init_impl,
        .subscribe = publisher_subscribe_impl,
        .unsubscribe = publisher_unsubscribe_impl,
        .publish = publisher_publish_impl,
        .deinit = publisher_deinit_impl};

    /* Update context pointer */
    iface.context = publisher;

    return &iface;
}

Result_t LgcEventPublisher_Deinit(LgcEventPublisher_t *publisher)
{
    return publisher_deinit_impl(publisher);
}
