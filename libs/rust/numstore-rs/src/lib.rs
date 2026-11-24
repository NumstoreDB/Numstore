mod errors;
mod numstore;

pub use errors::{Error, ErrorType, Result};

use std::ptr::NonNull;

use crate::numstore::{
    b_size, nsdb_plan_t, nsdb_t, nsdb_txn_t, nsdb_var_t, rs_ns_begin, rs_ns_cleanup, rs_ns_close,
    rs_ns_commit, rs_ns_open, rs_ns_plan_create, rs_ns_plan_execute, rs_ns_plan_free,
    rs_ns_plan_get_var, rs_ns_plan_malloc, rs_ns_plan_read, rs_ns_plan_write, rs_ns_rollback,
    rs_ns_var_free, rs_ns_var_len, CBuf,
};

pub struct Database {
    handle: NonNull<nsdb_t>,
}

pub struct Variable {
    handle: NonNull<nsdb_var_t>,
}

pub struct Plan {
    handle: NonNull<nsdb_plan_t>,
}

pub struct Txn {
    handle: NonNull<nsdb_txn_t>,
}

impl Plan {
    fn new(db: &Database, query: &str) -> Result<Plan> {
        Ok(Plan {
            handle: rs_ns_plan_create(db.handle, query)?,
        })
    }

    pub fn execute(&self, txn: Option<&Txn>) -> Result<()> {
        rs_ns_plan_execute(self.handle, txn.map(|t| t.handle))
    }

    /// None is `get if exists` on a variable that isn't there
    pub fn get(&self, txn: Option<&Txn>) -> Result<Option<Variable>> {
        let handle = rs_ns_plan_get_var(self.handle, txn.map(|t| t.handle))?;
        Ok(handle.map(|handle| Variable { handle }))
    }

    pub fn read(&self, txn: Option<&Txn>, dest: &mut [u8]) -> Result<b_size> {
        rs_ns_plan_read(self.handle, txn.map(|t| t.handle), dest)
    }

    pub fn write(&self, txn: Option<&Txn>, src: &[u8]) -> Result<b_size> {
        rs_ns_plan_write(self.handle, txn.map(|t| t.handle), src)
    }

    pub fn read_malloc(&self, txn: Option<&Txn>) -> Result<CBuf> {
        rs_ns_plan_malloc(self.handle, txn.map(|t| t.handle))
    }
}

impl Drop for Plan {
    fn drop(&mut self) {
        rs_ns_plan_free(self.handle);
    }
}

impl Variable {
    pub fn len(&self) -> b_size {
        rs_ns_var_len(self.handle)
    }
}

impl Drop for Variable {
    fn drop(&mut self) {
        rs_ns_var_free(self.handle);
    }
}

impl Database {
    pub fn open(fname: &str) -> Result<Database> {
        Ok(Database {
            handle: rs_ns_open(fname)?,
        })
    }

    /// Remove the database at [fname]
    pub fn cleanup(fname: &str) -> Result<()> {
        rs_ns_cleanup(fname)
    }

    /// Transactions

    pub fn begin(&self) -> Result<Txn> {
        Ok(Txn {
            handle: rs_ns_begin(self.handle)?,
        })
    }

    pub fn commit(&self, txn: &Txn) -> Result<()> {
        rs_ns_commit(self.handle, txn.handle)
    }

    pub fn rollback(&self, txn: &Txn) -> Result<()> {
        rs_ns_rollback(self.handle, txn.handle)
    }

    /// Queries

    pub fn execute(&self, txn: Option<&Txn>, query: &str) -> Result<()> {
        Plan::new(self, query)?.execute(txn)
    }

    pub fn get(&self, txn: Option<&Txn>, query: &str) -> Result<Option<Variable>> {
        Plan::new(self, query)?.get(txn)
    }

    pub fn read(&self, txn: Option<&Txn>, dest: &mut [u8], query: &str) -> Result<b_size> {
        Plan::new(self, query)?.read(txn, dest)
    }

    pub fn write(&self, txn: Option<&Txn>, src: &[u8], query: &str) -> Result<b_size> {
        Plan::new(self, query)?.write(txn, src)
    }

    pub fn read_malloc(&self, txn: Option<&Txn>, query: &str) -> Result<CBuf> {
        Plan::new(self, query)?.read_malloc(txn)
    }
}

impl Drop for Database {
    fn drop(&mut self) {
        match rs_ns_close(self.handle) {
            Ok(()) => (),
            Err(e) => {
                eprintln!("Failed to close database: {:?}", e);
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::errors::ErrorType;

    // Removes the database files once the test ends. Declare it before the
    // database so it drops after the database is closed.
    struct Cleanup(String);
    impl Drop for Cleanup {
        fn drop(&mut self) {
            let _ = Database::cleanup(&self.0);
        }
    }

    fn db_path(name: &str) -> String {
        std::env::temp_dir()
            .join(format!("numstore_rs_{}_{}", name, std::process::id()))
            .to_str()
            .unwrap()
            .to_string()
    }

    fn u32_bytes(vals: &[u32]) -> Vec<u8> {
        vals.iter().flat_map(|v| v.to_ne_bytes()).collect()
    }

    // Create a database with a u32 variable "foo" holding [0, 1, ..., 9]
    fn populated(db: &Database) -> Result<()> {
        db.execute(None, "create foo u32")?;
        let src = u32_bytes(&(0..10).collect::<Vec<_>>());
        assert_eq!(db.write(None, &src, "insert foo 0 10")?, 10);
        Ok(())
    }

    // =========================================================================
    // Lifecycle
    // =========================================================================

    #[test]
    fn open_and_close() -> Result<()> {
        let path = db_path("open_and_close");
        let _clean = Cleanup(path.clone());
        let _db = Database::open(&path)?;
        Ok(())
    }

    #[test]
    fn open_rejects_interior_nul() {
        let err = Database::open("foo\0bar").err().unwrap();
        assert!(matches!(err.code, ErrorType::InvalidArgument));
    }

    #[test]
    fn open_unwritable_path_fails() {
        let err = Database::open("/nonexistent_dir/numstore_rs/db")
            .err()
            .unwrap();
        assert!(matches!(err.code, ErrorType::IoErr));
        assert!(!err.message.is_empty());
    }

    #[test]
    fn reopen_persists_data() -> Result<()> {
        let path = db_path("reopen");
        let _clean = Cleanup(path.clone());
        {
            let db = Database::open(&path)?;
            populated(&db)?;
        }

        let db = Database::open(&path)?;
        assert_eq!(db.get(None, "get foo")?.unwrap().len(), 10);
        Ok(())
    }

    // =========================================================================
    // Transactions
    // =========================================================================

    #[test]
    fn begin_commit() -> Result<()> {
        let path = db_path("commit");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        let txn = db.begin()?;
        db.execute(Some(&txn), "create foo u64")?;
        db.commit(&txn)?;

        assert_eq!(db.get(None, "get foo")?.unwrap().len(), 0);
        Ok(())
    }

    #[test]
    fn rollback_discards_changes() -> Result<()> {
        let path = db_path("rollback");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        let txn = db.begin()?;
        db.execute(Some(&txn), "create foo u64")?;
        db.rollback(&txn)?;

        assert!(db.get(None, "get foo").is_err());
        Ok(())
    }

    // =========================================================================
    // Queries
    // =========================================================================

    #[test]
    fn execute_create() -> Result<()> {
        let path = db_path("execute");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        db.execute(None, "create foo u32")?;
        Ok(())
    }

    #[test]
    fn execute_invalid_syntax_fails() -> Result<()> {
        let path = db_path("bad_syntax");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        let err = db.execute(None, "create foo INVALID").unwrap_err();
        assert!(matches!(err.code, ErrorType::IoErr));
        Ok(())
    }

    #[test]
    fn query_rejects_interior_nul() -> Result<()> {
        let path = db_path("query_nul");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        let err = db.execute(None, "create foo\0 u32").unwrap_err();
        assert!(matches!(err.code, ErrorType::InvalidArgument));
        Ok(())
    }

    #[test]
    fn get_returns_variable() -> Result<()> {
        let path = db_path("get");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        db.execute(None, "create foo u32")?;
        assert_eq!(db.get(None, "get foo")?.unwrap().len(), 0);
        Ok(())
    }

    #[test]
    fn get_missing_variable_fails() -> Result<()> {
        let path = db_path("get_missing");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        assert!(db.get(None, "get nope").is_err());
        Ok(())
    }

    #[test]
    fn get_if_exists_missing_variable_is_none() -> Result<()> {
        let path = db_path("get_if_exists_missing");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        assert!(db.get(None, "get if exists nope")?.is_none());

        db.execute(None, "create foo u32")?;
        assert!(db.get(None, "get if exists foo")?.is_some());
        Ok(())
    }

    // A failed call's error is taken when it is turned into an Err, so it
    // isn't mistaken for the error of a later call
    #[test]
    fn get_if_exists_after_a_failed_call_is_none() -> Result<()> {
        let path = db_path("get_if_exists_after_failure");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        let err = db.execute(None, "this is not a query").err().unwrap();
        assert!(!err.message.is_empty());
        assert!(db.get(None, "get if exists nope")?.is_none());

        // The same inside a transaction
        let txn = db.begin()?;
        let mut dest = [0u8; 4];
        assert!(db.read(Some(&txn), &mut dest, "read nope").is_err());
        assert!(db.get(Some(&txn), "get if exists nope")?.is_none());
        db.rollback(&txn)?;
        Ok(())
    }

    #[test]
    fn write_grows_variable() -> Result<()> {
        let path = db_path("write");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        populated(&db)?;
        assert_eq!(db.get(None, "get foo")?.unwrap().len(), 10);
        Ok(())
    }

    #[test]
    fn write_to_missing_variable_fails() -> Result<()> {
        let path = db_path("write_missing");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        let src = u32_bytes(&[1, 2, 3]);
        assert!(db.write(None, &src, "insert nope 0 3").is_err());
        Ok(())
    }

    #[test]
    fn read_roundtrip() -> Result<()> {
        let path = db_path("read");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;
        populated(&db)?;

        let mut dest = vec![0u8; 40];
        assert_eq!(db.read(None, &mut dest, "read foo[0:10]")?, 10);
        assert_eq!(dest, u32_bytes(&(0..10).collect::<Vec<_>>()));
        Ok(())
    }

    #[test]
    fn read_into_small_buffer_truncates() -> Result<()> {
        let path = db_path("read_small");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;
        populated(&db)?;

        let mut dest = vec![0u8; 4];
        let n = db.read(None, &mut dest, "read foo[0:10]")?;
        assert_eq!(n, 1);
        assert_eq!(dest, u32_bytes(&[0]));
        Ok(())
    }

    #[test]
    fn read_malloc_roundtrip() -> Result<()> {
        let path = db_path("read_malloc");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;
        populated(&db)?;

        let buf = db.read_malloc(None, "read foo[2:5]")?;
        assert_eq!(&*buf, u32_bytes(&[2, 3, 4]).as_slice());
        Ok(())
    }

    #[test]
    fn remove_shrinks_variable() -> Result<()> {
        let path = db_path("remove");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;
        populated(&db)?;

        let mut dest = vec![0u8; 20];
        assert_eq!(db.read(None, &mut dest, "remove foo[0:5]")?, 5);
        assert_eq!(dest, u32_bytes(&[0, 1, 2, 3, 4]));
        assert_eq!(db.get(None, "get foo")?.unwrap().len(), 5);
        Ok(())
    }

    // =========================================================================
    // Plans
    // =========================================================================

    #[test]
    fn plan_reused_for_read() -> Result<()> {
        let path = db_path("plan");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;
        populated(&db)?;

        let plan = Plan::new(&db, "read foo[0:3]")?;
        let mut dest = vec![0u8; 12];
        assert_eq!(plan.read(None, &mut dest)?, 3);
        assert_eq!(dest, u32_bytes(&[0, 1, 2]));
        Ok(())
    }

    #[test]
    fn plan_invalid_query_fails() -> Result<()> {
        let path = db_path("plan_bad");
        let _clean = Cleanup(path.clone());
        let db = Database::open(&path)?;

        assert!(Plan::new(&db, "not a query").is_err());
        Ok(())
    }
}
