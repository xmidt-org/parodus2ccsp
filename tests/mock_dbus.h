/**
 *  Copyright 2017 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _MOCK_DBUS_H_
#define _MOCK_DBUS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/** How many bits are in the changed_stamp used to validate iterators */
#define CHANGED_STAMP_BITS 21

#define _DBUS_HEADER_FIELD_VALUE_UNKNOWN -1
#define _DBUS_HEADER_FIELD_VALUE_NONEXISTENT -2

/**
 * Header field code for the number of unix file descriptors associated
 * with this message.
 */
#define DBUS_HEADER_FIELD_UNIX_FDS       9

/**
 * Value of the highest-numbered header field code, can be used to determine
 * the size of an array indexed by header field code. Remember though
 * that unknown codes must be ignored, so check for that before
 * indexing the array.
 */
#define DBUS_HEADER_FIELD_LAST DBUS_HEADER_FIELD_UNIX_FDS

/**
 * Initial number of buckets in hash table (hash table statically
 * allocates its buckets for this size and below).
 * The initial mask has to be synced to this.
 */
#define DBUS_SMALL_HASH_TABLE 4

/**
 * The dummy size of the variable-length "elements"
 * field in DBusMemBlock
 */
#define ELEMENT_PADDING 4

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef void* (*CCSP_MESSAGE_BUS_MALLOC) (size_t size); // this signature is different from standard malloc
typedef void  (*CCSP_MESSAGE_BUS_FREE)   (void   *ptr);
typedef int   DBusHandlerResult;
typedef unsigned int dbus_uint32_t;
typedef dbus_uint32_t  dbus_bool_t;
typedef int dbus_int32_t;

/** A process ID */
typedef unsigned long dbus_pid_t;
/** A user ID */
typedef unsigned long dbus_uid_t;

/** Opaque type representing an atomically-modifiable integer
 * that can be used from multiple threads.
 */
typedef struct DBusAtomic DBusAtomic;

typedef struct DBusHeader      DBusHeader;
typedef struct DBusHeaderField DBusHeaderField;

typedef struct DBusString DBusString;

typedef struct DBusList DBusList;

typedef void (* DBusFreeFunction) (void *memory);
typedef struct DBusDataSlotList DBusDataSlotList;

/** Opaque typedef for DBusDataSlot */
typedef struct DBusDataSlot DBusDataSlot;

typedef struct DBusMessage DBusMessage;

/** Opaque type representing a connection to a remote application and associated incoming/outgoing message queues. */
typedef struct DBusConnection DBusConnection;

typedef DBusHandlerResult (*DBusObjectPathMessageFunction) (DBusConnection  *connection,
                                                            DBusMessage     *message,
                                                            void            *user_data);

typedef struct DBusMessageLoader DBusMessageLoader;

/**
 *  * A mutex which is recursive if possible, else non-recursive.
 *   * This is typically recursive, but that cannot be relied upon.
 *    */
typedef struct DBusRMutex DBusRMutex;

/**
 *  * A mutex suitable for use with condition variables.
 *   * This is typically non-recursive.
 *    */
typedef struct DBusCMutex DBusCMutex;

/** An opaque condition variable type provided by the #DBusThreadFunctions implementation installed by dbus_threads_init(). */
typedef struct DBusCondVar DBusCondVar;

typedef struct DBusCounter DBusCounter;

typedef void (*DBusCounterNotifyFunction) (DBusCounter *counter,
                                           void        *user_data);

typedef struct DBusKeyring DBusKeyring;
typedef struct DBusAuth DBusAuth;
/** Object that contains a list of credentials such as UNIX or Windows user ID */
typedef struct DBusCredentials DBusCredentials;
typedef struct DBusTransport DBusTransport;
typedef struct DBusTransportVTable DBusTransportVTable;

/**
 * Called when a #DBusObjectPathVTable is unregistered (or its connection is freed).
 * Found in #DBusObjectPathVTable.
 */
typedef void (*DBusObjectPathUnregisterFunction) (DBusConnection  *connection,
                                                  void            *user_data);

typedef struct DBusWatch DBusWatch;
typedef struct DBusWatchList DBusWatchList;

/** function to run when the watch is handled */
typedef dbus_bool_t (*DBusWatchHandler) (DBusWatch    *watch,
                                         unsigned int  flags,
                                         void         *data);

typedef struct DBusTimeoutList DBusTimeoutList;

/** function to run when the timeout is handled */
typedef dbus_bool_t (*DBusTimeoutHandler) (void *data);

/* Allowing an arbitrary function as with GLib
 * would be nicer for a public API, but for
 * an internal API this saves typing, we can add
 * more whenever we feel like it.
 */
typedef enum
{
  DBUS_HASH_STRING,        /**< Hash keys are strings. */
  DBUS_HASH_INT,           /**< Hash keys are integers. */
  DBUS_HASH_UINTPTR        /**< Hash keys are integer capable to hold a pointer. */
} DBusHashType;

typedef struct DBusHashTable DBusHashTable;

/**
 * Typedef for DBusHashEntry
 */
typedef struct DBusHashEntry DBusHashEntry;

/** A preallocated hash entry */
typedef struct DBusPreallocatedHash DBusPreallocatedHash;

/**
 * Function used to find and optionally create a hash entry.
 */
typedef DBusHashEntry* (*DBusFindEntryFunction) (DBusHashTable        *table,
                                                 void                 *key,
                                                 dbus_bool_t           create_if_not_found,
                                                 DBusHashEntry      ***bucket,
                                                 DBusPreallocatedHash *preallocated);

/**
 * typedef so DBusFreedElement struct can refer to itself.
 */
typedef struct DBusFreedElement DBusFreedElement;
typedef struct DBusMemPool DBusMemPool;

/**
 * Typedef for DBusMemBlock so the struct can recursively
 * point to itself.
 */
typedef struct DBusMemBlock DBusMemBlock;

/**
 * Called when the main loop's thread should be notified that there's now work
 * to do. Set with dbus_connection_set_wakeup_main_function().
 */
typedef void (*DBusWakeupMainFunction) (void *data);

/**
 * Indicates the status of incoming data on a #DBusConnection. This determines whether
 * dbus_connection_dispatch() needs to be called.
 */
typedef enum
{
  DBUS_DISPATCH_DATA_REMAINS,  /**< There is more data to potentially convert to messages. */
  DBUS_DISPATCH_COMPLETE,      /**< All currently available data has been processed. */
  DBUS_DISPATCH_NEED_MEMORY    /**< More memory is needed to continue. */
} DBusDispatchStatus;

typedef enum
{
  DBUS_VALID = 0, /**< the data is valid */
  DBUS_INVALID = 1,
} DBusValidity;

/**
 * This function appends an initial client response to the given string
 */
typedef dbus_bool_t (*DBusInitialResponseFunction)  (DBusAuth         *auth,
                                                     DBusString       *response);

/**
 * This function processes a block of data received from the peer.
 * i.e. handles a DATA command.
 */
typedef dbus_bool_t (*DBusAuthDataFunction)     (DBusAuth         *auth,
                                                 const DBusString *data);

/**
 * This function encodes a block of data from the peer.
 */
typedef dbus_bool_t (*DBusAuthEncodeFunction)   (DBusAuth         *auth,
                                                 const DBusString *data,
                                                 DBusString       *encoded);

/**
 * This function decodes a block of data from the peer.
 */
typedef dbus_bool_t (*DBusAuthDecodeFunction)   (DBusAuth         *auth,
                                                 const DBusString *data,
                                                 DBusString       *decoded);

/**
 * This function is called when the mechanism is abandoned.
 */
typedef void        (*DBusAuthShutdownFunction) (DBusAuth       *auth);

/**
 * Virtual table representing a particular auth mechanism.
 */
typedef struct
{
  const char *mechanism; /**< Name of the mechanism */
  DBusAuthDataFunction server_data_func; /**< Function on server side for DATA */
  DBusAuthEncodeFunction server_encode_func; /**< Function on server side to encode */
  DBusAuthDecodeFunction server_decode_func; /**< Function on server side to decode */
  DBusAuthShutdownFunction server_shutdown_func; /**< Function on server side to shut down */
  DBusInitialResponseFunction client_initial_response_func; /**< Function on client side to handle initial response */
  DBusAuthDataFunction client_data_func; /**< Function on client side for DATA */
  DBusAuthEncodeFunction client_encode_func; /**< Function on client side for encode */
  DBusAuthDecodeFunction client_decode_func; /**< Function on client side for decode */
  DBusAuthShutdownFunction client_shutdown_func; /**< Function on client side for shutdown */
} DBusAuthMechanismHandler;

/**
 * Enumeration for the known authentication commands.
 */
typedef enum {
  DBUS_AUTH_COMMAND_AUTH,
  DBUS_AUTH_COMMAND_CANCEL,
  DBUS_AUTH_COMMAND_DATA,
  DBUS_AUTH_COMMAND_BEGIN,
  DBUS_AUTH_COMMAND_REJECTED,
  DBUS_AUTH_COMMAND_OK,
  DBUS_AUTH_COMMAND_ERROR,
  DBUS_AUTH_COMMAND_UNKNOWN,
  DBUS_AUTH_COMMAND_NEGOTIATE_UNIX_FD,
  DBUS_AUTH_COMMAND_AGREE_UNIX_FD
} DBusAuthCommand;

/** 
 * Called when the return value of dbus_connection_get_dispatch_status()
 * may have changed. Set with dbus_connection_set_dispatch_status_function().
 */
typedef void (*DBusDispatchStatusFunction) (DBusConnection *connection,
                                            DBusDispatchStatus new_status,
                                            void           *data);

typedef struct DBusObjectTree DBusObjectTree;

/** Subnode of the object hierarchy */
typedef struct DBusObjectSubtree DBusObjectSubtree;

/**
 * Called during authentication to check whether the given UNIX user
 * ID is allowed to connect, if the client tried to auth as a UNIX
 * user ID. Normally on Windows this would never happen. Set with
 * dbus_connection_set_unix_user_function().
 */
typedef dbus_bool_t (*DBusAllowUnixUserFunction)  (DBusConnection *connection,
                                                   unsigned long   uid,
                                                   void           *data);

/**
 * Called during authentication to check whether the given Windows user
 * ID is allowed to connect, if the client tried to auth as a Windows
 * user ID. Normally on UNIX this would never happen. Set with
 * dbus_connection_set_windows_user_function().
 */
typedef dbus_bool_t (*DBusAllowWindowsUserFunction)  (DBusConnection *connection,
                                                      const char     *user_sid,
                                                      void           *data);

/**
 * Auth state function, determines the reaction to incoming events for
 * a particular state. Returns whether we had enough memory to
 * complete the operation.
 */
typedef dbus_bool_t (*DBusAuthStateFunction) (DBusAuth         *auth,
                                              DBusAuthCommand   command,
                                              const DBusString *args);

/**
 * Information about a auth state.
 */
typedef struct
{
  const char *name;               /**< Name of the state */
  DBusAuthStateFunction handler;  /**< State function for this state */
} DBusAuthStateData;

/**
 * An atomic integer safe to increment or decrement from multiple threads.
 */
struct DBusAtomic
{
  volatile int value; /**< Value of the atomic integer. */
};

/**
 * Cached information about a header field in the message
 */
struct DBusHeaderField
{
  int            value_pos; /**< Position of field value, or -1/-2 */
};

/**
 * DBusString object
 */
struct DBusString
{
  const char *dummy1; /**< placeholder */
  int   dummy2;       /**< placeholder */
  int   dummy3;       /**< placeholder */
  unsigned int dummy_bit1 : 1; /**< placeholder */
  unsigned int dummy_bit2 : 1; /**< placeholder */
  unsigned int dummy_bit3 : 1; /**< placeholder */
  unsigned int dummy_bits : 3; /**< placeholder */
};

/**
 * Message header data and some cached details of it.
 */
struct DBusHeader
{
  DBusString data; /**< Header network data, stored
                    * separately from body so we can
                    * independently realloc it.
                    */

  DBusHeaderField fields[DBUS_HEADER_FIELD_LAST + 1]; /**< Track the location
                                                       * of each field in header
                                                       */

  dbus_uint32_t padding : 3;        /**< bytes of alignment in header */
  dbus_uint32_t byte_order : 8;     /**< byte order of header */
};

/** An opaque list type */
struct DBusList
{
  DBusList *prev; /**< Previous list node. */
  DBusList *next; /**< Next list node. */
  void     *data; /**< Data stored at this element. */
};

/** DBusDataSlot is used to store application data on the connection */
struct DBusDataSlot
{
  void *data;                      /**< The application data */
  DBusFreeFunction free_data_func; /**< Free the application data */
};

/**
 * Data structure that stores the actual user data set at a given
 * slot.
 */
struct DBusDataSlotList
{
  DBusDataSlot *slots;   /**< Data slots */
  int           n_slots; /**< Slots we have storage for in data_slots */
};

/**
 * @brief Internals of DBusMessage
 *
 * Object representing a message received from or to be sent to
 * another application. This is an opaque object, all members
 * are private.
 */
struct DBusMessage
{
  DBusAtomic refcount; /**< Reference count */

  DBusHeader header; /**< Header network data and associated cache */

  DBusString body;   /**< Body network data. */

  unsigned int locked : 1; /**< Message being sent, no modifications allowed. */

  unsigned int in_cache : 1; /**< Has been "freed" since it's in the cache (this is a debug feature) */

  DBusList *counters;   /**< 0-N DBusCounter used to track message size/unix fds. */
  long size_counter_delta;   /**< Size we incremented the size counters by.   */

  dbus_uint32_t changed_stamp : CHANGED_STAMP_BITS; /**< Incremented when iterators are invalidated. */

  DBusDataSlotList slot_list;   /**< Data stored by allocated integer ID */

  int generation; /**< _dbus_current_generation when message was created */

#ifdef HAVE_UNIX_FD_PASSING
  int *unix_fds;
  /**< Unix file descriptors associated with this message. These are
     closed when the message is destroyed, hence make sure to dup()
     them when adding or removing them here. */
  unsigned n_unix_fds; /**< Number of valid fds in the array */
  unsigned n_unix_fds_allocated; /**< Allocated size of the array */

  long unix_fd_counter_delta; /**< Size we incremented the unix fd counter by */
#endif
};

/**
 * Implementation details of DBusMessageLoader.
 * All members are private.
 */
struct DBusMessageLoader
{
  int refcount;        /**< Reference count. */

  DBusString data;     /**< Buffered data */

  DBusList *messages;  /**< Complete messages. */

  long max_message_size; /**< Maximum size of a message */
  long max_message_unix_fds; /**< Maximum unix fds in a message */

  DBusValidity corruption_reason; /**< why we were corrupted */

  unsigned int corrupted : 1; /**< We got broken data, and are no longer working */

  unsigned int buffer_outstanding : 1; /**< Someone is using the buffer to read */

#ifdef HAVE_UNIX_FD_PASSING
  unsigned int unix_fds_outstanding : 1; /**< Someone is using the unix fd array to read */

  int *unix_fds; /**< File descriptors that have been read from the transport but not yet been handed to any message. Array will be allocated at first use. */
  unsigned n_unix_fds_allocated; /**< Number of file descriptors this array has space for */
  unsigned n_unix_fds; /**< Number of valid file descriptors in array */
#endif
};

struct DBusRMutex {
  pthread_mutex_t lock; /**< the lock */
};

struct DBusCMutex {
  pthread_mutex_t lock; /**< the lock */
};

struct DBusCondVar {
  pthread_cond_t cond; /**< the condition */
};

/**
 * @brief Internals of DBusCounter.
 * 
 * DBusCounter internals. DBusCounter is an opaque object, it must be
 * used via accessor functions.
 */
struct DBusCounter
{
  int refcount;  /**< reference count */

  long size_value;       /**< current size counter value */
  long unix_fd_value;    /**< current unix fd counter value */

  long notify_size_guard_value;    /**< call notify function when crossing this size value */
  long notify_unix_fd_guard_value; /**< call notify function when crossing this unix fd value */

  DBusCounterNotifyFunction notify_function; /**< notify function */
  void *notify_data; /**< data for notify function */
  dbus_bool_t notify_pending : 1; /**< TRUE if the guard value has been crossed */
};

/**
 * Internals of DBusTimeout
 */
struct DBusTimeout
{
  int refcount;                                /**< Reference count */
  int interval;                                /**< Timeout interval in milliseconds. */

  DBusTimeoutHandler handler;                  /**< Timeout handler. */
  void *handler_data;                          /**< Timeout handler data. */
  DBusFreeFunction free_handler_data_function; /**< Free the timeout handler data. */

  void *data;                                  /**< Application data. */
  DBusFreeFunction free_data_function;         /**< Free the application data. */
  unsigned int enabled : 1;                    /**< True if timeout is active. */
};

/**
 * The virtual table that must be implemented to
 * create a new kind of transport.
 */
struct DBusTransportVTable
{
  void        (* finalize)              (DBusTransport *transport);
  /**< The finalize method must free the transport. */

  dbus_bool_t (* handle_watch)          (DBusTransport *transport,
                                         DBusWatch     *watch,
                                         unsigned int   flags);
  /**< The handle_watch method handles reading/writing
   * data as indicated by the flags.
   */

  void        (* disconnect)            (DBusTransport *transport);
  /**< Disconnect this transport. */

  dbus_bool_t (* connection_set)        (DBusTransport *transport);
  /**< Called when transport->connection has been filled in */

  void        (* do_iteration)          (DBusTransport *transport,
                                         unsigned int   flags,
                                         int            timeout_milliseconds);
  /**< Called to do a single "iteration" (block on select/poll
   * followed by reading or writing data).
   */

  void        (* live_messages_changed) (DBusTransport *transport);
  /**< Outstanding messages counter changed */

  dbus_bool_t (* get_socket_fd) (DBusTransport *transport,
                                 int           *fd_p);
  /**< Get socket file descriptor */
};

/**
 * @defgroup DBusCredentialsInternals Credentials implementation details
 * @ingroup  DBusInternals
 * @brief DBusCredentials implementation details
 *
 * Private details of credentials code.
 *
 * @{
 */
struct DBusCredentials {
  int refcount;
  dbus_uid_t unix_uid;
  dbus_pid_t unix_pid;
  char *windows_sid;
  void *adt_audit_data;
  dbus_int32_t adt_audit_data_size;
};

/**
 * A single key from the cookie file
 */
typedef struct
{
  dbus_int32_t id; /**< identifier used to refer to the key */

  long creation_time; /**< when the key was generated,
                       *   as unix timestamp. signed long
                       *   matches struct timeval.
                       */

  DBusString secret; /**< the actual key */

} DBusKey;

/**
 * @brief Internals of DBusKeyring.
 * 
 * DBusKeyring internals. DBusKeyring is an opaque object, it must be
 * used via accessor functions.
 */
struct DBusKeyring
{
  int refcount;             /**< Reference count */
  DBusString directory;     /**< Directory the below two items are inside */
  DBusString filename;      /**< Keyring filename */
  DBusString filename_lock; /**< Name of lockfile */
  DBusKey *keys; /**< Keys loaded from the file */
  int n_keys;    /**< Number of keys */
  DBusCredentials *credentials; /**< Credentials containing user the keyring is for */
};

/**
 * Internal members of DBusAuth.
 */
struct DBusAuth
{
  int refcount;           /**< reference count */
  const char *side;       /**< Client or server */

  DBusString incoming;    /**< Incoming data buffer */
  DBusString outgoing;    /**< Outgoing data buffer */

  const DBusAuthStateData *state;         /**< Current protocol state */

  const DBusAuthMechanismHandler *mech;   /**< Current auth mechanism */

  DBusString identity;                   /**< Current identity we're authorizing
                                          *   as.
                                          *                                             */

  DBusCredentials *credentials;          /**< Credentials read from socket
                                          */

  DBusCredentials *authorized_identity; /**< Credentials that are authorized */

  DBusCredentials *desired_identity;    /**< Identity client has requested */

  DBusString context;               /**< Cookie scope */
  DBusKeyring *keyring;             /**< Keyring for cookie mechanism. */
  int cookie_id;                    /**< ID of cookie to use */
  DBusString challenge;             /**< Challenge sent to client */

  char **allowed_mechs;             /**< Mechanisms we're allowed to use,
                                     * or #NULL if we can use any
                                     *                                      */

  unsigned int needed_memory : 1;   /**< We needed memory to continue since last
                                     * successful getting something done
                                     *                                      */
  unsigned int already_got_mechanisms : 1;       /**< Client already got mech list */
  unsigned int already_asked_for_initial_response : 1; /**< Already sent a blank challenge to get an initial response */
  unsigned int buffer_outstanding : 1; /**< Buffer is "checked out" for reading data into */

  unsigned int unix_fd_possible : 1;  /**< This side could do unix fd passing */
  unsigned int unix_fd_negotiated : 1; /**< Unix fd was successfully negotiated */
};

/**
 * Object representing a transport such as a socket.
 * A transport can shuttle messages from point A to point B,
 * and is the backend for a #DBusConnection.
 *
 */
struct DBusTransport
{
  int refcount;                               /**< Reference count. */

  const DBusTransportVTable *vtable;          /**< Virtual methods for this instance. */

  DBusConnection *connection;                 /**< Connection owning this transport. */

  DBusMessageLoader *loader;                  /**< Message-loading buffer. */

  DBusAuth *auth;                             /**< Authentication conversation */

  DBusCredentials *credentials;               /**< Credentials of other end read from the socket */

  long max_live_messages_size;                /**< Max total size of received messages. */
  long max_live_messages_unix_fds;            /**< Max total unix fds of received messages. */

  DBusCounter *live_messages;                 /**< Counter for size/unix fds of all live messages. */

  char *address;                              /**< Address of the server we are connecting to (#NULL for the server side of a transport) */

  char *expected_guid;                        /**< GUID we expect the server to have, #NULL on server side or if we don't have an expectation */

  DBusAllowUnixUserFunction unix_user_function; /**< Function for checking whether a user is authorized. */
  void *unix_user_data;                         /**< Data for unix_user_function */

  DBusFreeFunction free_unix_user_data;         /**< Function to free unix_user_data */

  DBusAllowWindowsUserFunction windows_user_function; /**< Function for checking whether a user is authorized. */
  void *windows_user_data;                            /**< Data for windows_user_function */

  DBusFreeFunction free_windows_user_data;            /**< Function to free windows_user_data */

  unsigned int disconnected : 1;              /**< #TRUE if we are disconnected. */
  unsigned int authenticated : 1;             /**< Cache of auth state; use _dbus_transport_get_is_authenticated() to query value */
  unsigned int send_credentials_pending : 1;  /**< #TRUE if we need to send credentials */
  unsigned int receive_credentials_pending : 1; /**< #TRUE if we need to receive credentials */
  unsigned int is_server : 1;                 /**< #TRUE if on the server side */
  unsigned int unused_bytes_recovered : 1;    /**< #TRUE if we've recovered unused bytes from auth */
  unsigned int allow_anonymous : 1;           /**< #TRUE if an anonymous client can connect */
};

/**
 * Implementation of DBusWatch
 */
struct DBusWatch
{
  int refcount;                        /**< Reference count */
  int fd;                              /**< File descriptor. */
  unsigned int flags;                  /**< Conditions to watch. */

  DBusWatchHandler handler;                    /**< Watch handler. */
  void *handler_data;                          /**< Watch handler data. */
  DBusFreeFunction free_handler_data_function; /**< Free the watch handler data. */

  void *data;                          /**< Application data. */
  DBusFreeFunction free_data_function; /**< Free the application data. */
  unsigned int enabled : 1;            /**< Whether it's enabled. */
  unsigned int oom_last_time : 1;      /**< Whether it was OOM last time. */
};

/**
 * @brief Internal representation of a hash entry.
 * 
 * A single entry (key-value pair) in the hash table.
 * Internal to hash table implementation.
 */
struct DBusHashEntry
{
  DBusHashEntry *next;    /**< Pointer to next entry in this
                           * hash bucket, or #NULL for end of
                           * chain.
                           */
  void *key;              /**< Hash key */
  void *value;            /**< Hash value */
};

/**
 * struct representing an element on the free list.
 * We just cast freed elements to this so we can
 * make a list out of them.
 */
struct DBusFreedElement
{
  DBusFreedElement *next; /**< next element of the free list */
};

/**
 * DBusMemBlock object represents a single malloc()-returned
 * block that gets chunked up into objects in the memory pool.
 */
struct DBusMemBlock
{
  DBusMemBlock *next;  /**< next block in the list, which is already used up;
                        *   only saved so we can free all the blocks
                        *   when we free the mem pool.
                        */

  /* this is a long so that "elements" is aligned */
  long used_so_far;     /**< bytes of this block already allocated as elements. */

  unsigned char elements[ELEMENT_PADDING]; /**< the block data, actually allocated to required size */
};

/**
 * Internals fields of DBusMemPool
 */
struct DBusMemPool
{
  int element_size;                /**< size of a single object in the pool */
  int block_size;                  /**< size of most recently allocated block */
  unsigned int zero_elements : 1;  /**< whether to zero-init allocated elements */

  DBusFreedElement *free_elements; /**< a free list of elements to recycle */
  DBusMemBlock *blocks;            /**< blocks of memory from malloc() */
  int allocated_elements;          /**< Count of outstanding allocated elements */
};

/**
 * @brief Internals of DBusHashTable.
 * 
 * Hash table internals. Hash tables are opaque objects, they must be
 * used via accessor functions.
 */
struct DBusHashTable {
  int refcount;                       /**< Reference count */

  DBusHashEntry **buckets;            /**< Pointer to bucket array.  Each
                                       * element points to first entry in
                                       *                                        * bucket's hash chain, or #NULL.
                                       *                                                                               */
  DBusHashEntry *static_buckets[DBUS_SMALL_HASH_TABLE];
                                       /**< Bucket array used for small tables
 *                                         * (to avoid mallocs and frees).
 *                                                                                 */
  int n_buckets;                       /**< Total number of buckets allocated
                                        * at **buckets.
                                        *                                         */
  int n_entries;                       /**< Total number of entries present
                                        * in table.
                                        *                                         */
  int hi_rebuild_size;                 /**< Enlarge table when n_entries gets
                                        * to be this large.
                                        *                                         */
  int lo_rebuild_size;                 /**< Shrink table when n_entries gets
                                        * below this.
                                        *                                         */
  int down_shift;                      /**< Shift count used in hashing
                                        * function.  Designed to use high-
                                        *                                         * order bits of randomized keys.
                                        *                                                                                 */
  int mask;                            /**< Mask value used in hashing
                                        * function.
                                        *                                         */
  DBusHashType key_type;               /**< Type of keys used in this table */


  DBusFindEntryFunction find_function; /**< Function for finding entries */

  DBusFreeFunction free_key_function;   /**< Function to free keys */
  DBusFreeFunction free_value_function; /**< Function to free values */

  DBusMemPool *entry_pool;              /**< Memory pool for hash entries */
};

/**
 * Internals of DBusObjectTree
 */
struct DBusObjectTree
{
  int                 refcount;   /**< Reference count */
  DBusConnection     *connection; /**< Connection this tree belongs to */

  DBusObjectSubtree  *root;       /**< Root of the tree ("/" node) */
};

/**
 * Struct representing a single registered subtree handler, or node
 * that's a parent of a registered subtree handler. If
 * message_function != NULL there's actually a handler at this node.
 */
struct DBusObjectSubtree
{
  DBusAtomic                         refcount;            /**< Reference count */
  DBusObjectSubtree                 *parent;              /**< Parent node */
  DBusObjectPathUnregisterFunction   unregister_function; /**< Function to call on unregister */
  DBusObjectPathMessageFunction      message_function;    /**< Function to handle messages */
  void                              *user_data;           /**< Data for functions */
  DBusObjectSubtree                **subtrees;            /**< Child nodes */
  int                                n_subtrees;          /**< Number of child nodes */
  int                                max_subtrees;        /**< Number of allocated entries in subtrees */
  unsigned int                       invoke_as_fallback : 1; /**< Whether to invoke message_function when child nodes don't handle the message */
  char                               name[1]; /**< Allocated as large as necessary */
};

/**
 * Implementation details of DBusConnection. All fields are private.
 */
struct DBusConnection
{
  DBusAtomic refcount; /**< Reference count. */

  DBusRMutex *mutex; /**< Lock on the entire DBusConnection */

  DBusCMutex *dispatch_mutex;     /**< Protects dispatch_acquired */
  DBusCondVar *dispatch_cond;    /**< Notify when dispatch_acquired is available */
  DBusCMutex *io_path_mutex;      /**< Protects io_path_acquired */
  DBusCondVar *io_path_cond;     /**< Notify when io_path_acquired is available */

  DBusList *outgoing_messages; /**< Queue of messages we need to send, send the end of the list first. */
  DBusList *incoming_messages; /**< Queue of messages we have received, end of the list received most recently. */
  DBusList *expired_messages;  /**< Messages that will be released when we next unlock. */

  DBusMessage *message_borrowed; /**< Filled in if the first incoming message has been borrowed;
                                  *   dispatch_acquired will be set by the borrower
                                  *                                     */

  int n_outgoing;              /**< Length of outgoing queue. */
  int n_incoming;              /**< Length of incoming queue. */

  DBusCounter *outgoing_counter; /**< Counts size of outgoing messages. */

  DBusTransport *transport;    /**< Object that sends/receives messages over network. */
  DBusWatchList *watches;      /**< Stores active watches. */
  DBusTimeoutList *timeouts;   /**< Stores active timeouts. */

  DBusList *filter_list;        /**< List of filters. */

  DBusRMutex *slot_mutex;        /**< Lock on slot_list so overall connection lock need not be taken */
  DBusDataSlotList slot_list;   /**< Data stored by allocated integer ID */

  DBusHashTable *pending_replies;  /**< Hash of message serials to #DBusPendingCall. */

  dbus_uint32_t client_serial;       /**< Client serial. Increments each time a message is sent  */
  DBusList *disconnect_message_link; /**< Preallocated list node for queueing the disconnection message */

  DBusWakeupMainFunction wakeup_main_function; /**< Function to wake up the mainloop  */
  void *wakeup_main_data; /**< Application data for wakeup_main_function */
  DBusFreeFunction free_wakeup_main_data; /**< free wakeup_main_data */

  DBusDispatchStatusFunction dispatch_status_function; /**< Function on dispatch status changes  */
  void *dispatch_status_data; /**< Application data for dispatch_status_function */
  DBusFreeFunction free_dispatch_status_data; /**< free dispatch_status_data */

  DBusDispatchStatus last_dispatch_status; /**< The last dispatch status we reported to the application. */

  DBusObjectTree *objects; /**< Object path handlers registered with this connection */

  char *server_guid; /**< GUID of server if we are in shared_connections, #NULL if server GUID is unknown or connection is private */

  /* These two MUST be bools and not bitfields, because they are protected by a separate lock
   * from connection->mutex and all bitfields in a word have to be read/written together.
   * So you can't have a different lock for different bitfields in the same word.
   */
  bool dispatch_acquired; /**< Someone has dispatch path (can drain incoming queue) */
  bool io_path_acquired;  /**< Someone has transport io path (can use the transport to read/write messages) */

  unsigned int shareable : 1; /**< #TRUE if libdbus owns a reference to the connection and can return it from dbus_connection_open() more than once */

  unsigned int exit_on_disconnect : 1; /**< If #TRUE, exit after handling disconnect signal */

  unsigned int route_peer_messages : 1; /**< If #TRUE, if org.freedesktop.DBus.Peer messages have a bus name, don't handle them automatically */

  unsigned int disconnected_message_arrived : 1;   /**< We popped or are dispatching the disconnected message.
                                                    * if the disconnect_message_link is NULL then we queued it, but
                                                    * this flag is whether it got to the head of the queue.
                                                    */
  unsigned int disconnected_message_processed : 1; /**< We did our default handling of the disconnected message,
                                                    * such as closing the connection.
                                                    */

  unsigned int have_connection_lock : 1; /**< Used to check locking */

  int generation; /**< _dbus_current_generation that should correspond to this connection */
};

#endif
