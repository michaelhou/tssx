#ifndef EPOLL_OVERRIDES_H
#define EPOLL_OVERRIDES_H

#include <signal.h>
#include <stddef.h>
#include <sys/epoll.h>

#include "tssx/common-poll-overrides.h"
#include "tssx/vector.h"

/******************** DEFINITIONS ********************/

#define NUMBER_OF_EPOLL_INSTANCES 1024
#define INITIAL_INSTANCE_CAPACITY 8

/******************** REAL FUNCTIONS ********************/

typedef int (*real_epoll_create_t)(int);
typedef int (*real_epoll_create1_t)(int);
typedef int (*real_epoll_ctl_t)(int, int, int, struct epoll_event *);
typedef int (*real_epoll_wait_t)(int, struct epoll_event *, int, int);

// clang-format off
typedef int (*real_epoll_pwait_t)
  (int, struct epoll_event *, int, int, const sigset_t *);
// clang-format on

/******************** STRUCTURES ********************/

struct Connection;

typedef int epoll_operation_t;

typedef struct EpollEntry {
	int fd;
	struct epoll_event event;
	Connection *connection;
	short flags;
} EpollEntry;

typedef struct EpollInstance {
	struct Vector entries;
	struct EpollEntry first;
	size_t tssx_count;
	size_t normal_count;
} EpollInstance;

typedef struct EpollTask {
	int epfd;
	struct epoll_event *events;
	size_t number_of_events;
	int timeout;
	event_count_t *shared_event_count;
	int event_count;
} EpollTask;

/******************** REAL FUNCTIONS ********************/

int real_epoll_create(int size);
int real_epoll_create1(int flags);
int real_epoll_ctl(int epfd, int operation, int fd, struct epoll_event *event);
int real_epoll_wait(int epfd,
										struct epoll_event *events,
										int number_of_events,
										int timeout);
int real_epoll_pwait(int epfd,
										 struct epoll_event *events,
										 int number_of_events,
										 int timeout,
										 const sigset_t *sigmask);

/******************** OVERRIDES ********************/

int epoll_create(int size);
int epoll_create1(int flags);

int epoll_ctl(int epfd, int operation, int fd, struct epoll_event *event);

int epoll_wait(int epfd,
							 struct epoll_event *events,
							 int number_of_events,
							 int timeout);
int epoll_pwait(int epfd,
								struct epoll_event *events,
								int number_of_events,
								int timeout,
								const sigset_t *sigmask);

bool has_epoll_instance_associated(int epfd);

int epoll_instance_size(int epfd);

int close_epoll_instance(int epfd);

/******************** PRIVATE DEFINITIONS ********************/

#define ENABLED 0x1
#define READ_EDGE 0x2
#define WRITE_EDGE 0x4

extern epoll_operation_t _epoll_operation_map[2];
extern EpollInstance _epoll_instances[NUMBER_OF_EPOLL_INSTANCES];
extern bool _epoll_instances_are_initialized;

extern pthread_mutex_t _epoll_lock;

/******************** PRIVATE ********************/

int _setup_epoll_instances();

int _epoll_tssx_control_operation(int epfd,
																	int operation,
																	int fd,
																	struct epoll_event *event,
																	Session *session);

int _epoll_normal_control_operation(int epfd,
																		int operation,
																		int fd,
																		struct epoll_event *event);

int _epoll_add_to_instance(EpollInstance *instance,
													 int fd,
													 struct epoll_event *event,
													 Session *session);

int _epoll_set_first_entry(EpollInstance *instance,
													 int fd,
													 struct epoll_event *event,
													 Session *session);

int _epoll_push_back_entry(EpollInstance *instance,
													 int fd,
													 struct epoll_event *event,
													 Session *session);

int _epoll_update_instance(EpollInstance *instance,
													 int fd,
													 const struct epoll_event *new_event);

EpollEntry *_find_epoll_entry(EpollInstance *instance, int fd);

int _epoll_erase_from_instance(EpollInstance *instance, int fd);
int _epoll_erase_first_from_instance(EpollInstance *instance);

int _simple_tssx_epoll_wait(EpollInstance *instance,
														struct epoll_event *events,
														size_t number_of_events,
														int timeout);

int _concurrent_epoll_wait(int epfd,
													 struct epoll_event *events,
													 size_t number_of_events,
													 int timeout);

int _start_normal_epoll_wait_thread(pthread_t *normal_thread, EpollTask *task);
void _normal_epoll_wait(EpollTask *task);
int _concurrent_tssx_epoll_wait(EpollInstance *instance,
																struct epoll_event *events,
																size_t number_of_events,
																pthread_t normal_thread,
																event_count_t *shared_event_count,
																int timeout);

int _tssx_epoll_wait_for_single_entry(EpollEntry *entry,
																			struct epoll_event *events,
																			size_t number_of_events,
																			int timeout);

bool _check_epoll_entry(EpollEntry *entry,
												struct epoll_event *events,
												size_t number_of_events,
												size_t event_count);
bool _check_epoll_event(EpollEntry *entry,
												struct epoll_event *output_event,
												Operation operation);

bool _epoll_operation_registered(EpollEntry *entry, size_t operation_index);
bool _epoll_event_registered(const EpollEntry *entry, int event);

bool _epoll_peer_died(EpollEntry *entry);

void _notify_of_epoll_hangup(const EpollEntry *entry,
														 struct epoll_event *output_event);

Operation _convert_operation(size_t operation_index);
void _invalid_argument_exception();

void _destroy_epoll_lock();

bool _is_edge_triggered(const EpollEntry *entry);
bool _is_level_triggered(const EpollEntry *entry);

bool _is_oneshot(const EpollEntry *entry);
bool _is_edge_for(const EpollEntry *entry, Operation operation);
bool _is_enabled(const EpollEntry *entry);

void _set_poll_edge(EpollEntry *entry, Operation operation);
void _clear_poll_edge(EpollEntry *entry, Operation operation);

void _enable_poll_entry(EpollEntry *entry);
void _disable_poll_entry(EpollEntry *entry);

int _lazy_epoll_setup();
bool _lazy_epoll_setup_and_error();
int _validate_epoll_wait_arguments(int epfd, int number_of_events);

#endif /* EPOLL_OVERRIDES_H */
