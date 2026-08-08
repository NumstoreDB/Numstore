

/******************************************************************************
 * SECTION: Data Validator
 * ----------------------------------------------------------------------------
 * @brief A test that looks at two data writers and compares them
 ******************************************************************************/

/**
 * A data validator is used in testing to verify that a data writer
 * matches it's reference data writer. There are two data writers:
 *    ref - The reference writer. This is assumed correct
 *    sut - System under test. This is what we're testing.
 */
struct dvalidtr
{
  struct data_writer ref;
  struct data_writer sut;
  isvalid_func       isvalid;
};

/**
 * @brief Conducts a data validator random test. Loops through
 * [niters] times and calls a random method from insert read remove write
 * with random ranges. It does it for both ref and sut and compares the results
 * to ensure they match
 *
 * @param d The data validator to test on
 * @param size The size to use in the data validator test
 * @param niters The number of iterations to run
 * @param max_insert Maximum insertion length for one insert
 * @param e An error object to handle errors
 * @return Error result
 */
err_t dvalidtr_random_test (
    struct dvalidtr *d,
    u32              size,
    u32              niters,
    u64              max_insert,
    error           *e
);
