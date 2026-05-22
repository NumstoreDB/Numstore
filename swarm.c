#include <assert.h>
#include <math.h>
#include <rpcndr.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct nsdb nsdb_t;
typedef struct error error;
typedef struct type type;
typedef int sb_size;
typedef unsigned int b_size;

int type_byte_size( const struct type* t );
struct type* random_type();
char* type_str( struct type* t );
char* random_name( int min, int max );
void type_free( struct type* t );

// Lifecycle
nsdb_t* nsdb_open( const char* path );
int nsdb_cleanup( const char* path );
nsdb_t* nsdb_new_context( nsdb_t* ns );
int nsdb_close( nsdb_t* ns );
int nsdb_crash( nsdb_t* ns );

// Simple api
const char* nsdb_strerror( nsdb_t* ns );
int nsdb_perror( nsdb_t* ns, const char* prefix );
int nsdb_delete( nsdb_t* ns, const char* vname );

// Transaction Support
int nsdb_begin( nsdb_t* ns );
int nsdb_commit( nsdb_t* ns );
int nsdb_rollback( nsdb_t* ns );

// Variable API
int nsdb_create( nsdb_t* ns, const char* name, const char* type );
int nsdb_delete( nsdb_t* ns, const char* name );

// Primary API
sb_size nsdb_len( nsdb_t* ns, const char* vname );

// In array notation [a:b:c]
#define STOP_PRESENT ( 1 << 0 ) // [:c]
#define STEP_PRESENT ( 1 << 1 ) // [:b:]
#define START_PRESENT ( 1 << 2 ) // [a:]
#define COLON_PRESENT ( 1 << 3 ) // [:]

sb_size nsdb_insert( nsdb_t* ns, const char* name, const void* src, sb_size ofst, b_size slen );

sb_size nsdb_write(
  nsdb_t* ns,
  const char* name,
  const void* src,
  sb_size start,
  sb_size step,
  sb_size stop,
  int flags );

sb_size nsdb_read( nsdb_t* ns, const char* name, void* dest, sb_size start, sb_size step, sb_size stop, int flags );

sb_size nsdb_remove( nsdb_t* ns, const char* name, void* dest, sb_size start, sb_size step, sb_size stop, int flags );

struct action
{
  // Number of unique groups = 2^n - 1

  enum action_type
  {
    BEGIN_TXN, // Begin a new transaction
    COMMIT_TXN, // Commit this open transaction
    ROLLBACK_TXN, // Rollback this open transaction
    CRASH_AND_REOPEN, // Crash the database
    CLOSE_AND_REOPEN, // Close the database cleanly
    INSERT, // Insert a random amount of data
    REMOVE, // Remove a random amount of data
    READ, // Read a random amount of data
    WRITE, // Write a random amount of data
    CREATE, // Create a new variable
    SWITCH, // Switch to a different existing variable
    DELETE, // Delete an existing variable

    AT_LEN,
  } type;

  int weight;
};

struct var_meta
{
  char* name;
  char* typestr;
  struct type* type;
  int len;
};

struct var_meta random_var_meta( int minname, int maxname )
{
  char* name = random_name( minname, maxname );
  struct type* type = random_type();
  char* typestr = type_str( type );

  return (struct var_meta) {
    .name = name,
    .typestr = typestr,
    .type = type,
    .len = 0,
  };
}

void free_var_meta( struct var_meta* meta )
{
  free( meta->name );
  free( meta->typestr );
  type_free( meta->type );
}

struct swarm_test
{
  // Existing variables to pull from during a switch
  struct var_meta vars[ 40 ];
  int vlen; // Length

  // Currently selected variable
  struct var_meta* current;

  int enabled[ AT_LEN ]; // What actions are currently available
  int allowed[ AT_LEN ]; // What actions we can actually take (e.g. remove is enabled but len = 0)

  // Main db instance
  nsdb_t* db;

  int in_txn;
  const char* dbname;

  int max_insert_len;

  // Probability that after a test step that our list of enabled actions changes
  // When it does change - a random set of actions are selected from enabled
  // 0 means true swarm test
  float probability_of_swarm_change;

  // Probability that after a test step that we do one big read of all the data
  // in reference and system under test and compare them
  float probability_of_full_read;
};

void swarm_set_random_enabled( struct swarm_test* test )
{
  /**
   * To get a uniform distribution of 
   * selections from our swarm options:
   *
   * N = 2^C - 1
   * N' (without choice c) = 2^(C-1) - 1 
   *
   * Number of times c belongs to samples = N - N' = ... =  2^(C - 1)
   * Probability c belongs in samples = (N - N') / N
   *    = (2^(C-1) / (2^C - 1))
   */
  for ( int i = 0; i < AT_LEN; ++i ) {
    float c = (float) rand() / (float) RAND_MAX;
    float p = ( powf( 2.0, AT_LEN - 1 ) / ( powf( 2, AT_LEN ) - 1 ) );

    if ( c <= p ) {
      test->enabled[ i ] = 1;
    } else {
      test->enabled[ i ] = 0;
    }
  }
}

void swarm_set_allowed( struct swarm_test* test )
{
  assert( test );
  assert( test->db );
  assert( test->dbname );

  memset( test->allowed, 0, sizeof( test->allowed ) );

  // Can always crash
  test->allowed[ CRASH_AND_REOPEN ] = 1;

  // Can create if we have room
  if ( test->vlen < 40 ) {
    test->allowed[ CREATE ] = 1;
  }

  if ( !test->in_txn ) {
    // Can begin txn if not in txn
    test->allowed[ BEGIN_TXN ] = 1;

    // Can close if we're done with all txns
    test->allowed[ CLOSE_AND_REOPEN ] = 1;
  }

  if ( test->in_txn ) {
    // Can commit if we are in a txn
    test->allowed[ COMMIT_TXN ] = 1;

    // Can rollback if we are in a txn
    test->allowed[ ROLLBACK_TXN ] = 1;
  }

  if ( test->current ) {
    assert( test->vlen > 0 );

    // Can insert if we have any variable
    test->allowed[ INSERT ] = 1;

    if ( test->current->len > 0 ) {
      test->allowed[ REMOVE ] = 1;
      test->allowed[ READ ] = 1;
      test->allowed[ WRITE ] = 1;
    }
  } else {
    assert( test->vlen == 0 );
  }

  if ( test->vlen > 1 ) {
    test->allowed[ DELETE ] = 1;
    test->allowed[ SWITCH ] = 1;
  }
}

struct swarm_test* swarm_test_create(
  float probability_of_swarm_change,
  float probability_of_full_read,
  int start_enabled[ AT_LEN ],
  const char* dbname,
  int max_insert_len )
{
  struct swarm_test* ret = malloc( sizeof *ret );

  *ret = (struct swarm_test) {
    .vlen = 0,
    .current = NULL,
    .db = nsdb_open( dbname ),
    .in_txn = 0,
    .dbname = dbname,
    .max_insert_len = max_insert_len,
    .probability_of_swarm_change = probability_of_swarm_change,
    .probability_of_full_read = probability_of_full_read,
  };

  memcpy( ret->enabled, start_enabled, AT_LEN * sizeof( int ) );
}

void swarm_test_fail()
{
  printf( "Failed swarm test\n" );
}

//////////////////////////////////////////////
///// Action Framework

void swarm_test_begin_txn( struct swarm_test* meta )
{
  assert( !meta->in_txn );

  if ( nsdb_begin( meta->db ) ) {
    swarm_test_fail();
  }

  meta->in_txn = 1;
}

void swarm_test_commit_txn( struct swarm_test* meta )
{
  assert( meta->in_tx );

  if ( nsdb_commit( meta->db ) ) {
    swarm_test_fail();
  }

  meta->in_txn = 0;
}

void swarm_test_rollback_txn( struct swarm_test* meta )
{
  assert( meta->in_tx );

  if ( nsdb_rollback( meta->db ) ) {
    swarm_test_fail();
  }

  meta->in_txn = 0;
}

void swarm_test_crash_and_reopen( struct swarm_test* meta )
{
  // Crash
  if ( nsdb_crash( meta->db ) ) {
    swarm_test_fail();
  }

  // Reopen
  meta->db = nsdb_open( meta->dbname );
  if ( meta->db == NULL ) {
    swarm_test_fail();
  }

  meta->in_txn = 0;
}

void swarm_test_close_and_reopen( struct swarm_test* meta )
{
  // Close
  if ( nsdb_close( meta->db ) ) {
    swarm_test_fail();
  }

  // Reopen
  meta->db = nsdb_open( meta->dbname );
  if ( meta->db == NULL ) {
    swarm_test_fail();
  }

  meta->in_txn = 0;
}

void swarm_test_insert( struct swarm_test* meta )
{
  // Construct Parameters
  int len = ( rand() % ( meta->max_insert_len ) ) + 1;
  int ofst = rand() % ( meta->current->len + 1 );

  // The data to insert
  int blen = len * type_byte_size( meta->current->type );
  uint8_t* data = malloc( blen );
  for ( int i = 0; i < blen; ++i ) {
    data[ i ] = rand();
  }

  // Insert data
  if ( nsdb_insert( meta->db, meta->current->name, data, ofst, len ) ) {
    swarm_test_fail();
  }

  meta->current->len += len;

  free( data );
}

void swarm_test_remove( struct swarm_test* meta )
{
  // Construct Parameters
  int ofst = rand() % ( meta->current->len + 1 ); // [ 0, len ]
  int stride = rand() % meta->current->len - ofst; // [ 0, len - ofst ]
  int len = rand() % ( ( meta->current->len - ofst ) / stride ); // [ 0, (len - ofst) / stride ]

  // The removed data
  uint8_t* data = calloc( len, type_byte_size( meta->current->type ) );

  // Do Remove
  if ( nsdb_remove( meta->db, meta->current->name, data, ofst, stride, ofst + len, 0xFFFFFFFF ) ) {
    swarm_test_fail();
  }

  assert( meta->current->len > len );
  meta->current->len -= len;

  free( data );
}

void swarm_test_read( struct swarm_test* meta )
{
  // Construct Parameters
  int ofst = rand() % ( meta->current->len + 1 ); // [ 0, len ]
  int stride = rand() % meta->current->len - ofst; // [ 0, len - ofst ]
  int len = rand() % ( ( meta->current->len - ofst ) / stride ); // [ 0, (len - ofst) / stride ]

  // The removed data
  uint8_t* data = calloc( len, type_byte_size( meta->current->type ) );

  // Do Remove
  if ( nsdb_read( meta->db, meta->current->name, data, ofst, stride, ofst + len, 0xFFFFFFFF ) ) {
    swarm_test_fail();
  }

  free( data );
}

void swarm_test_write( struct swarm_test* meta )
{
  // Construct Parameters
  int ofst = rand() % ( meta->current->len + 1 ); // [ 0, len ]
  int stride = rand() % meta->current->len - ofst; // [ 0, len - ofst ]
  int len = rand() % ( ( meta->current->len - ofst ) / stride ); // [ 0, (len - ofst) / stride ]

  // The data to write
  int blen = len * type_byte_size( meta->current->type );
  uint8_t* data = malloc( blen );
  for ( int i = 0; i < blen; ++i ) {
    data[ i ] = rand();
  }

  // Do Remove
  if ( nsdb_write( meta->db, meta->current->name, data, ofst, stride, ofst + len, 0xFFFFFFFF ) ) {
    swarm_test_fail();
  }

  free( data );
}

void swarm_test_create( struct swarm_test* meta )
{
  nsdb_create( meta->db, /* random name not already exists */, /* random type*/ );

  // Append entry to meta->vars
}

void swarm_test_switch( struct swarm_test* meta )
{
  // Switch to random entry inside our enabled vars

  // Enabled = >= 2 vars available
}

void swarm_test_delete( struct swarm_test* meta )
{
  // Pick a random non chosen one to delete

  // Enabled = >= 2 vars available
}

void run_swarm_test()
{
  struct swarm_test* meta = malloc( sizeof *meta );
  meta->vars = malloc( sizeof *meta->vars );

  meta->vars[ 0 ] = (struct var_meta) {
    .name = malloc( sizeof( "start" ) ),
    .type = malloc( sizeof( "u32" ) ),
    .len = 0,
  };

  // TODO - set up signal for timer

  while ( 1 ) {
  }
}
