//! The only file that changes when numstore is integrated.

// Well under tonic's 4 MiB default message limit.
pub const CHUNK: usize = 64 * 1024;

pub const PULL_TOTAL_DEFAULT: usize = 1024 * 1024;

pub const ERR_BAD_VERB: i32 = 1;
pub const ERR_TOO_MANY_BYTES: i32 = 2;
pub const ERR_BAD_QUERY: i32 = 3;

/// The code and message that go back in-band as an `Error`.
pub type HookError = (i32, String);

pub fn verb(query: &str) -> Option<String> {
    query
        .split_whitespace()
        .next()
        .map(|v| v.to_ascii_lowercase())
}

pub fn check_verb(query: &str, allowed: &[&str]) -> Result<(), HookError> {
    match verb(query) {
        Some(v) if allowed.contains(&v.as_str()) => Ok(()),
        Some(_) => Err((ERR_BAD_VERB, "verb not allowed here".to_string())),
        None => Err((ERR_BAD_QUERY, "empty query".to_string())),
    }
}

/// The total is the last token of the query, so `insert foo 10 100` expects
/// 100 bytes. Fewer is fine - the server keeps waiting; more is an error.
pub struct Push {
    remaining: usize,
}

impl Push {
    pub fn new(query: &str) -> Result<Self, HookError> {
        let total = query
            .split_whitespace()
            .next_back()
            .and_then(|t| t.parse::<usize>().ok())
            .ok_or((
                ERR_BAD_QUERY,
                "last token of the query must be a byte count".to_string(),
            ))?;
        Ok(Self { remaining: total })
    }

    pub fn take(&mut self, n: usize) -> Result<(), HookError> {
        if n > self.remaining {
            return Err((
                ERR_TOO_MANY_BYTES,
                format!("{n} bytes pushed but only {} expected", self.remaining),
            ));
        }
        self.remaining -= n;
        Ok(())
    }
}

/// Shaped as "read up to N bytes, return what you got, empty means end"
/// because that is how the real internal reader works. When that reader lands
/// it will be synchronous, so this call will need `spawn_blocking`.
pub struct Reader {
    pos: usize,
    total: usize,
}

impl Reader {
    pub fn new(query: &str) -> Self {
        // A trailing integer sets the total.
        let total = query
            .split_whitespace()
            .next_back()
            .and_then(|t| t.trim_end_matches(']').rsplit(':').next())
            .and_then(|t| t.parse::<usize>().ok())
            .unwrap_or(PULL_TOTAL_DEFAULT);
        Self { pos: 0, total }
    }

    pub fn read(&mut self, max: usize) -> Vec<u8> {
        let n = max.min(self.total - self.pos);
        let out = (self.pos..self.pos + n).map(|i| (i % 256) as u8).collect();
        self.pos += n;
        out
    }
}

impl Drop for Reader {
    fn drop(&mut self) {
        // Where the real reader releases its pager fixes and transaction.
    }
}

pub fn execute(_query: &str) -> Result<(), HookError> {
    Ok(())
}

pub fn get(_query: &str) -> Vec<u8> {
    b"numstore-var".to_vec()
}
