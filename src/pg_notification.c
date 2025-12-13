#include "postgres.h"
#include "pgembedded.h"

#include <stdint.h>

#include "commands/async.h"

typedef struct pg_notification_node
{
	char *channel;
	char *payload;
	int sender_pid;
	struct pg_notification_node *next;
} pg_notification_node;

static pg_notification_node *notification_queue_head = NULL;
static pg_notification_node *notification_queue_tail = NULL;



/*
 * Capture notifications into our local queue for embedded mode.
 * This is called by ProcessNotifyInterrupt via our custom whereToSendOutput handler.
 */
static void
pg_embedded_capture_notification(const char *channel, const char *payload, int32 srcPid)
{
	pg_notification_node *node;

	node = (pg_notification_node *) malloc(sizeof(pg_notification_node));
	if (!node)
		return;

	node->channel = strdup(channel);
	node->payload = strdup(payload ? payload : "");
	node->sender_pid = srcPid;
	node->next = NULL;

	if (!node->channel || !node->payload)
	{
		if (node->channel) free(node->channel);
		if (node->payload) free(node->payload);
		free(node);
		return;
	}

	if (notification_queue_tail)
	{
		notification_queue_tail->next = node;
		notification_queue_tail = node;
	}
	else
	{
		notification_queue_head = notification_queue_tail = node;
	}
}

/*
 * pg_embedded_poll_notifications
 *
 * Poll for pending notifications and return the first one
 * Returns a newly allocated pg_notification structure that must be freed
 * with pg_embedded_free_notification(), or NULL if no notifications pending
 *
 * In embedded mode, we collect notifications by overriding NotifyMyFrontEnd()
 * to store them in a local queue. This function processes the async notification
 * queue (by calling ProcessNotifyInterrupt) and returns notifications one at a time.
 *
 * Only call this after initializing the DB
 */
pg_notification *
pg_embedded_poll_notifications(void)
{
	pg_notification *result = NULL;
	pg_notification_node *node;

	PG_TRY();
	{
		ProcessNotifyInterrupt(false);
	}
	PG_CATCH();
	{
		ErrorData *edata;

		edata = CopyErrorData();
		FlushErrorState();
		snprintf(pg_error_msg, sizeof(pg_error_msg),
				 "Poll notifications failed: %s", edata->message);
		FreeErrorData(edata);
	}
	PG_END_TRY();

	if (notification_queue_head)
	{
		node = notification_queue_head;
		notification_queue_head = node->next;
		if (!notification_queue_head)
			notification_queue_tail = NULL;

		result = (pg_notification *) malloc(sizeof(pg_notification));
		if (result)
		{
			result->channel = node->channel;
			result->payload = node->payload;
			result->sender_pid = node->sender_pid;
		}
		else
		{
			free(node->channel);
			free(node->payload);
			snprintf(pg_error_msg, sizeof(pg_error_msg), "Out of memory");
		}
		free(node);
	}

	return result;
}


/*
 * pg_embedded_free_notification
 *
 * Free a notification structure returned by pg_embedded_poll_notifications
 */
void
pg_embedded_free_notification(pg_notification *notification)
{
	if (!notification)
		return;

	if (notification->channel)
		free(notification->channel);
	if (notification->payload)
		free(notification->payload);
	free(notification);
}


void reset_notification_queue() {
	while (notification_queue_head)
	{
		pg_notification_node *node = notification_queue_head;
		notification_queue_head = node->next;
		free(node->channel);
		free(node->payload);
		free(node);
	}
	notification_queue_tail = NULL;
}

void install_notification_hook() {
	pg_notify_hook = pg_embedded_capture_notification;
}
