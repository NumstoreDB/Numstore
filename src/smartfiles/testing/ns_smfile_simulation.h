#ifndef NS_SMFILE_SIMULATION_H
#define NS_SMFILE_SIMULATION_H

enum smfile_action_type
{
  SMF_BEGIN_TXN,
  SMF_COMMIT_TXN,
  SMF_ROLLBACK_TXN,

  SMF_CRASH_REOPEN,
  SMF_CLOSE_REOPEN,

  SMF_INSERT,
  SMF_REMOVE,
  SMF_READ,
  SMF_WRITE,

  SMF_AT_LEN,
};

// Open a new simulation
struct smfile_simulation *smf_simul_open (
    int         initial_enabled[SMF_AT_LEN], // Which options above are enabled at t=0
    const char *dbname,                      // Database name
    int         max_insert_len,              // Maximum bytes to insert
    int         max_size,                    // Maximum size to read / write / remove
    float       sample_space_prob            // Probability that enabled state changes
);

// Close the simulation
int smfile_simul_close (struct smfile_simulation *smfs);

// Do one simulation step
int smfile_simul_step (struct smfile_simulation *smfs);

#endif
